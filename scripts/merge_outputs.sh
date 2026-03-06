#!/usr/bin/env bash
set -euo pipefail

# Merge burn chunk files into the codimension-specific huge output file.
c=6
increment=2

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
RAW_BURN_DIR="${REPO_ROOT}/data/raw/burnfiles/c${c}"
HUGE_OUTPUT="${RAW_BURN_DIR}/huge_output.txt"

mkdir -p "${RAW_BURN_DIR}"
touch "${HUGE_OUTPUT}"

for (( n=0; n<=22; n++ )); do
  start=$((n * increment + 1))
  end=$((start + increment - 1))
  cat "${RAW_BURN_DIR}/burn${start}-${end}.txt" >> "${HUGE_OUTPUT}"
done
