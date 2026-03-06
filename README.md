# boij-soderberg-engine

Tools and datasets for experimenting with Boij-Soderberg degree-sequence searches.

## Repository Structure

- `src/` - shared C++ implementation files.
- `include/` - C++ headers used by the applications.
- `apps/` - CLI entrypoint source files (one `main` per executable).
- `scripts/` - shell helpers for running batch generation/merge/parse workflows.
- `data/` - organized datasets and outputs.
- `docs/` - project notes and historical documentation.
- `research/` - snapshots, exploratory scripts, and archived historical artifacts.
- `build/` - generated build output (binaries/objects), created by `make`.

## Build

From the repository root:

```bash
make
```

Default binaries are placed in `build/bin/`:

- `bad_one_generator`
- `parse_huge_output`
- `remove_duplicates`
- `test_program`
- `tell_which_violations`
- `find_big_ones`

Additional binaries can be built individually, for example:

```bash
make L_finder
```
