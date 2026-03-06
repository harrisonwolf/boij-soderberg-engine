#!/usr/bin/env bash
set -euo pipefail

# Benchmark Macaulay2 baseline using built-in generation and pureBetti only.
# Usage:
#   bash scripts/benchmark_m2_builtin.sh [codim] [max_degree]
# Example:
#   bash scripts/benchmark_m2_builtin.sh 6 26

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

C="${1:-6}"
D="${2:-26}"

if ! [[ "${C}" =~ ^[0-9]+$ && "${D}" =~ ^[0-9]+$ ]]; then
  echo "Usage: bash scripts/benchmark_m2_builtin.sh [codim] [max_degree]" >&2
  exit 1
fi

M2_BIN=""
if command -v M2 >/dev/null 2>&1; then
  M2_BIN="M2"
elif command -v macaulay2 >/dev/null 2>&1; then
  M2_BIN="macaulay2"
else
  echo "Macaulay2 executable not found (expected 'M2' or 'macaulay2')." >&2
  exit 2
fi

M2_SCRIPT="$(mktemp)"
cat > "${M2_SCRIPT}" <<EOF
loadPackage "BoijSoederberg";

c = ${C};
d = ${D};

genDegSeqsBuiltIn = (c,d) -> (
    subsets(toList(1..d), c) / (i -> {0} | i)
    );

isBEHViolator = (B,c) -> (
    any(toList(0..c), i -> B_i < binomial(c,i))
    );

isLLBCViolator = (B,c) -> (
    sum B < (2^c * 3/2)
    );

findBadOnesBuiltIn = (c,d) -> (
    possibles := genDegSeqsBuiltIn(c,d);
    select(possibles, D -> (
        B := pureBetti D;
        isBEHViolator(B,c) or isLLBCViolator(B,c)
        ))
    );

print("m2_mode=builtin_only");
print("m2_c=" | toString c | " m2_d=" | toString d);
print("m2_candidate_count=" | toString binomial(d,c));
badOnes = findBadOnesBuiltIn(c,d);
print("m2_bad_count=" | toString(#badOnes));
EOF

echo "Running Macaulay2 built-in baseline on c=${C}, d=${D}..."
/usr/bin/time -f 'm2 elapsed=%E cpu=%P maxrss_kb=%M' "${M2_BIN}" --script "${M2_SCRIPT}"

rm -f "${M2_SCRIPT}"
