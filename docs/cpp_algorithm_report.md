# C++ Algorithm Report

## Scope

This report covers only the active C++ code in `src/`, `include/`, and the current CLI apps in `apps/`. The Macaulay2 code under `research/macaulay2/` is intentionally excluded.

## High-Level Architecture

The repository is organized as a small C++ library plus thin command-line drivers:

- `src/seq_funcs.cc` and `include/seq_funcs.h` contain the original core math and pipeline helpers: degree-sequence generation, pure Betti construction, direct BEH/LLBC predicates, parsing, and deduplication.
- `src/test_funcs.cc` and `include/test_funcs.h` contain the overflow-aware or diagnostic variants of that math: direct conjecture testing from degree sequences, violation reporting, `L(D)` inspection, and an experimental nondegenerate generator.
- `apps/bad_one_generator.cc` is the main search driver. It generates degree sequences, tests conjectures, and writes the violating sequences plus a quick rinsed subset.
- `apps/tell_which_violations.cc`, `apps/remove_duplicates.cc`, and `apps/parse_huge_output.cc` are post-processing tools around that search pipeline.

In other words, the repo is built around an exhaustive-search workflow:

1. generate candidate degree sequences,
2. derive the pure-Betti data needed for a test,
3. evaluate BEH and LLBC,
4. retain violators,
5. parse and deduplicate output.

## Primary Algorithms and Functions

### 1. Degree-sequence generator

- Functions: `gen_subsets_fast`, `gen_deg_seqs`
- Definitions: `src/seq_funcs.cc:111`, `src/seq_funcs.cc:141`
- Main caller: `apps/bad_one_generator.cc:208`

Purpose:

- Enumerate all strictly increasing degree sequences of the form `{0,d1,...,dc}` in fixed codimension `c`, with final degree bounded by `d` and optional lower bound `lowbound`.

Methodology:

- `gen_subsets_fast(start, o, n, lowbound)` recursively generates all size-`o` increasing subsets of `{start,...,n}`.
- The base case emits singletons `i` with `i >= lowbound`.
- The recursive case fixes a leading element `i`, recursively generates the remaining suffix, and prepends `i` to each returned subset.
- `gen_deg_seqs(c, d, lowbound)` then prepends `0` to every generated subset, turning combinations into degree sequences.

Architectural role:

- This is the front door of the search engine. Every exhaustive scan begins here.
- `apps/bad_one_generator.cc` also includes a progress-reporting wrapper, `gen_deg_seqs_with_progress`, but that wrapper preserves the same recursive enumeration strategy (`apps/bad_one_generator.cc:91-117`).

### 2. Pure Betti sequence construction

- Function: `pure_betti`
- Definition: `src/seq_funcs.cc:158`

Purpose:

- Convert a degree sequence `D = {0,d1,...,dc}` into its pure Betti sequence.

Methodology:

- For each Betti position `i`, it forms the Herzog-Kühl style rational product
  over the other degrees using:
  - numerator: product of the other positive degrees,
  - denominator: product of the absolute differences `|d_j - d_i|`.
- It stores each entry as a reduced fraction `(num, den)`.
- It computes a common scaling factor `L` as the `lcm` of the denominators.
- It multiplies numerators by `L` and divides by denominators to obtain integral Betti numbers.

Architectural role:

- This is the mathematical constructor behind the project.
- It is still useful as a reference implementation and for exploratory tools like `find_big_ones`, but the main search path no longer relies on fully materializing pure Betti tables for every candidate.

### 3. Direct conjecture tester on degree sequences

- Functions: `test_conjs`, `test_conjs_v2`
- Definitions: `src/seq_funcs.cc:361`, `src/test_funcs.cc:19`
- Main caller: `apps/bad_one_generator.cc:243-247`

Purpose:

- Decide whether the pure resolution attached to a degree sequence passes the BEH and LLBC lower-bound checks, without having to run the older two-step `pure_betti -> test_BEH/test_LLBC` path.

Methodology:

- The tester first builds each `pi_i` as a product of small fractions `(d_j)/( |d_j-d_i| )` rather than immediately multiplying to a huge integer.
- It performs pairwise `gcd` cancellations across numerators and denominators inside each `pi_i`.
- It computes a common scaling factor `L` from the denominator products.
- It uses an early-exit rule: if an intermediate denominator product or the global `L` already reaches `binom(c, floor(c/2))`, the sequence is declared an automatic pass.
- If no early exit happens, it clears denominators using `L`, multiplies the remaining numerator factors, and checks:
  - BEH: each `B_i >= binom(c, i)`
  - LLBC: total Betti sum `>= 3 * 2^(c-1)`

Architectural role:

- This is the repository’s main algorithmic optimization.
- The core application, `bad_one_generator`, uses `test_conjs` instead of calling `pure_betti`, `test_BEH`, and `test_LLBC` separately.
- `test_conjs_v2` is the more verbose diagnostic variant in `src/test_funcs.cc`; `test_conjs` is the streamlined production version actually used in the main batch search.

### 4. Violation explainer / diagnostic tester

- Function: `which_violations`
- Definition: `src/test_funcs.cc:251`
- Main caller: `apps/tell_which_violations.cc:17-20`

Purpose:

- Explain how a degree sequence fails by reporting exactly which `B_i` violate BEH and whether LLBC also fails.

Methodology:

- It reuses the same factorized computation structure as `test_conjs`:
  - construct fraction lists for each `pi_i`,
  - cancel by `gcd`,
  - compute `L`,
  - short-circuit to pass if `L` is already large enough,
  - otherwise compute the relevant Betti entries.
- Instead of returning a boolean, it emits a string such as failing `B_i` indices and LLBC totals.

Architectural role:

- This is the interpretability layer for the search engine.
- The search pipeline finds candidate violators; this function turns them into human-readable evidence.

### 5. Deduplication by scalar multiples

- Functions: `gcd_rinse`, `rinse_seqs`
- Definitions: `src/seq_funcs.cc:257`, `src/seq_funcs.cc:330`
- Main callers: `apps/bad_one_generator.cc:255-268`, `apps/remove_duplicates.cc:69`

Purpose:

- Remove redundant degree sequences that differ only by an overall scalar multiple.

Methodology:

- `gcd_rinse` is a fast heuristic filter. It checks only the last two entries of a sequence and keeps the sequence when their `gcd` is `1`. This is intentionally not a rigorous equivalence test.
- `rinse_seqs` is the full deduplication pass. It tracks already-seen base sequences and explicitly marks all scalar multiples `fac * seq` up to the largest degree present in the input set. A sequence is kept only if it has not already been marked as a multiple of an earlier one.

Architectural role:

- Deduplication is a core part of the repository’s research workflow, because raw exhaustive scans produce many equivalent sequences.
- The project uses a two-stage approach:
  - quick inline rinsing during search with `gcd_rinse`,
  - exact offline deduplication with `rinse_seqs`.

## Important Secondary/Support Algorithms

These are important to the workflow, but they are not the main mathematical engines.

### 6. BEH and LLBC standalone predicates

- Functions: `test_BEH`, `test_LLBC`
- Definitions: `src/seq_funcs.cc:230`, `src/seq_funcs.cc:247`

Purpose:

- Apply the BEH and LLBC inequalities to an already-materialized pure Betti sequence.

Methodology:

- `test_BEH` checks each Betti entry against `binom(c,i)`.
- `test_LLBC` sums the Betti entries and compares against `3 * 2^(c-1)`.

Architectural role:

- These are conceptually central because they define the conjectural tests the whole repo is built around.
- In implementation terms, they now serve more as the straightforward reference path than the high-throughput production path.

### 7. Sequence parser for post-processing

- Function: `read_degree_seqs`
- Definition: `src/seq_funcs.cc:273`
- Main callers: `apps/parse_huge_output.cc`, `apps/remove_duplicates.cc`, `apps/tell_which_violations.cc`

Purpose:

- Recover brace-form degree sequences from noisy merged output files.

Methodology:

- It scans character-by-character, ignores text until an opening brace, accumulates digits into integers, and emits a sequence on each closing brace.

Architectural role:

- This is what makes long-running batch output reusable: the search stage and the cleanup/reporting stage are decoupled by text files.

## Experimental or Non-Primary C++ Routines

These are present and relevant, but they are not the main current engine.

### 8. Nondegenerate-only generator

- Functions: `is_degen`, `gen_subsets_fast_v2`, `gen_deg_seqs_v2`
- Definitions: `src/test_funcs.cc:179`, `src/test_funcs.cc:192`, `src/test_funcs.cc:237`

Purpose:

- Attempt to prune the search space by excluding sequences considered “degenerate.”

Methodology:

- `is_degen` uses `gcd` checks on adjacent degrees.
- `gen_subsets_fast_v2` reuses the recursive subset generator, but at the top level discards candidates flagged by `is_degen`.

Architectural role:

- This is an exploratory pruning path, exposed mainly through `test_program`, not the main production scan.
- It matters as evidence of the repo’s optimization direction, but it is not the default architecture used by `bad_one_generator`.

### 9. Fraction-form inspection helpers

- Functions: `pi`, `calc_L`
- Definitions: `src/test_funcs.cc:414`, `src/test_funcs.cc:474`

Purpose:

- Expose the internal fraction decomposition and common denominator `L(D)` used by the direct conjecture-testing algorithm.

Methodology:

- `pi` builds the factored rational representation of each pure-Betti entry.
- `calc_L` repeats the cancellation logic and computes the common denominator scale.

Architectural role:

- These are introspection tools for understanding or debugging the main test logic.
- `calc_L` is used by `apps/L_finder.cc`; `pi` is exposed in `test_program`.

## What The Repo Is Primarily Built To Support

If the goal is to name the small set of functions that most directly define the repository, they are:

- `gen_deg_seqs`: exhaustive candidate generation
- `test_conjs`: main high-throughput conjecture tester
- `which_violations`: diagnostic explanation of failures
- `pure_betti`: reference constructor for pure Betti sequences
- `gcd_rinse` and `rinse_seqs`: redundancy removal for search output

The core application-level workflow is:

- `bad_one_generator` = generate with `gen_deg_seqs`, test with `test_conjs`, then quick-rinse with `gcd_rinse`
- `parse_huge_output` = recover clean sequences from merged logs
- `remove_duplicates` = exact scalar-multiple deduplication via `rinse_seqs`
- `tell_which_violations` = explain failed inequalities using `which_violations`

## Notes on Code Status

- `include/degseq.h` defines an incomplete degree-sequence class and is not part of the active architecture.
- `calc_sum` in `src/test_funcs.cc` is stubbed out.
- `find_172.cc` and `foo.cc` are placeholders rather than meaningful parts of the current engine.

