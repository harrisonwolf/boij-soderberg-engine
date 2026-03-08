#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BENCH_SCRIPT="${REPO_ROOT}/scripts/benchmark_against_m2_built_in.sh"
OUT_DIR="${REPO_ROOT}/data/processed/benchmarks"

if [[ $# -lt 4 || $# -gt 5 ]]; then
  echo "Usage: bash scripts/run_until_m2_threshold.sh <codim> <start_d> <step> <threshold_seconds> [lowbound]" >&2
  exit 1
fi

CODIM="$1"
START_D="$2"
STEP="$3"
THRESHOLD="$4"
LOWBOUND="${5:-1}"

CSV_PATH="${OUT_DIR}/codim${CODIM}_until_m2_${THRESHOLD}s.csv"
LOG_DIR="${OUT_DIR}/codim${CODIM}_until_m2_${THRESHOLD}s_logs"

mkdir -p "${LOG_DIR}"

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

extract_n() {
  awk '/^Generated [0-9]+ degree sequences in / { print $2; exit }' "$1"
}

printf '%s\n' \
  "codim,max_degree,n,cpp_total_sec,m2_total_sec,cpp_mem_kb,m2_mem_kb,threshold_sec" \
  > "${CSV_PATH}"

curr_d="${START_D}"
while true; do
  log_path="${LOG_DIR}/c${CODIM}_d${curr_d}.log"
  echo "[run] c=${CODIM} d=${curr_d}"
  bash "${BENCH_SCRIPT}" "${CODIM}" "${curr_d}" "${LOWBOUND}" > "${log_path}"

  n="$(extract_n "${log_path}")"
  cpp_total_sec="$(extract_time_seconds 2 "${log_path}")"
  m2_total_sec="$(extract_time_seconds 4 "${log_path}")"
  cpp_mem_kb="$(extract_equals_value 'maxrss_kb' 1 "${log_path}")"
  m2_mem_kb="$(extract_equals_value 'maxrss_kb' 2 "${log_path}")"

  printf '%s\n' \
    "${CODIM},${curr_d},${n},${cpp_total_sec},${m2_total_sec},${cpp_mem_kb},${m2_mem_kb},${THRESHOLD}" \
    >> "${CSV_PATH}"

  if awk -v total="${m2_total_sec}" -v threshold="${THRESHOLD}" 'BEGIN { exit !(total >= threshold) }'; then
    echo "[stop] M2 total runtime ${m2_total_sec}s exceeded threshold ${THRESHOLD}s at c=${CODIM}, d=${curr_d}"
    break
  fi

  curr_d=$((curr_d + STEP))
done

echo "[done] Wrote ${CSV_PATH}"
