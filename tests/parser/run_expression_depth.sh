#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
MAX_DEPTH="$(awk '$2 == "FISICS_FRONTEND_MAX_EXPRESSION_DEPTH" { value = $3; sub(/u$/, "", value); print value }' \
  "$ROOT_DIR/src/Compiler/frontend_limits.h")"
if [[ ! "$MAX_DEPTH" =~ ^[0-9]+$ ]]; then
  echo "unable to read frontend expression depth limit" >&2
  exit 1
fi
STRESS_DEPTH=$((MAX_DEPTH + 1))

# Limit only the compiler subprocess, preserving the test driver/toolchain stack.
run_compiler() (
  ulimit -s 1024
  "$BIN" "$@"
)

WORK_DIR="$(mktemp -d)"
trap 'rm -rf "$WORK_DIR"' EXIT

write_case() {
  local path="$1"
  local prefix="$2"
  local suffix="$3"
  local depth="$4"
  {
    printf 'int main(void) { return '
    for ((i = 0; i < depth; ++i)); do printf '%s' "$prefix"; done
    printf '1'
    for ((i = 0; i < depth; ++i)); do printf '%s' "$suffix"; done
    printf '; }\n'
  } >"$path"
}

write_case "$WORK_DIR/shallow.c" '(' ')' 32
run_compiler -c "$WORK_DIR/shallow.c" -o "$WORK_DIR/shallow.o" \
  >"$WORK_DIR/shallow.log" 2>&1

for kind in grouped unary; do
  if [ "$kind" = grouped ]; then
    write_case "$WORK_DIR/$kind.c" '(' ')' "$STRESS_DEPTH"
  else
    write_case "$WORK_DIR/$kind.c" '!' '' "$STRESS_DEPTH"
  fi

  status=0
  run_compiler -c "$WORK_DIR/$kind.c" -o "$WORK_DIR/$kind.o" \
    >"$WORK_DIR/$kind.log" 2>&1 || status=$?
  if [ "$status" -ne 1 ]; then
    echo "$kind depth case returned $status instead of a diagnostic rejection" >&2
    cat "$WORK_DIR/$kind.log" >&2
    exit 1
  fi
  if ! grep -q "expression nesting limit exceeded" "$WORK_DIR/$kind.log"; then
    echo "$kind depth case omitted the nesting-limit diagnostic" >&2
    cat "$WORK_DIR/$kind.log" >&2
    exit 1
  fi
  if [ -e "$WORK_DIR/$kind.o" ]; then
    echo "$kind depth case produced an object after its fatal parser diagnostic" >&2
    exit 1
  fi
done

# Adjacent comments must use constant stack, while operators remain intact.
{
  for ((i = 0; i < 10000; ++i)); do
    if ((i % 2)); then printf '//line\n'; else printf '/*block*/ '; fi
  done
  printf 'int main(void) { int value = 8; value /= 2; return value / 2; }\n'
} >"$WORK_DIR/comments.c"
run_compiler -c "$WORK_DIR/comments.c" -o "$WORK_DIR/comments.o" \
  >"$WORK_DIR/comments.log" 2>&1

# The ceiling still admits near-limit grouped and unary expressions.
for kind in grouped unary; do
  if [ "$kind" = grouped ]; then
    write_case "$WORK_DIR/near-$kind.c" '(' ')' "$((MAX_DEPTH - 1))"
  else
    write_case "$WORK_DIR/near-$kind.c" '!' '' "$((MAX_DEPTH - 1))"
  fi
  run_compiler -c "$WORK_DIR/near-$kind.c" -o "$WORK_DIR/near-$kind.o" \
    >"$WORK_DIR/near-$kind.log" 2>&1
done

echo "parser expression depth and comment tests passed (1024 KiB stack)."
