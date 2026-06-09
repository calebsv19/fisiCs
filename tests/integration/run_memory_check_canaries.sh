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

run_canary() {
  local name="$1"
  local source="$TMP_DIR/${name}.c"
  local exe="$TMP_DIR/${name}"
  shift

  if ! FISICS_MAX_PROCS=0 "$BIN" --overlay=memory-check "$source" -o "$exe" >"$TMP_OUTPUT" 2>&1; then
    echo "memory-check canary compile/link failed: $name" >&2
    cat "$TMP_OUTPUT" >&2
    exit 1
  fi

  if ! "$exe" >"$TMP_OUTPUT" 2>&1; then
    echo "memory-check canary executable failed: $name" >&2
    cat "$TMP_OUTPUT" >&2
    exit 1
  fi

  for expected in "$@"; do
    if ! grep -Fq "$expected" "$TMP_OUTPUT"; then
      echo "memory-check canary missing expected output for $name: $expected" >&2
      cat "$TMP_OUTPUT" >&2
      exit 1
    fi
  done
}

run_canary_env() {
  local name="$1"
  local env_assignment="$2"
  local source="$TMP_DIR/${name}.c"
  local exe="$TMP_DIR/${name}"
  shift 2

  if ! FISICS_MAX_PROCS=0 "$BIN" --overlay=memory-check "$source" -o "$exe" >"$TMP_OUTPUT" 2>&1; then
    echo "memory-check canary compile/link failed: $name" >&2
    cat "$TMP_OUTPUT" >&2
    exit 1
  fi

  if ! env "$env_assignment" "$exe" >"$TMP_OUTPUT" 2>&1; then
    echo "memory-check canary executable failed: $name" >&2
    cat "$TMP_OUTPUT" >&2
    exit 1
  fi

  for expected in "$@"; do
    if ! grep -Fq "$expected" "$TMP_OUTPUT"; then
      echo "memory-check canary missing expected output for $name: $expected" >&2
      cat "$TMP_OUTPUT" >&2
      exit 1
    fi
  done
}

run_canary_no_output_env() {
  local name="$1"
  local env_assignment="$2"
  local source="$TMP_DIR/${name}.c"
  local exe="$TMP_DIR/${name}"

  if ! FISICS_MAX_PROCS=0 "$BIN" --overlay=memory-check "$source" -o "$exe" >"$TMP_OUTPUT" 2>&1; then
    echo "memory-check canary compile/link failed: $name" >&2
    cat "$TMP_OUTPUT" >&2
    exit 1
  fi

  if ! env "$env_assignment" "$exe" >"$TMP_OUTPUT" 2>&1; then
    echo "memory-check canary executable failed: $name" >&2
    cat "$TMP_OUTPUT" >&2
    exit 1
  fi

  if [ -s "$TMP_OUTPUT" ]; then
    echo "memory-check canary expected no output for $name" >&2
    cat "$TMP_OUTPUT" >&2
    exit 1
  fi
}

cat > "$TMP_DIR/clean_free_null.c" <<'SRC'
#include <stdlib.h>

void __fisics_memcheck_report(void);

int main(void) {
    void *p = malloc(8);
    if (!p) return 2;
    free(0);
    free(p);
    __fisics_memcheck_report();
    return 0;
}
SRC

run_canary "clean_free_null" \
  "[fisics:memory-check] summary: active=0 leaked_bytes=0 allocs=1 frees=1 double_free=0 unknown_free=0 tracker_failures=0"

cat > "$TMP_DIR/leak_report.c" <<'SRC'
#include <stdlib.h>

void __fisics_memcheck_report(void);

int main(void) {
    void *p = malloc(13);
    if (!p) return 2;
    __fisics_memcheck_report();
    return 0;
}
SRC

run_canary "leak_report" \
  "[fisics:memory-check] summary: active=1 leaked_bytes=13 allocs=1 frees=0 double_free=0 unknown_free=0 tracker_failures=0"

cat > "$TMP_DIR/double_free.c" <<'SRC'
#include <stdlib.h>

void __fisics_memcheck_report(void);

int main(void) {
    void *p = malloc(8);
    if (!p) return 2;
    free(p);
    free(p);
    __fisics_memcheck_report();
    return 0;
}
SRC

run_canary "double_free" \
  "[fisics:memory-check] double free" \
  "[fisics:memory-check] summary: active=0 leaked_bytes=0 allocs=1 frees=1 double_free=1 unknown_free=0 tracker_failures=0"

cat > "$TMP_DIR/unknown_pointer_free.c" <<'SRC'
#include <stdlib.h>

void __fisics_memcheck_report(void);

int main(void) {
    int local = 0;
    free(&local);
    __fisics_memcheck_report();
    return 0;
}
SRC

run_canary "unknown_pointer_free" \
  "[fisics:memory-check] unknown pointer free" \
  "[fisics:memory-check] summary: active=0 leaked_bytes=0 allocs=0 frees=0 double_free=0 unknown_free=1 tracker_failures=0"

cat > "$TMP_DIR/realloc_success.c" <<'SRC'
#include <stdlib.h>

void __fisics_memcheck_report(void);

int main(void) {
    void *p = malloc(8);
    if (!p) return 2;
    p = realloc(p, 16);
    if (!p) return 3;
    free(p);
    __fisics_memcheck_report();
    return 0;
}
SRC

run_canary "realloc_success" \
  "[fisics:memory-check] summary: active=0 leaked_bytes=0 allocs=2 frees=2 double_free=0 unknown_free=0 tracker_failures=0"

cat > "$TMP_DIR/auto_exit_leak_site.c" <<'SRC'
#include <stdlib.h>

int main(void) {
    void *p = malloc(21);
    if (!p) return 2;
    return 0;
}
SRC

run_canary "auto_exit_leak_site" \
  "[fisics:memory-check] summary: active=1 leaked_bytes=21 allocs=1 frees=0 double_free=0 unknown_free=0 tracker_failures=0" \
  "[fisics:memory-check] leak: size=21 allocated_at=auto_exit_leak_site.c:4"

cat > "$TMP_DIR/report_policy_errors_clean.c" <<'SRC'
#include <stdlib.h>

int main(void) {
    void *p = malloc(8);
    if (!p) return 2;
    free(p);
    return 0;
}
SRC

run_canary_no_output_env "report_policy_errors_clean" "FISICS_MEMCHECK_REPORT=errors"

cat > "$TMP_DIR/report_policy_leaks_double_free.c" <<'SRC'
#include <stdlib.h>

int main(void) {
    void *p = malloc(8);
    if (!p) return 2;
    free(p);
    free(p);
    return 0;
}
SRC

run_canary_env "report_policy_leaks_double_free" "FISICS_MEMCHECK_REPORT=leaks" \
  "[fisics:memory-check] double free" \
  "[fisics:memory-check]   free_site=report_policy_leaks_double_free.c:7" \
  "[fisics:memory-check] summary: active=0 leaked_bytes=0 allocs=1 frees=1 double_free=1 unknown_free=0 tracker_failures=0"

echo "memory_check_canaries integration test passed"
