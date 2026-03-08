#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BENCH_SCRIPT="${REPO_ROOT}/scripts/benchmark_against_m2_built_in.sh"
OUT_DIR="${REPO_ROOT}/data/processed/benchmarks"
LOG_DIR="${OUT_DIR}/builtin_m2_logs"
CSV_PATH="${OUT_DIR}/builtin_m2_matrix.csv"

mkdir -p "${LOG_DIR}"

if [[ ! -x "${BENCH_SCRIPT}" ]]; then
  echo "Missing benchmark script: ${BENCH_SCRIPT}" >&2
  exit 1
fi

elapsed_to_seconds() {
  awk -v t="$1" '
    BEGIN {
      n = split(t, a, ":");
      if (n == 3) printf "%.6f\n", (a[1] * 3600) + (a[2] * 60) + a[3];
      else if (n == 2) printf "%.6f\n", (a[1] * 60) + a[2];
      else printf "%.6f\n", a[1];
    }
  '
}

extract_nth() {
  local pattern="$1"
  local field="$2"
  local index="$3"
  local file="$4"
  awk -v pat="$pattern" -v fld="$field" -v idx="$index" '
    $0 ~ pat {
      count++;
      if (count == idx) {
        print $fld;
        exit;
      }
    }
  ' "$file"
}

extract_time_seconds() {
  local index="$1"
  local file="$2"
  awk -v idx="$index" '
    match($0, /in ([0-9.eE+-]+) seconds/, a) {
      count++;
      if (count == idx) {
        print a[1];
        exit;
      }
    }
  ' "$file"
}

extract_equals_value() {
  local key="$1"
  local index="$2"
  local file="$3"
  awk -v key="$key" -v idx="$index" '
    {
      for (i = 1; i <= NF; i++) {
        if ($i ~ ("^" key "=")) {
          count++;
          if (count == idx) {
            sub("^" key "=", "", $i);
            print $i;
            exit;
          }
        }
      }
    }
  ' "$file"
}

default_pairs=(
  "5 14"
  "5 16"
  "6 15"
  "5 19"
  "6 17"
  "7 16"
  "6 20"
  "7 18"
)

pairs=()
if [[ $# -gt 0 ]]; then
  if (( $# % 2 != 0 )); then
    echo "Usage: bash scripts/run_builtin_benchmark_matrix.sh [c1 d1 c2 d2 ...]" >&2
    exit 1
  fi
  while [[ $# -gt 0 ]]; do
    pairs+=("$1 $2")
    shift 2
  done
else
  pairs=("${default_pairs[@]}")
fi

printf '%s\n' \
  "codim,max_degree,n,cpp_wall_sec,cpp_mem_kb,cpp_total_sec,cpp_gen_sec,cpp_test_sec,m2_wall_sec,m2_mem_kb,m2_total_sec,m2_gen_sec,m2_test_sec" \
  > "${CSV_PATH}"

for pair in "${pairs[@]}"; do
  read -r codim max_degree <<< "${pair}"
  log_path="${LOG_DIR}/c${codim}_d${max_degree}.log"

  echo "[run] c=${codim} d=${max_degree}"
  bash "${BENCH_SCRIPT}" "${codim}" "${max_degree}" > "${log_path}"

  n="$(extract_nth '^Generated [0-9]+ degree sequences in ' 2 1 "${log_path}")"

  cpp_wall_raw="$(extract_equals_value 'elapsed' 1 "${log_path}")"
  cpp_mem_kb="$(extract_equals_value 'maxrss_kb' 1 "${log_path}")"
  cpp_total_sec="$(extract_time_seconds 2 "${log_path}")"
  cpp_gen_sec="$(extract_time_seconds 1 "${log_path}")"
  cpp_test_sec="$(awk -v total="${cpp_total_sec}" -v gen="${cpp_gen_sec}" 'BEGIN { printf "%.6f", total - gen }')"

  m2_wall_raw="$(extract_equals_value 'elapsed' 2 "${log_path}")"
  m2_mem_kb="$(extract_equals_value 'maxrss_kb' 2 "${log_path}")"
  m2_total_sec="$(extract_time_seconds 4 "${log_path}")"
  m2_gen_sec="$(extract_time_seconds 3 "${log_path}")"
  m2_test_sec="$(awk -v total="${m2_total_sec}" -v gen="${m2_gen_sec}" 'BEGIN { printf "%.6f", total - gen }')"

  cpp_wall_sec="$(elapsed_to_seconds "${cpp_wall_raw}")"
  m2_wall_sec="$(elapsed_to_seconds "${m2_wall_raw}")"

  printf '%s\n' \
    "${codim},${max_degree},${n},${cpp_wall_sec},${cpp_mem_kb},${cpp_total_sec},${cpp_gen_sec},${cpp_test_sec},${m2_wall_sec},${m2_mem_kb},${m2_total_sec},${m2_gen_sec},${m2_test_sec}" \
    >> "${CSV_PATH}"
done

echo "[done] Wrote ${CSV_PATH}"
