#!/usr/bin/env bash
set -euo pipefail

# Run the performance-first interview demo flow end-to-end.
# Default workload:
#   - c=6, d=26 (fast nontrivial run)
#   - c=8, d=26 (million+ candidate run)
# Use --fallback to switch the second run to d=24.

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN_DIR="${REPO_ROOT}/build/bin"

FAST_C=6
FAST_D=26
FAST_L=1

BIG_C=8
BIG_D=26
BIG_L=1

if [[ "${1:-}" == "--fallback" ]]; then
  BIG_D=24
fi

if [[ "${1:-}" == "--help" ]]; then
  cat <<'EOF'
Usage: bash scripts/interview_demo.sh [--fallback]

Options:
  --fallback  Use lighter second run (c=8, d=24) for slower machines.
  --help      Show this help text.
EOF
  exit 0
fi

if [[ "${1:-}" != "" && "${1:-}" != "--fallback" ]]; then
  echo "Unknown option: ${1}" >&2
  echo "Run with --help for usage." >&2
  exit 1
fi

cd "${REPO_ROOT}"

if [[ ! -x "${BIN_DIR}/bad_one_generator" ]] \
  || [[ ! -x "${BIN_DIR}/parse_huge_output" ]] \
  || [[ ! -x "${BIN_DIR}/remove_duplicates" ]] \
  || [[ ! -x "${BIN_DIR}/tell_which_violations" ]]; then
  echo "[build] Missing one or more binaries. Running make..."
fi

echo
echo "== Step 1: Build =="
echo "Narration: This pipeline is CLI-first and each stage is measurable."
make

tmp_fast="$(mktemp)"
tmp_big="$(mktemp)"
tmp_parsed="$(mktemp)"
tmp_unique="$(mktemp)"
fast_log="$(mktemp)"
big_log="$(mktemp)"
tell_stderr_log="$(mktemp)"

echo
echo "== Step 2: Fast throughput run (c=${FAST_C}, d=${FAST_D}) =="
echo "Narration: Nontrivial search in under a second on this machine."
/usr/bin/time -f 'elapsed=%E cpu=%P maxrss_kb=%M' \
  "${BIN_DIR}/bad_one_generator" "${tmp_fast}" "${FAST_C}" "${FAST_D}" "${FAST_L}" \
  > "${fast_log}"
rg -n "Generated|Tested|Finished all tests" "${fast_log}"
echo "sample output lines:"
sed -n '1,8p' "${tmp_fast}"

echo
echo "== Step 3: Million+ candidate run (c=${BIG_C}, d=${BIG_D}) =="
echo "Narration: Same algorithm scales to million-level candidate volume."
/usr/bin/time -f 'elapsed=%E cpu=%P maxrss_kb=%M' \
  "${BIN_DIR}/bad_one_generator" "${tmp_big}" "${BIG_C}" "${BIG_D}" "${BIG_L}" \
  > "${big_log}"
rg -n "Generated|Tested|Finished all tests" "${big_log}"
echo "output file line count:"
wc -l "${tmp_big}"

echo
echo "== Step 4: Parse + dedupe pipeline =="
echo "Narration: Generation, parsing, and dedupe are separate measurable stages."
/usr/bin/time -f 'parse elapsed=%E cpu=%P maxrss_kb=%M' \
  "${BIN_DIR}/parse_huge_output" data/samples/burntest.txt "${tmp_parsed}"
/usr/bin/time -f 'rinse elapsed=%E cpu=%P maxrss_kb=%M' \
  "${BIN_DIR}/remove_duplicates" "${tmp_parsed}" "${tmp_unique}"
wc -l "${tmp_parsed}" "${tmp_unique}"

echo
echo "== Step 5: Interpretability output =="
echo "Narration: Output is not only fast; it explicitly reports failed inequalities."
"${BIN_DIR}/tell_which_violations" data/processed/bad_ones_c3_rinsed.txt \
  2> "${tell_stderr_log}" | sed -n '1,20p'
if [[ -s "${tell_stderr_log}" ]]; then
  echo "(debug stderr was captured in ${tell_stderr_log})"
fi

echo
echo "== Step 6: Macaulay2 baseline evidence from repo history =="
echo "Narration: Historical M2 runs in this repo show longer runtime and heap pressure."
rg -n "findBadOnes\\(3,400\\)|Too many heap sections" research/macaulay2/badOneFinder.m2

echo
echo "== Artifacts =="
echo "fast output:   ${tmp_fast}"
echo "big output:    ${tmp_big}"
echo "parsed output: ${tmp_parsed}"
echo "unique output: ${tmp_unique}"
echo "fast log:      ${fast_log}"
echo "big log:       ${big_log}"
echo "tell stderr:   ${tell_stderr_log}"
echo
echo "Tip: run with '--fallback' if the machine is slow."
