# CLI Reference

Detailed notes for the project executables live here instead of the top-level README.

## Main executables

### `boij_soderberg_calculator`

Usage:

```text
boij_soderberg_calculator
```

Interactive calculator for entering a degree sequence as space-separated integers beginning with `0` and printing the pure Betti resolution, `L` value, `pi_i`, `B_i`, and BEH/LLBC status.

Notes:

- Type `q`, `quit`, or `exit` to leave.
- This is intended to be the main entry point as the project expands.

### `bad_one_generator`

Usage:

```text
bad_one_generator [outputfile] [codimension c] [max degree d] [optional: lowbound l]
```

Generates degree sequences with `gen_deg_seqs(c,d,l)`, tests them with the full `pure_betti` + `test_BEH` + `test_LLBC` path, and writes all violators plus a `gcd_rinsed` subset to the output file.

Notes:

- The output file must already exist.
- Large `(c,d)` ranges can become expensive quickly.

### `parse_huge_output`

Usage:

```text
parse_huge_output [input file] [output file]
```

Extracts brace-form degree sequences from merged or noisy logs and writes one cleaned sequence per line.

Notes:

- The output file must already exist.
- This is mainly for post-processing batch outputs.

### `remove_duplicates`

Usage:

```text
remove_duplicates [input_file.txt] [output_file.txt]
```

Removes sequences that are constant multiples of earlier sequences in the input and writes the rinsed list to the output file.

Notes:

- The output file must already exist.
- Compact sequence format like `{0,1,2,3}` is the safest input form.

### `tell_which_violations`

Usage:

```text
tell_which_violations [input filename]
```

Reports which `B_i` values fail BEH and whether LLBC fails for each input sequence, printing `(passed both)` when nothing fails.

Notes:

- Always pass an input filename.
- The current implementation may also emit debug output on `stderr`.

## Development-facing executables

### `test_program`

Usage:

```text
test_program
```

Interactive driver for development and testing of internal routines, including `gen_deg_seqs`, `pure_betti`, `test_conjs_v2`, `gen_deg_seqs_v2`, `is_degen`, `pi`, and `calc_L`.

### `find_big_ones`

Usage:

```text
find_big_ones
```

Runs a built-in scan with `gen_deg_seqs(3,100)` and prints sequences whose pure Betti entries exceed an internal threshold.

## Experimental scratch executables

These binaries are research scratchpads rather than stable interfaces:

- `L_finder`: searches for self-dual patterns with a chosen `L`.
- `quick_run`: ad hoc targeted test loop.
- `find_172`: scaffold for a specific search target.
- `foo`: empty stub.
