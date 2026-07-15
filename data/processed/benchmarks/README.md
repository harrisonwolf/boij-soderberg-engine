# Historical benchmark table

`benchmark_results.csv` is a preserved historical result table, not a reproducibility bundle.

Each row appears to contain one observation for a deterministic `(codimension, max_degree, lowbound)` case. The file does not record timestamps, commands, source commit, compiler flags, Macaulay2/package versions, hardware/OS, raw stdout/stderr, repetition counts, or a failure log. Consequently:

- values such as the `0.0927126`-second Macaulay2 entry for `c=3,d=20` may be described only as historical single-run program time;
- a blank cell means missing data, not OOM;
- `skip` and `timeout` describe the table's historical campaign decisions/outcomes but do not prove why a process stopped;
- the `c=3,d=500` built-in M2 row completed in 1,021 seconds; its blank memory cell is not evidence that it ran out of memory; and
- the `m2port_*` columns must not support correctness or speed claims. The old transcription used an unsound early-pass shortcut and exact-set regression testing found a real misclassification at `c=7,d=12` even where counts looked plausible.

Use `benchmarks/run_benchmarks.py` for new evidence. Its bundles capture the missing provenance, repeat paired runs, retain raw logs, classify timeout and error process outcomes, record any shared-cgroup `oom_kill` delta only as an unattributed concurrent observation, and require exact equality of both the bad and gcd-rinsed result arrays.
