#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN_DIR="${REPO_ROOT}/build/bin"

if [[ $# -lt 2 || $# -gt 3 ]]; then
  echo "Usage: bash scripts/benchmark_against_m2_built_in.sh <codimension> <maxdeg> [lowbound]" >&2
  exit 1
fi

C="$1"
D="$2"
L="${3:-1}"

if [[ ! -x "${BIN_DIR}/bad_one_generator" ]] \
  || [[ "${REPO_ROOT}/apps/bad_one_generator.cc" -nt "${BIN_DIR}/bad_one_generator" ]] \
  || [[ "${REPO_ROOT}/src/seq_funcs.cc" -nt "${BIN_DIR}/bad_one_generator" ]] \
  || [[ "${REPO_ROOT}/include/seq_funcs.h" -nt "${BIN_DIR}/bad_one_generator" ]]; then
  echo "[build] Missing C++ binary. Running make..."
  make -C "${REPO_ROOT}"
fi

M2_BIN="$(command -v M2 || command -v macaulay2 || true)"
if [[ -z "${M2_BIN}" ]]; then
  echo "Macaulay2 is not installed. Expected \`M2\` or \`macaulay2\` on PATH." >&2
  exit 2
fi

cpp_out="$(mktemp)"
cpp_log="$(mktemp)"
m2_script="$(mktemp --suffix=.m2)"
m2_log="$(mktemp)"
cpp_bad_raw="$(mktemp)"
cpp_rinsed="$(mktemp)"
m2_bad_raw="$(mktemp)"
m2_rinsed="$(mktemp)"
cpp_summary="$(mktemp)"
m2_summary="$(mktemp)"
cpp_time_log="$(mktemp)"
m2_time_log="$(mktemp)"
cpp_progress_file="$(mktemp)"
m2_milestone_file="$(mktemp)"

GREEN=$'\033[32m'
RED=$'\033[31m'
RESET=$'\033[0m'

cleanup() {
  rm -f "${m2_script}" "${cpp_progress_file}" "${m2_milestone_file}"
}
trap cleanup EXIT

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

format_elapsed_precise() {
  awk -v total="$1" '
    BEGIN {
      hours = int(total / 3600);
      minutes = int((total - (hours * 3600)) / 60);
      seconds = total - (hours * 3600) - (minutes * 60);
      if (hours > 0) {
        printf "%d:%02d:%07.4f", hours, minutes, seconds;
      } else {
        printf "%d:%07.4f", minutes, seconds;
      }
    }
  '
}

emit_progress_line() {
  local message="$1"
  if ! { printf '%s\n' "${message}" > /dev/tty; } 2>/dev/null; then
    printf '%s\n' "${message}" >&2
  fi
}

run_with_progress() {
  local label="$1"
  local interval="$2"
  local progress_file="${3:-}"
  local milestone_file="${4:-}"
  shift 4

  local start_ts
  start_ts="$(date +%s)"

  "$@" &
  local cmd_pid=$!

  local milestone_pid=""
  if [[ -n "${milestone_file}" ]]; then
    (
      while kill -0 "${cmd_pid}" 2>/dev/null; do
        if [[ -s "${milestone_file}" ]]; then
          emit_progress_line "[progress] ${label} $(<"${milestone_file}")"
          exit 0
        fi
        sleep 0.2
      done
    ) &
    milestone_pid=$!
  fi

  (
    while true; do
      sleep "${interval}"
      if ! kill -0 "${cmd_pid}" 2>/dev/null; then
        exit 0
      fi

      local now_ts elapsed progress_message
      now_ts="$(date +%s)"
      elapsed=$((now_ts - start_ts))
      progress_message="still running"
      if [[ -n "${progress_file}" && -s "${progress_file}" ]]; then
        local progress_line phase count total
        progress_line="$(<"${progress_file}")"
        phase="$(awk '{for (i = 1; i <= NF; i++) if ($i ~ /^phase=/) { sub(/^phase=/, "", $i); print $i; exit }}' <<< "${progress_line}")"
        count="$(awk '{for (i = 1; i <= NF; i++) if ($i ~ /^count=/) { sub(/^count=/, "", $i); print $i; exit }}' <<< "${progress_line}")"
        total="$(awk '{for (i = 1; i <= NF; i++) if ($i ~ /^total=/) { sub(/^total=/, "", $i); print $i; exit }}' <<< "${progress_line}")"
        if [[ -n "${phase}" && "${count}" =~ ^[0-9]+$ && "${total}" =~ ^[0-9]+$ ]]; then
          case "${phase}" in
            generation) progress_message="generated ${count}/${total}" ;;
            testing) progress_message="tested ${count}/${total}" ;;
            done) progress_message="finished ${total}/${total}" ;;
          esac
        fi
      fi
      emit_progress_line "[progress] ${label} ${progress_message} (${elapsed}s elapsed)"
    done
  ) &
  local progress_pid=$!

  wait "${cmd_pid}"
  local cmd_status=$?
  if [[ -n "${milestone_pid}" ]]; then
    kill "${milestone_pid}" 2>/dev/null || true
    wait "${milestone_pid}" 2>/dev/null || true
  fi
  kill "${progress_pid}" 2>/dev/null || true
  wait "${progress_pid}" 2>/dev/null || true
  return "${cmd_status}"
}

extract_after_equals() {
  sed -n "s/.*${1}=\\([^ ]*\\).*/\\1/p"
}

extract_time_seconds() {
  awk '
    match($0, /in ([0-9.eE+-]+) seconds/, a) { print a[1] }
  ' "$1"
}

color_cell() {
  local value="$1"
  local winner="$2"
  local padded
  padded="$(printf '%-22s' "${value}")"
  if [[ "$winner" == "1" ]]; then
    printf "%s%s%s" "${GREEN}" "${padded}" "${RESET}"
  else
    printf "%s%s%s" "${RED}" "${padded}" "${RESET}"
  fi
}

print_row() {
  local label="$1"
  local m2_display="$2"
  local cpp_display="$3"
  local m2_value="$4"
  local cpp_value="$5"
  local lower_is_better="${6:-1}"
  local cpp_wins=0
  local m2_wins=0
  local improvement

  if [[ -n "${m2_value}" && -n "${cpp_value}" ]] && awk -v a="${m2_value}" -v b="${cpp_value}" -v lib="${lower_is_better}" 'BEGIN {
      if (a == b) exit 2;
      if (lib == 1) exit (a < b ? 0 : 1);
      exit (a > b ? 0 : 1);
    }'; then
    m2_wins=1
  else
    case $? in
      1) cpp_wins=1 ;;
      2) cpp_wins=1; m2_wins=1 ;;
    esac
  fi

  improvement="$(awk -v m2="${m2_value}" -v cpp="${cpp_value}" -v lib="${lower_is_better}" 'BEGIN {
    if (m2 == "" || cpp == "") {
      print "n/a";
    } else if ((m2 + 0) == 0 || (cpp + 0) == 0) {
      print "n/a";
    } else if (lib == 1) {
      printf "%.0f%%", ((m2 / cpp) * 100) - 100;
    } else {
      printf "%.0f%%", ((cpp / m2) * 100) - 100;
    }
  }')"

  printf "| %-28s | %b | %b | %-19s |\n" \
    "${label}" \
    "$(color_cell "${m2_display}" "${m2_wins}")" \
    "$(color_cell "${cpp_display}" "${cpp_wins}")" \
    "${improvement}"
}

cat > "${m2_script}" <<EOF
loadPackage "BoijSoederberg";

CODIM = ${C};
MAXDEG = ${D};
LOWBOUND = ${L};
MILESTONE_FILE = "${m2_milestone_file}";

genDegSeqsBuiltIn = (c,d,lowbound) -> (
    lb := lowbound;
    if lb < 1 then lb = 1;
    select(subsets(toList(1..d), c), seq -> last(seq) >= lb) / (seq -> {0} | seq)
    );

testBEH = (B,c) -> (
    any(toList(0..c), i -> B_i < binomial(c,i))
    );

testLLBC = (B,c) -> (
    sum B < (2^c * 3 / 2)
    );

isBad = (D,c) -> (
    B := pureBetti D;
    testBEH(B,c) or testLLBC(B,c)
    );

gcdRinseBuiltIn = (seqs) -> (
    select(seqs, seq -> gcd(seq#(#seq - 1), seq#(#seq - 2)) == 1)
    );

genSecs := cpuTime();
possibleSeqs := genDegSeqsBuiltIn(CODIM, MAXDEG, LOWBOUND);
genSecs = cpuTime() - genSecs;
<<"Generated " << #possibleSeqs << " degree sequences in " << toString(genSecs) << " seconds" << endl;
<<"Generation complete; starting BEH/LLBC tests on " << #possibleSeqs << " degree sequences" << endl;
if MILESTONE_FILE =!= "" then (
    (openOut MILESTONE_FILE) << "Generation complete; starting BEH/LLBC tests on " << #possibleSeqs << " degree sequences" << endl << close;
    );
testStartSecs := cpuTime();
badOnes := select(possibleSeqs, seq -> isBad(seq, CODIM));
rinsedBadOnes := gcdRinseBuiltIn(badOnes);
totalSecs := genSecs + (cpuTime() - testStartSecs);

<<"Tested BEH and LLBC on " << #possibleSeqs << " degree sequences" << endl;
<<"Finished all tests in " << toString(totalSecs) << " seconds" << endl;
<<"badOnesList:" << endl;
for seq in badOnes do (
    << toString(seq) << endl;
    );
<<"gcdRinsedList:" << endl;
for seq in rinsedBadOnes do (
    << toString(seq) << endl;
    );
exit 0
EOF

echo "== C++ benchmark =="
run_with_progress "C++ benchmark" 5 "${cpp_progress_file}" "" \
  /usr/bin/time -f 'elapsed=%E cpu=%P maxrss_kb=%M' \
  env BAD_ONE_PROGRESS_FILE="${cpp_progress_file}" "${BIN_DIR}/bad_one_generator" "${cpp_out}" "${C}" "${D}" "${L}" \
  > "${cpp_log}" 2> "${cpp_time_log}"
cat "${cpp_time_log}"
awk 'BEGIN{mode=0} /^Bad ones for /{mode=1;next} /^gcd_rinsed ones: /{exit} mode==1 && /^\{/' "${cpp_out}" > "${cpp_bad_raw}"
"${BIN_DIR}/remove_duplicates" "${cpp_bad_raw}" "${cpp_rinsed}" > /dev/null
awk '
  /Generated/ { print; next }
  /Generation complete; starting BEH\/LLBC tests/ { print; next }
  /Tested BEH and LLBC/ { print; next }
  /Finished all tests/ { print; next }
' "${cpp_log}" > "${cpp_summary}"
cat "${cpp_summary}"
awk '/^\{/' "${cpp_rinsed}"

echo
echo "== Macaulay2 benchmark (built-ins only) =="
run_with_progress "Macaulay2 benchmark" 5 "" "${m2_milestone_file}" \
  /usr/bin/time -f 'elapsed=%E cpu=%P maxrss_kb=%M' \
  "${M2_BIN}" --script "${m2_script}" \
  > "${m2_log}" 2> "${m2_time_log}"
cat "${m2_time_log}"
awk 'BEGIN{mode=0} /^badOnesList:/{mode=1;next} /^gcdRinsedList:/{mode=0} mode==1 && /^\{/{gsub(/ /,""); print}' "${m2_log}" > "${m2_bad_raw}"
"${BIN_DIR}/remove_duplicates" "${m2_bad_raw}" "${m2_rinsed}" > /dev/null
awk '
  /Generated .* degree sequences in .* seconds/ { print; next }
  /Generation complete; starting BEH\/LLBC tests on .* degree sequences/ { print; next }
  /Tested BEH and LLBC on .* degree sequences/ { print; next }
  /Finished all tests in .* seconds/ { print; next }
' "${m2_log}" > "${m2_summary}"
cat "${m2_summary}"
awk '/^\{/' "${m2_rinsed}"

echo
cpp_elapsed="$(extract_after_equals elapsed < "${cpp_time_log}")"
cpp_cpu="$(extract_after_equals cpu < "${cpp_time_log}" | tr -d '%')"
cpp_mem="$(extract_after_equals maxrss_kb < "${cpp_time_log}")"
m2_elapsed="$(extract_after_equals elapsed < "${m2_time_log}")"
m2_cpu="$(extract_after_equals cpu < "${m2_time_log}" | tr -d '%')"
m2_mem="$(extract_after_equals maxrss_kb < "${m2_time_log}")"

cpp_gen_secs="$(extract_time_seconds "${cpp_summary}" | sed -n '1p')"
cpp_total_secs="$(extract_time_seconds "${cpp_summary}" | sed -n '2p')"
cpp_test_secs="$(awk -v total="${cpp_total_secs}" -v gen="${cpp_gen_secs}" 'BEGIN { printf "%.6f", total - gen }')"
m2_gen_secs="$(extract_time_seconds "${m2_summary}" | sed -n '1p')"
m2_total_secs="$(extract_time_seconds "${m2_summary}" | sed -n '2p')"
m2_test_secs="$(awk -v total="${m2_total_secs}" -v gen="${m2_gen_secs}" 'BEGIN { printf "%.6f", total - gen }')"

cpp_total_display="$(format_elapsed_precise "${cpp_total_secs}")"
m2_total_display="$(format_elapsed_precise "${m2_total_secs}")"

cpp_elapsed_secs="$(elapsed_to_seconds "${cpp_elapsed}")"
m2_elapsed_secs="$(elapsed_to_seconds "${m2_elapsed}")"
cpp_elapsed_display="$(format_elapsed_precise "${cpp_elapsed_secs}")"
m2_elapsed_display="$(format_elapsed_precise "${m2_elapsed_secs}")"

echo "Comparison"
printf "| %-28s | %-22s | %-22s | %-19s |\n" "Metric" "M2" "C++" "C++ % Improvement"
printf "|-%-28s-|-%-22s-|-%-22s-|-%-19s-|\n" "$(printf '%.0s-' {1..28})" "$(printf '%.0s-' {1..22})" "$(printf '%.0s-' {1..22})" "$(printf '%.0s-' {1..19})"
print_row "Total wall time elapsed" "${m2_elapsed_display}" "${cpp_elapsed_display}" "${m2_elapsed_secs}" "${cpp_elapsed_secs}"
print_row "CPU usage" "${m2_cpu}%" "${cpp_cpu}%" "${m2_cpu}" "${cpp_cpu}"
print_row "Memory usage" "${m2_mem} KB" "${cpp_mem} KB" "${m2_mem}" "${cpp_mem}"
print_row "Program-reported total" "${m2_total_display}" "${cpp_total_display}" "${m2_total_secs}" "${cpp_total_secs}"
print_row "Degree-sequence time" "${m2_gen_secs}s" "${cpp_gen_secs}s" "${m2_gen_secs}" "${cpp_gen_secs}"
print_row "Conjecture-test time" "${m2_test_secs}s" "${cpp_test_secs}s" "${m2_test_secs}" "${cpp_test_secs}"
