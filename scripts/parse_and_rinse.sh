#!/usr/bin/env bash
set -euo pipefail

# Parse huge burn output and rinse duplicates into the processed dataset.
c=8

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN_DIR="${REPO_ROOT}/build/bin"
RAW_INPUT="${REPO_ROOT}/data/raw/burnfiles/c${c}/huge_output.txt"
PROCESSED_DIR="${REPO_ROOT}/data/processed/burnfiles/c${c}"
PARSED_OUTPUT="${PROCESSED_DIR}/huge_output_parsed.txt"
UNIQUE_OUTPUT="${PROCESSED_DIR}/huge_output_unique.txt"

if [[ ! -x "${BIN_DIR}/parse_huge_output" ]]; then
  echo "Missing ${BIN_DIR}/parse_huge_output. Run 'make parse_huge_output' first."
  exit 1
fi
if [[ ! -x "${BIN_DIR}/remove_duplicates" ]]; then
  echo "Missing ${BIN_DIR}/remove_duplicates. Run 'make remove_duplicates' first."
  exit 1
fi

mkdir -p "${PROCESSED_DIR}"
rm -f "${PARSED_OUTPUT}" "${UNIQUE_OUTPUT}"
touch "${PARSED_OUTPUT}" "${UNIQUE_OUTPUT}"

"${BIN_DIR}/parse_huge_output" "${RAW_INPUT}" "${PARSED_OUTPUT}"
"${BIN_DIR}/remove_duplicates" "${PARSED_OUTPUT}" "${UNIQUE_OUTPUT}"
