#!/usr/bin/env bash
#
# mayhem/test.sh — RUN this repo's OWN functional test suite (already built by mayhem/build.sh).
# exit 0 = pass. EDIT per repo. PATCH-grade oracle: after an agent patches the source, the grader
# rebuilds (build.sh) then runs this. DELETE this file if the repo has no meaningful tests.
#
# IMPORTANT:
#  * Must assert BEHAVIOR/OUTPUT, not just exit status. The oracle has to check asserted values /
#    golden-output diffs / known-answer results — so a PATCH that "fixes" a bug by making the program
#    exit(0) (or any no-op) FAILS here. Running inputs and checking only "exit 0 / didn't crash" is
#    NOT a functional test (it's trivially reward-hackable) — use the project's real assertion suite.
#  * Do NOT build here — mayhem/build.sh already compiled the test suite (with the project's normal
#    flags). This script only RUNS the pre-built tests and reports counts. If the test runner is
#    missing, that's a build.sh bug — fail loudly rather than silently rebuilding.
#  * REQUIRED OUTPUT — a CTRF (https://ctrf.io) summary so Mayhem/the PATCH grader reads the counts:
#      - writes a CTRF JSON report to ${CTRF_REPORT:-$SRC/ctrf-report.json}, and
#      - prints a one-line `CTRF {...}` marker to stdout (same JSON, compact).
#    Only `results.summary` (with tests/passed/failed/pending/skipped/other) is required.
#    Use the emit_ctrf helper below; it computes tests = passed+failed+skipped and sets the exit
#    code (0 iff failed==0). Map your framework's output to passed/failed/skipped.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${MAYHEM_JOBS:=$(nproc)}"   # build parallelism; env-overridable, falls back to nproc (use -j"$MAYHEM_JOBS")
cd "$SRC"

# emit_ctrf <tool> <passed> <failed> [skipped] [pending] [other]
# Writes a CTRF report (file + stdout `CTRF {...}` marker) and returns non-zero iff failed>0.
emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

# Upstream's googletest suites (tests/<component>/), built by mayhem/build.sh in build-tests/ with
# the project's normal flags: fileformat (PE/ELF/COFF/Mach-O/IHEX/raw parsing of embedded samples),
# demangler (known mangled -> demangled strings), config, serdes, ctypes, ctypesparser, utils, common.
# Each binary writes a gtest XML report; counts come from those reports, so a binary that exits
# without running its tests (no report) counts as a failure.
mapfile -t BINS < <(find build-tests/tests -type f -name 'retdec-tests-*' -perm -u+x 2>/dev/null | sort)
if [ "${#BINS[@]}" -eq 0 ]; then
  echo "no retdec-tests-* binaries under build-tests/tests — mayhem/build.sh did not build the suite" >&2
  emit_ctrf "googletest" 0 1 0
  exit 1
fi

OUT="$(mktemp -d "${TMPDIR:-/tmp}/retdec-tests.XXXXXX")"
passed=0 failed=0 skipped=0
for b in "${BINS[@]}"; do
  name="$(basename "$b")"
  echo "== $name"
  ( cd "$(dirname "$b")" && "$PWD/$(basename "$b")" --gtest_output="xml:$OUT/$name.xml" ) > "$OUT/$name.log" 2>&1
  rc=$?
  tail -n 3 "$OUT/$name.log"
  if [ ! -s "$OUT/$name.xml" ]; then
    echo "   $name produced no report (exit $rc)" >&2
    failed=$((failed + 1))
    continue
  fi
  read -r p f s < <(python3 -I - "$OUT/$name.xml" <<'PY'
import sys, xml.etree.ElementTree as ET
r = ET.parse(sys.argv[1]).getroot()
tests = int(r.get("tests", 0)); fail = int(r.get("failures", 0)) + int(r.get("errors", 0))
skip = int(r.get("disabled", 0)) + int(r.get("skipped", 0))
print(max(tests - fail - skip, 0), fail, skip)
PY
)
  [ "$rc" -ne 0 ] && [ "$f" -eq 0 ] && f=1
  passed=$((passed + p)); failed=$((failed + f)); skipped=$((skipped + s))
done
rm -rf "$OUT"
emit_ctrf "googletest" "$passed" "$failed" "$skipped"
