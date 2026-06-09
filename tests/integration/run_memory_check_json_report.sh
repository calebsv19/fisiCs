#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_DIR=$(mktemp -d)
TMP_OUTPUT=$(mktemp)
trap 'rm -rf "$TMP_DIR" "$TMP_OUTPUT"' EXIT

SOURCE="$TMP_DIR/memory_check_json_report.c"
EXE="$TMP_DIR/memory_check_json_report"
REPORT_JSON="$TMP_DIR/memory_check_report.json"

cat > "$SOURCE" <<'SRC'
#include <stdlib.h>

void __fisics_memcheck_report(void);

int main(void) {
    void *p = malloc(21);
    if (!p) return 2;
    __fisics_memcheck_report();
    return 0;
}
SRC

if ! FISICS_MAX_PROCS=0 "$BIN" --overlay=memory-check "$SOURCE" -o "$EXE" >"$TMP_OUTPUT" 2>&1; then
  echo "memory-check JSON report compile/link failed" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! env FISICS_MEMCHECK_REPORT=manual FISICS_MEMCHECK_REPORT_JSON="$REPORT_JSON" "$EXE" >"$TMP_OUTPUT" 2>&1; then
  echo "memory-check JSON report executable failed" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if [ ! -s "$REPORT_JSON" ]; then
  echo "memory-check JSON report was not written" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "[fisics:memory-check] summary: active=1 leaked_bytes=21 allocs=1 frees=0 double_free=0 unknown_free=0 tracker_failures=0" "$TMP_OUTPUT"; then
  echo "memory-check JSON report run changed stderr summary shape" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

python3 - "$REPORT_JSON" <<'PY'
import json
import sys

path = sys.argv[1]
with open(path, "r", encoding="utf-8") as handle:
    report = json.load(handle)

assert report["profile"] == "memory_check_report_v1", report
assert report["schema_version"] == 1, report
assert report["runtime"] == "fisics_memory_check", report
assert report["trigger"] == "manual", report

summary = report["summary"]
expected_summary = {
    "active": 1,
    "leaked_bytes": 21,
    "allocs": 1,
    "frees": 0,
    "double_free": 0,
    "unknown_free": 0,
    "tracker_failures": 0,
}
assert summary == expected_summary, summary

leaks = report["leaks"]
assert len(leaks) == 1, leaks
assert leaks[0]["size"] == 21, leaks
allocated_at = leaks[0]["allocated_at"]
assert allocated_at["file"] == "memory_check_json_report.c", allocated_at
assert isinstance(allocated_at["line"], int) and allocated_at["line"] > 0, allocated_at
PY

echo "memory_check_json_report integration test passed"
