# boij-soderberg-engine

C++ tooling and datasets for Boij-Soderberg-style degree-sequence experiments, centered on pure Betti tables and searches for sequences that violate BEH or LLBC-style checks. The repository is CLI-first and research-oriented rather than a polished end-user package, is not intended as a general-purpose Boij-Soderberg library API, and may require parameter tuning for very large runs.

## What This Repo Does

This repository is used to:

- generate candidate degree sequences in fixed codimension,
- compute pure Betti sequences from those degree sequences,
- calculate `L` values and related `pi_i`/`B_i` data,
- test conjectural constraints such as BEH and LLBC,
- collect potential counterexamples ("bad ones"),
- parse and deduplicate large batch outputs.

Historical outputs and research artifacts are kept in-repo for reproducibility. Longer operational notes, detailed CLI usage, and batch workflow material have been moved into [docs/cli_reference.md](/mnt/c/Users/wolve/.codex/worktrees/1037/boij-soderberg-engine-repo/docs/cli_reference.md) and [docs/batch_workflows.md](/mnt/c/Users/wolve/.codex/worktrees/1037/boij-soderberg-engine-repo/docs/batch_workflows.md).

## Subdirectories

- `apps/`: executable entry points and one-off research drivers.
- `src/`: shared implementation for sequence generation, Betti computations, checks, and utilities.
- `include/`: headers for the shared C++ code.
- `scripts/`: shell helpers for demos, benchmarks, and batch processing workflows.
- `data/`: stored inputs and outputs used by the project.
- `data/raw/`: raw generated or collected outputs before cleanup.
- `data/processed/`: parsed, rinsed, or benchmark-ready outputs.
- `data/samples/`: small files for smoke tests and examples.
- `data/archive/`: older retained outputs that are no longer part of the main workflow.
- `docs/`: project notes and extended documentation moved out of the README.
- `docs/notes/`: smaller supporting notes and scratch documentation.
- `research/`: archived experiments, historical code snapshots, and Macaulay2 material.
- `research/archive/`: legacy generated artifacts and old code snapshots.
- `research/macaulay2/`: Macaulay2 scripts and benchmarking inputs.
- `build/`: generated build artifacts created by `make`.
- `build/bin/`: compiled executables.
- `build/obj/`: compiled object files.
- `.codex/`: Codex workspace metadata for this worktree.

## Build

Requirements:

- `g++` (or a compatible C++ compiler)
- `make`

Build the default toolset from the repository root:

```bash
make
```

This includes the main binaries in `build/bin/`, including:

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

Build the project:

```bash
make
```

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

For the scripted interview walkthrough, see [docs/interview_demo.md](/mnt/c/Users/wolve/.codex/worktrees/1037/boij-soderberg-engine-repo/docs/interview_demo.md).

## Main Tools/Algorithms

Main executables:

- `boij_soderberg_calculator`: interactive entry point for entering a degree sequence and seeing its pure Betti resolution, `L` value, `pi_i`, `B_i`, and BEH/LLBC status.
- `bad_one_generator`: generates degree sequences in a search window and writes out those that fail one or both checks.
- `parse_huge_output`: extracts brace-form degree sequences from noisy merged outputs.
- `remove_duplicates`: removes constant-multiple duplicates from a sequence list.
- `tell_which_violations`: reports which `B_i` values fail BEH and whether LLBC fails.

Core algorithms and concepts:

- **Degree sequence**: represented as `{0,d1,d2,...,dc}` with strictly increasing entries.
- **Codimension `c`**: inferred as `sequence_length - 1`.
- **Pure Betti sequence**: computed by `pure_betti(...)` in [src/seq_funcs.cc](/mnt/c/Users/wolve/.codex/worktrees/1037/boij-soderberg-engine-repo/src/seq_funcs.cc).
- **BEH check**: `test_BEH(...)` compares Betti entries against binomial lower bounds.
- **LLBC check**: `test_LLBC(...)` compares the total Betti sum against its lower bound.
- **`L` value**: computed from the reduced `pi_i` denominators, used to scale a pure Betti table to integers.
- **`pi_i` values**: rational factors associated to a degree sequence before clearing denominators.
- **Bad ones**: degree sequences whose pure Betti data fail BEH, LLBC, or both.
- **Rinsing/deduplication**: `gcd_rinse(...)` is a quick filter and `rinse_seqs(...)` removes constant-multiple duplicates more thoroughly.

Detailed executable usage, scratch binaries, and development-facing tools are documented in [docs/cli_reference.md](/mnt/c/Users/wolve/.codex/worktrees/1037/boij-soderberg-engine-repo/docs/cli_reference.md).

## Data Format

Preferred sequence format:

```text
{0,1,2,3}
```

Parsing expectations:

- Degree sequences are modeled as strictly increasing sequences beginning with `0`.
- `parse_huge_output` scans for `{...}` patterns and extracts comma-separated integers.
- Compact brace format with no spaces is the safest form for downstream tools such as `remove_duplicates`.

Additional parsing notes, batch scripts, benchmark commands, and legacy workflow details are documented in [docs/batch_workflows.md](/mnt/c/Users/wolve/.codex/worktrees/1037/boij-soderberg-engine-repo/docs/batch_workflows.md).
