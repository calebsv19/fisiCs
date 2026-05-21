#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TMP_DIR="$(mktemp -d "${TMPDIR:-/tmp}/fisics_make_final_monitored.XXXXXX")"
trap 'rm -rf "$TMP_DIR"' EXIT

CSV_PATH="$TMP_DIR/make_final_timing_log.csv"
NOTES_PATH="$TMP_DIR/make_final_timing_notes.md"
BASELINE_PATH="$TMP_DIR/make_final_timing_baseline.json"
RUNS_ROOT="$TMP_DIR/runs"

cat >"$CSV_PATH" <<'EOF'
timestamp_utc,git_head,tag,runs,median_seconds,mean_seconds,min_seconds,max_seconds,pass_count,fail_count,skip_count,total_count,avg_seconds_per_test,last_run_status
2026-05-06T06:02:55Z,5dd3edd,manual,1,438.469854,438.469854,438.469854,438.469854,2518,0,36,2554,0.171680,ok
2026-05-12T21:23:57Z,5ce87aa,manual,1,399.926575,399.926575,399.926575,399.926575,2532,0,36,2568,0.155735,ok
2026-05-15T16:46:32Z,ac35ba4,manual,1,479.876251,479.876251,479.876251,479.876251,2532,0,36,2568,0.186868,ok
EOF

python3 "$ROOT/scripts/run_make_final_monitored.py" \
  --csv "$CSV_PATH" \
  --notes "$NOTES_PATH" \
  --baseline-json "$BASELINE_PATH" \
  --runs-root "$RUNS_ROOT" \
  --tag smoke \
  --note "integration smoke" \
  --min-first-check-seconds 1 \
  --first-check-buffer-seconds 0 \
  --poll-seconds 1 \
  --stall-seconds 5 \
  -- \
  python3 -c "print('PASS smoke__monitored'); print(); print('0 failing, 0 skipped')"

test -f "$BASELINE_PATH"
test -f "$NOTES_PATH"
test -d "$RUNS_ROOT"

STATUS_FILE="$(find "$RUNS_ROOT" -name status.json | head -n 1)"
test -n "$STATUS_FILE"
test -f "$STATUS_FILE"

grep -q '"state": "completed"' "$STATUS_FILE"
grep -q '"baseline_source": "median_3"' "$STATUS_FILE"
grep -q '"sample_count": 4' "$BASELINE_PATH"
grep -q '"last_success_tag": "smoke"' "$BASELINE_PATH"
grep -q 'integration smoke' "$NOTES_PATH"
grep -q 'smoke' "$CSV_PATH"

echo "make final monitored smoke: PASS"
