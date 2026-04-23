#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR=$(cd "$(dirname "$0")/.." && pwd)
DEFAULT_LOG_PATH="$ROOT_DIR/../docs/private_program_docs/fisiCs/audits/make_final_timing_log.csv"
DEFAULT_NOTES_PATH="$ROOT_DIR/../docs/private_program_docs/fisiCs/audits/make_final_timing_notes.md"

RUNS="${FINAL_TIMING_RUNS:-1}"
LOG_PATH="${FINAL_TIMING_LOG:-$DEFAULT_LOG_PATH}"
NOTES_PATH="${FINAL_TIMING_NOTES:-$DEFAULT_NOTES_PATH}"
TAG="${FINAL_TIMING_TAG:-manual}"
NOTE="${FINAL_TIMING_NOTE:-}"

usage() {
  cat <<USAGE
Usage: $(basename "$0") [--runs N] [--log PATH] [--tag LABEL]
                           [--notes PATH] [--note TEXT]

Captures timing for 'make final' and appends one summarized row to the CSV log.

Options:
  --runs N      Number of full-suite runs to execute (default: 1)
  --log PATH    CSV log output path
                (default: ../docs/private_program_docs/fisiCs/audits/make_final_timing_log.csv)
  --notes PATH  Notes markdown output path
                (default: ../docs/private_program_docs/fisiCs/audits/make_final_timing_notes.md)
  --tag LABEL   Freeform label for the capture (default: manual)
  --note TEXT   Optional one-line note appended to the notes markdown
USAGE
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --runs)
      RUNS="${2:-}"
      shift 2
      ;;
    --log)
      LOG_PATH="${2:-}"
      shift 2
      ;;
    --tag)
      TAG="${2:-}"
      shift 2
      ;;
    --notes)
      NOTES_PATH="${2:-}"
      shift 2
      ;;
    --note)
      NOTE="${2:-}"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown argument: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
done

case "$RUNS" in
  ''|*[!0-9]*)
    echo "--runs must be a positive integer" >&2
    exit 2
    ;;
esac
if [ "$RUNS" -le 0 ]; then
  echo "--runs must be greater than 0" >&2
  exit 2
fi

mkdir -p "$(dirname "$LOG_PATH")"
if [ ! -f "$LOG_PATH" ]; then
  echo "timestamp_utc,git_head,tag,runs,median_seconds,mean_seconds,min_seconds,max_seconds,pass_count,fail_count,skip_count,total_count,avg_seconds_per_test,last_run_status" > "$LOG_PATH"
fi

orig_dir=$(pwd)
cd "$ROOT_DIR"

durations=""
last_pass=0
last_fail=0
last_skip=0
last_status="ok"

for run_idx in $(seq 1 "$RUNS"); do
  run_log=$(mktemp)
  start=$(perl -MTime::HiRes=time -e 'printf "%.6f", time')
  if make -C "$ROOT_DIR" final >"$run_log" 2>&1; then
    status="ok"
  else
    status="fail"
  fi
  end=$(perl -MTime::HiRes=time -e 'printf "%.6f", time')

  elapsed=$(awk -v s="$start" -v e="$end" 'BEGIN{printf "%.6f", (e-s)}')
  durations="$durations $elapsed"

  pass_count=$(grep -c '^PASS ' "$run_log" || true)
  fail_count=$(grep -c '^FAIL ' "$run_log" || true)
  skip_count=$(grep -c '^SKIP ' "$run_log" || true)

  last_pass=$pass_count
  last_fail=$fail_count
  last_skip=$skip_count
  last_status=$status

  rm -f "$run_log"

done

read -r median mean minv maxv <<<"$(printf '%s\n' "$durations" | awk '
  {
    for (i = 1; i <= NF; ++i) {
      n += 1
      v[n] = $i + 0
      sum += v[n]
      if (n == 1 || v[n] < min) min = v[n]
      if (n == 1 || v[n] > max) max = v[n]
    }
  }
  END {
    if (n == 0) {
      printf "0 0 0 0"
      exit
    }
    for (i = 1; i <= n; ++i) {
      for (j = i + 1; j <= n; ++j) {
        if (v[j] < v[i]) {
          t = v[i]
          v[i] = v[j]
          v[j] = t
        }
      }
    }
    if (n % 2 == 1) med = v[(n + 1) / 2]
    else med = (v[n / 2] + v[n / 2 + 1]) / 2.0
    mean = sum / n
    printf "%.6f %.6f %.6f %.6f", med, mean, min, max
  }
')"

total_count=$((last_pass + last_fail + last_skip))
avg_per_test=$(awk -v total="$total_count" -v med="$median" 'BEGIN{if (total > 0) printf "%.6f", med/total; else printf "0.000000"}')
timestamp_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)
git_head=$(git rev-parse --short HEAD)

printf "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n" \
  "$timestamp_utc" "$git_head" "$TAG" "$RUNS" "$median" "$mean" "$minv" "$maxv" \
  "$last_pass" "$last_fail" "$last_skip" "$total_count" "$avg_per_test" "$last_status" \
  >> "$LOG_PATH"

note_clean=$(printf '%s' "$NOTE" | tr '\n' ' ' | sed -E 's/[[:space:]]+/ /g; s/^ //; s/ $//')
if [ -n "$note_clean" ]; then
  mkdir -p "$(dirname "$NOTES_PATH")"
  if [ ! -f "$NOTES_PATH" ]; then
    cat > "$NOTES_PATH" <<NOTES_HEADER
# make final timing notes

Use this file for notable context attached to timing captures.

NOTES_HEADER
  fi
  printf -- "- %s | head=%s | tag=%s | median=%ss | total=%s | avg_per_test=%ss | status=%s | note=%s\n" \
    "$timestamp_utc" "$git_head" "$TAG" "$median" "$total_count" "$avg_per_test" "$last_status" "$note_clean" \
    >> "$NOTES_PATH"
fi

echo "Appended timing snapshot to: $LOG_PATH"
echo "timestamp_utc=$timestamp_utc"
echo "git_head=$git_head"
echo "runs=$RUNS"
echo "median_seconds=$median"
echo "mean_seconds=$mean"
echo "pass_count=$last_pass fail_count=$last_fail skip_count=$last_skip total_count=$total_count"
echo "avg_seconds_per_test=$avg_per_test"
echo "last_run_status=$last_status"
if [ -n "$note_clean" ]; then
  echo "Appended note to: $NOTES_PATH"
fi

cd "$orig_dir"
