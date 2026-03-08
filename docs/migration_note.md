# Repository Migration Note

This document records the structural migration from the original mixed-layout repository to the current organized layout.

## High-Level Path Mapping

- `code/cppCode/*.cc` (core implementation) -> `src/`
- `code/cppCode/*.h` -> `include/`
- `code/cppCode/*main-style .cc` -> `apps/`
- `code/cppCode/*.sh` -> `scripts/`
- `code/cppCode/README` -> `docs/legacy_cppcode_notes.md`
- `code/.cppCode_backup_*` -> `research/snapshots/cppCode_backup_*`
- `code/*.m2` -> `research/macaulay2/`
- editor backup files (`*~`, `#*#`) -> `research/archive/editor-backups/`
- generated binaries/objects from active code area -> `research/archive/generated/`
- `code/cppCode/burnfiles` -> `data/raw/burnfiles`
- parsed/unique burnfile outputs -> `data/processed/burnfiles`
- `data/bad_ones_c*.txt` -> `data/raw/`
- `data/bad_ones_c*_rinsed.txt` -> `data/processed/`

## Deferred (Intentional)

- No algorithmic or feature-level code changes were made.
- Legacy snapshot content under `research/snapshots/` was preserved as-is, including legacy build artifacts and scripts.
