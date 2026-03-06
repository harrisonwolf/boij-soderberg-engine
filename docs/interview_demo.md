# Interview Demo Runbook (Performance-First)

This runbook is for an 8-12 minute interview walkthrough that emphasizes:

1. high-throughput search,
2. clean staged pipeline,
3. interpretable mathematical outputs,
4. evidence-based comparison to a live Macaulay2 run when available.

## One-command demo

From repo root:

```bash
bash scripts/interview_demo.sh
```

Fallback for slower machines:

```bash
bash scripts/interview_demo.sh --fallback
```

## Manual command sequence

```bash
cd /home/wolve/projects/boij-soderberg-engine-repo
make

tmp_fast=$(mktemp)
/usr/bin/time -f 'elapsed=%E cpu=%P maxrss_kb=%M' ./build/bin/bad_one_generator "$tmp_fast" 6 26 1

tmp_big=$(mktemp)
/usr/bin/time -f 'elapsed=%E cpu=%P maxrss_kb=%M' ./build/bin/bad_one_generator "$tmp_big" 8 26 1

tmp_parsed=$(mktemp)
tmp_unique=$(mktemp)
/usr/bin/time -f 'parse elapsed=%E cpu=%P maxrss_kb=%M' ./build/bin/parse_huge_output data/samples/burntest.txt "$tmp_parsed"
/usr/bin/time -f 'rinse elapsed=%E cpu=%P maxrss_kb=%M' ./build/bin/remove_duplicates "$tmp_parsed" "$tmp_unique"
wc -l "$tmp_parsed" "$tmp_unique"

./build/bin/tell_which_violations data/processed/bad_ones_c3_rinsed.txt | sed -n '1,20p'

bash scripts/benchmark_against_m2.sh 3 40 1
```

## Narration prompts (short)

- "This C++ pipeline separates generation, parsing, deduplication, and violation analysis, so each stage is measurable."
- "On this machine, the `c=8,d=26` run processes ~1.56M sequences in a few seconds."
- "This repo now has a standalone M2 benchmark script so the comparison is against an actual M2 run, not my C++ note."
- "If M2 is unavailable on the interview machine, I can still show the historical notes as background, but not as benchmark evidence."

## Rehearsal checklist

1. Run the full script once and record your exact timing numbers.
2. Keep one saved terminal log screenshot/text snippet with your best run.
3. If needed, use `--fallback` or manually switch to `d=24` on the big run.
4. Use `mktemp` outputs to satisfy `bad_one_generator`'s existing-file requirement.
