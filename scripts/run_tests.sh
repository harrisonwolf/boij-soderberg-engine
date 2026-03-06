#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN_DIR="${REPO_ROOT}/build/bin"
RAW_BURN_DIR="${REPO_ROOT}/data/raw/burnfiles"

c=8
increment=1

if [[ ! -x "${BIN_DIR}/bad_one_generator" ]]; then
  echo "Missing ${BIN_DIR}/bad_one_generator. Run 'make bad_one_generator' first."
  exit 1
fi

mkdir -p "${RAW_BURN_DIR}/c${c}"
touch "${RAW_BURN_DIR}/c${c}/huge_output.txt"

for (( n=29; n<=500; n++ )); do
  start=$((n * increment + 1))
  end=$((start + increment - 1))
  echo "running gen_bad_ones on ${start} to ${end}"
  burn_file="${RAW_BURN_DIR}/c${c}/burn${start}-${end}.txt"
  touch "${burn_file}"
  "${BIN_DIR}/bad_one_generator" "${burn_file}" "${c}" "${end}" "${start}"
  cat "${burn_file}" >> "${RAW_BURN_DIR}/c${c}/huge_output.txt"
done
