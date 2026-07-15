# Benchmark evidence studio

This directory produces task-matched, self-validating C++/Macaulay2 evidence bundles. It is deliberately stricter than the historical CSV: a timing is publishable only when both engines complete the same task and return identical exact results.

## Task contract

For each `(codimension, max_degree, lowbound)` case, both engines:

1. materialize the same candidate degree-sequence family;
2. compute the full pure Betti vector;
3. mark a sequence bad when it violates BEH or LLBC;
4. apply the same gcd rinse; and
5. emit the complete bad and gcd-rinsed sequence arrays.

The C++ path calls `gen_deg_seqs`, `pure_betti`, `test_BEH`, `test_LLBC`, and `gcd_rinse`. The oracle path loads Macaulay2's `BoijSoederberg` package and calls `pureBetti`, followed by explicit BEH/LLBC and gcd-rinse checks. The historical shortcut transcription is not used.

Every run starts with a `c=7,d=12,lowbound=1` correctness sentinel. It must produce 792 candidates, 28 bad sequences, and 26 gcd-rinsed sequences, including `{0,1,2,3,4,5,11,12}`. This is the case the historical shortcut misclassified even though a count could appear plausible. The studio also runs a known-safe arithmetic probe and a deliberate signed-64-bit overflow probe; the latter has exact `B_4 = 14,043,766,264,500,157,900`, so its failure is a genuine final-output overflow. The benchmark test gate separately verifies scale invariance through the formerly false-overflow sequence `{0,1000,2000,3000,4000,5000,6000,7000}`.

## Measurement policy

- C++ and Macaulay2 execute as a pair on one recorded machine.
- Order alternates by repetition to reduce order bias.
- External wall time and peak RSS come from GNU `time`; internal phase times are diagnostic.
- The summary uses medians over successful paired repetitions and preserves the planned, observed, and successful sample counts.
- Process outcomes are recorded as `timeout` or `error`. A positive shared-cgroup `oom_kill` counter delta is retained only as an unattributed concurrent observation; it is never proof that the measured process exhausted memory. A blank value, timeout, or signal also never proves OOM.
- The task is deterministic, so no random seed is applicable; inputs and exact result arrays are recorded instead.

The smoke profile has three repetitions and checks plumbing/correctness. Standard has five repetitions and is the default reporting profile. Headline has three larger repetitions with per-engine timeouts; run it only when its cost is intentional.

The strict validator accepts only complete, all-success paired bundles. The runner validates under a hidden staging directory whose bundle basename already matches the run ID, then atomically moves an approved bundle into `benchmarks/runs/<run-id>`. A completed run with a failed pair or failed validator moves to `benchmarks/runs/quarantine/<run-id>` instead; incomplete staging data is deleted. Quarantined diagnostics are deliberately not publication evidence and are expected to fail the strict validator.

## Published evidence

- [`20260715T180543Z-2e0daec3-smoke`](runs/20260715T180543Z-2e0daec3-smoke/) — pre-fix clean commit `2e0daec3`, two cases, three paired repetitions per case, six of six successful pairs, and exact bad/gcd-rinsed result equality throughout. It validates the benchmark driver’s exact task path, but predates consolidation of the public search helpers and is not evidence of repository-wide algorithm-suite health.

Smoke evidence establishes the harness and small-case measurements on its recorded machine. It is not a substitute for the denser standard/research scaling profiles.


## Run and validate

Requirements are `g++`, GNU `time`, Python 3, and Macaulay2 with `BoijSoederberg`.

```bash
make benchmark-smoke
make benchmark-standard
make benchmark-headline
make benchmark-tools-test
make benchmark-validate BUNDLE=benchmarks/runs/<run-id>
```

The benchmark make targets first require all 15 algorithm suites (300 fixture cases) plus the benchmark-tool tests to pass. The runner then refuses a dirty repository unless `--allow-dirty` is supplied explicitly. That option exists for scratch diagnostics; the validator rejects dirty bundles by default, so committed evidence must come from a clean build whose embedded commit/compiler metadata matches the repository manifest.

Each immutable bundle contains:

- a manifest with the full commit, build contract, exact commands, profile, Macaulay2 version, `BoijSoederberg` version, and oracle-template hash;
- sanitized machine/tool metadata (no username, hostname, `PATH`, or absolute workstation path);
- raw stdout, stderr, and GNU `time` output for every engine invocation;
- the rendered M2 scripts and normalized exact result arrays;
- per-pair records, JSON/CSV summaries, arithmetic/correctness preflight evidence; and
- a complete SHA-256 inventory.

`validate_bundle.py` rereads the arrays rather than trusting stored counts or hashes. It rejects missing repetitions, order drift, changed task/build metadata, malformed degree sequences, non-finite measurements, result-set differences, checksum changes, unsafe paths, private workstation identifiers, and dirty publication bundles. Negative tests prove that an equal count with one substituted bad or gcd-rinsed sequence is rejected.

## Interpreting results

Do not mix measurement boundaries. Historical values in `data/processed/benchmarks/benchmark_results.csv` are program-time observations; the studio's primary comparison is end-to-end external wall time, which includes interpreter/package startup. Quote the metric, profile, median, repetition count, machine, bundle path, and commit together.

Both implementations materialize the candidate list. The studio establishes performance only for its recorded cases and machine. It does not prove streaming behavior, an arbitrary-point input path, universal memory safety, or completion of a historical multi-billion-candidate campaign.
