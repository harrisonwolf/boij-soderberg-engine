# boij-soderberg-engine

A hand-written C++ engine for experiments in Boij–Söderberg theory. It enumerates candidate degree sequences, computes their pure Betti tables, and searches for sequences that violate two Betti-number lower-bound conjectures — the Buchsbaum–Eisenbud–Horrocks ("BEH") rank bound and an "LLBC" total-rank bound.

It was built in 2024 for the research behind a co-authored paper, *Arithmetic in the Boij–Söderberg Cone* (Boocher, Huang, Wolf, [arXiv:2512.24320](https://arxiv.org/abs/2512.24320)), where its exhaustive searches surfaced the counterexample candidates the paper then classifies. The repository is CLI-first and research-oriented, not a general-purpose library.

## Provenance

The ~1,200-line mathematical core — `src/seq_funcs.cc`, `src/test_funcs.cc`, `src/binom.cc` and their headers (`gen_deg_seqs`, `pure_betti`, `test_BEH`, `test_LLBC`, `calc_L`) — is hand-written, no AI, and is cross-validated case-by-case against Macaulay2's own `BoijSoederberg` package (`pureBetti`). The repository was later reorganized and given its test and benchmark harness with the help of an AI coding agent.

## What This Repo Does

- generate candidate degree sequences in fixed codimension,
- compute pure Betti sequences from those degree sequences,
- calculate `L` values and related `pi_i`/`B_i` data,
- test conjectural lower bounds (BEH and LLBC),
- collect potential counterexamples ("bad ones"),
- parse and deduplicate large batch outputs.

Historical outputs and research artifacts are kept in-repo for reproducibility. Longer operational notes, detailed CLI usage, and batch workflow material live in [docs/cli_reference.md](docs/cli_reference.md) and [docs/batch_workflows.md](docs/batch_workflows.md).

## Performance evidence

The following values are **historical, single-run program-time measurements** preserved in `data/processed/benchmarks/benchmark_results.csv`. They are useful orientation, not publication-grade benchmark evidence: the historical table does not contain the command, machine, tool versions, raw logs, repetitions, or a seed (the task is deterministic). A blank memory cell means “not recorded,” not “out of memory.”

| codim | sequences (`n`) | historical C++ | historical Macaulay2 built-in | ratio |
| ---: | ---: | ---: | ---: | ---: |
| 5 | 1,221,759 | 0.88 s | 100.6 s | 114× |
| 6 | 906,192 | 0.81 s | 93.1 s | 115× |
| 7 | 1,184,040 | 1.79 s | 156.4 s | 87× |
| 3 | 20,708,500 | 11.0 s | 1,021 s (~17 min) | 93× |
| 6 | 15,890,700 | 20.6 s | historical timeout at 25 min | — |

Fresh runs use the evidence studio in [`benchmarks/`](benchmarks/README.md). It records the commit and compiler flags, machine and tool metadata, Macaulay2 and `BoijSoederberg` versions, commands, raw output, GNU `time` measurements, repeated paired runs with alternating order, exact result arrays, and checksums. A bundle is rejected unless C++ and Macaulay2 agree on every bad sequence and every gcd-rinsed sequence; matching counts alone is insufficient.

The task materializes its full candidate list, so memory grows with the candidate count. That fact predicts memory pressure; it does not prove a particular run exhausted RAM. The studio reports timeout, confirmed OOM, and other errors separately, and scopes any confirmed OOM to the recorded machine and run.

Historical archive headers show a large 2024 batch campaign. They do not include the complete commands, low bounds, per-run manifests, or completion records needed to prove an exhaustive cumulative sweep. Accordingly, the portfolio-scale figure of about 59.6 billion should be described only as a combinatorial estimate or historical archive scale, not as a newly manifested completed sweep.

The old standalone Macaulay2 transcription contained an unsound early-pass shortcut: for `{0,1,2,3,4,5,11,12}`, it treated `L >= binomial(7,3)` as sufficient even though the exact Betti vector `{42,252,616,770,495,132,2,1}` violates BEH at index 6 (`2 < 7`). That path now computes the full Betti vector and the comparison script requires exact set equality. Historical `m2port_*` columns predate this correction and should not support claims.

## Build

Requirements:

- `g++` (or a compatible C++ compiler)
- `make`

Build the default toolset from the repository root:

```bash
make
```

This builds the main binaries in `build/bin/`, including:

- `boij_soderberg_calculator`
- `bad_one_generator`
- `parse_huge_output`
- `remove_duplicates`
- `tell_which_violations`
- `test_program`
- `find_big_ones`

Build extra research tools individually:

```bash
make L_finder quick_run find_172 foo
```

If your compiler defaults are older, build with C++17 explicitly:

```bash
make CXXFLAGS="-O2 -Wall -Wextra -std=c++17"
```

Clean generated build output:

```bash
make clean
```

## Quick Start

Run the interactive calculator:

```bash
./build/bin/boij_soderberg_calculator
```

Sample input:

```text
0 3 5 8
```

For a file-based workflow:

```bash
touch /tmp/bad_ones_demo.txt
./build/bin/bad_one_generator /tmp/bad_ones_demo.txt 3 30 1

touch /tmp/parsed_demo.txt
./build/bin/parse_huge_output data/samples/burntest.txt /tmp/parsed_demo.txt

touch /tmp/unique_demo.txt
./build/bin/remove_duplicates /tmp/parsed_demo.txt /tmp/unique_demo.txt

./build/bin/tell_which_violations /tmp/unique_demo.txt
```

## Main Tools / Algorithms

Main executables:

- `boij_soderberg_calculator`: interactive entry point for entering a degree sequence and seeing its pure Betti resolution, `L` value, `pi_i`, `B_i`, and BEH/LLBC status.
- `bad_one_generator`: generates degree sequences in a search window and writes out those that fail one or both checks (full `pure_betti` + `test_BEH` + `test_LLBC` path).
- `parse_huge_output`: extracts brace-form degree sequences from noisy merged outputs.
- `remove_duplicates`: removes constant-multiple duplicates from a sequence list.
- `tell_which_violations`: reports which `B_i` values fail BEH and whether LLBC fails.

Core algorithms and concepts:

- **Degree sequence**: represented as `{0,d1,d2,...,dc}` with strictly increasing entries.
- **Codimension `c`**: inferred as `sequence_length - 1`.
- **Pure Betti sequence**: computed by `pure_betti(...)` in `src/seq_funcs.cc` via a reformulation of the Herzog–Kühl equations.
- **BEH check**: `test_BEH(...)` compares each Betti entry against its binomial lower bound.
- **LLBC check**: `test_LLBC(...)` compares the total Betti sum against its lower bound.
- **`L` value**: computed from the reduced `pi_i` denominators, used to scale a pure Betti table to integers.
- **`pi_i` values**: rational factors associated to a degree sequence before clearing denominators.
- **Bad ones**: degree sequences whose pure Betti data fail BEH, LLBC, or both.
- **Rinsing/deduplication**: `gcd_rinse(...)` is a quick filter and `rinse_seqs(...)` removes constant-multiple duplicates more thoroughly.

Detailed executable usage and development-facing tools are documented in [docs/cli_reference.md](docs/cli_reference.md).

## Data Format

Preferred sequence format:

```text
{0,1,2,3}
```

Parsing expectations:

- Degree sequences are modeled as strictly increasing sequences beginning with `0`.
- `parse_huge_output` scans for `{...}` patterns and extracts comma-separated integers.
- Compact brace format with no spaces is the safest form for downstream tools such as `remove_duplicates`.

Additional parsing notes, batch scripts, and benchmark commands are documented in [docs/batch_workflows.md](docs/batch_workflows.md).

## Repository Layout

- `apps/` — executable entry points and one-off research drivers.
- `src/` — shared implementation for sequence generation, Betti computations, checks, and utilities.
- `include/` — headers for the shared C++ code.
- `scripts/` — shell helpers for demos, benchmarks, and batch processing.
- `data/` — stored inputs and outputs (`raw/`, `processed/`, `samples/`, `archive/`).
- `docs/` — CLI reference and batch-workflow notes.
- `research/` — archived experiments, historical code snapshots, and Macaulay2 material.
- `build/` — generated build artifacts (`make`).
