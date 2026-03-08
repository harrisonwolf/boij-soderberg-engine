# Theorem-Enabling Engineering Blurb

The C++ library and search pipeline in this repository were not just a convenience improvement over a Macaulay2 workflow. Based on the benchmark results in this repo, they were an enabling research tool for the conjecture-testing campaign that ultimately contributed to a new theorem.

For the repository's actual task, the C++ implementation consistently outperformed a built-in Macaulay2 baseline by a very large margin. Across the measured benchmark matrix, the C++ code was about `33.24x` faster end-to-end on average, about `38.43x` faster in the conjecture-testing phase itself, and about `20.49x` more memory-efficient. In the extended fixed-codimension `c=7` sweep, the first built-in M2 run above 5 minutes occurred at `c=7, d=32`, where the same workload took `13.0959s` in C++ versus `415.285s` in M2, with C++ using `240,400 KB` peak resident memory versus `1,290,816 KB` for M2.

Those measured gaps matter because the real research runs were vastly larger than the benchmark cases: they involved generating and testing degree sequences on the order of billions to trillions and ran for days or weeks in C++. Using the measured slowdown factors from the benchmarks, an equivalent built-in M2 pipeline would likely have taken months for jobs that took days in C++, and years for jobs that took weeks. That makes the central conclusion technically defensible: the performance engineering and architecture of this custom C++ library were part of the causal chain that made the team's large-scale computational validation feasible within the project timeline.


> The custom C++ conjecture-testing engine developed for this project made the large-scale computational search feasible in practice, and that computation was instrumental in converting a conjectural program into theorem-level results.

