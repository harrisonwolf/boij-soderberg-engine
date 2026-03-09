# Batch Workflows And Extra Notes

This page holds the longer operational notes that were moved out of the README.

## End-to-end pipeline

1. Build tools with `make`.
2. Generate candidate outputs with `bad_one_generator` or `scripts/run_tests.sh`.
3. Merge chunked outputs into `huge_output.txt` if needed.
4. Parse merged output with `parse_huge_output`.
5. Remove constant-multiple duplicates with `remove_duplicates`.
6. Inspect failures with `tell_which_violations`.

## Batch scripts

Scripts live in `scripts/` and expect binaries in `build/bin/`.

### `scripts/run_tests.sh`

Runs `bad_one_generator` over degree windows, writes per-window files under `data/raw/burnfiles/c*/`, and appends each window to `huge_output.txt` without truncating an existing file first.

Hardcoded defaults:

- `c=8`
- `increment=1`
- loop `n=29..500`

Run:

```bash
bash scripts/run_tests.sh
```

### `scripts/merge_outputs.sh`

Concatenates burn chunks into a codimension-specific `huge_output.txt`.

Hardcoded defaults:

- `c=6`
- `increment=2`

Run:

```bash
bash scripts/merge_outputs.sh
```

### `scripts/parse_and_rinse.sh`

Parses `data/raw/burnfiles/c*/huge_output.txt` into `huge_output_parsed.txt` and `huge_output_unique.txt`.

Hardcoded default:

- `c=8`

Run:

```bash
bash scripts/parse_and_rinse.sh
```

## Data and parsing notes

Preferred per-line sequence format:

```text
{0,1,2,3}
```

Parser behavior:

- `{...}` patterns with comma-separated integers are extracted.
- Header text and surrounding noise are tolerated.
- Compact brace format is preferred; whitespace-heavy forms like `{0, 1, 2, 3}` are less reliable for `remove_duplicates`.

## Demo and benchmark commands

Interview demo:

```bash
bash scripts/interview_demo.sh
```

Fallback demo profile:

```bash
bash scripts/interview_demo.sh --fallback
```

Runbook and narration prompts: [docs/interview_demo.md](/mnt/c/Users/wolve/.codex/worktrees/1037/boij-soderberg-engine-repo/docs/interview_demo.md)

Direct Macaulay2 baseline:

```bash
bash scripts/benchmark_m2_builtin.sh 6 26
```

This requires `M2` or `macaulay2` on `PATH`.

## Related documentation

- [docs/migration_note.md](/mnt/c/Users/wolve/.codex/worktrees/1037/boij-soderberg-engine-repo/docs/migration_note.md): notes on the structural migration from the legacy layout.
- [docs/legacy_cppcode_notes.md](/mnt/c/Users/wolve/.codex/worktrees/1037/boij-soderberg-engine-repo/docs/legacy_cppcode_notes.md): preserved operational notes from older versions.
- `research/`: archive and snapshot material; active development is centered in `apps/`, `src/`, `include/`, `scripts/`, and `data/`.
