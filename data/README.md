# Data Layout

- `raw/` - unprocessed or minimally processed inputs and generated chunk outputs.
- `processed/` - parsed/rinsed outputs derived from `raw/`.
- `samples/` - small sample datasets and example input/output files.
- `archive/` - legacy one-off outputs retained for historical reference.

## Burnfile Convention

- `raw/burnfiles/c*/` stores per-range burn chunks and `huge_output.txt`.
- `processed/burnfiles/c*/` stores `huge_output_parsed.txt` and `huge_output_unique.txt`.
