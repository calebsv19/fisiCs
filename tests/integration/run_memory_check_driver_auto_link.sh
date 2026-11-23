#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
RUNTIME_LIB="${2:-build/unsanitized/libfisics_memcheck_runtime.a}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

BIN="$(cd "$(dirname "$BIN")" && pwd -P)/$(basename "$BIN")"
RUNTIME_LIB="$(cd "$(dirname "$RUNTIME_LIB")" && pwd -P)/$(basename "$RUNTIME_LIB")"
if [ ! -f "$RUNTIME_LIB" ]; then
  echo "memory-check runtime archive not found at $RUNTIME_LIB" >&2
  exit 1
fi

unset FISICS_MEMCHECK_RUNTIME_LIB
TMP_DIR=$(mktemp -d)
TMP_OUTPUT=$(mktemp)
trap 'rm -rf "$TMP_DIR" "$TMP_OUTPUT"' EXIT

cat > "$TMP_DIR/memory_check_driver_link.c" <<'SRC'
#include <stdlib.h>

int main(void) {
    void *p = malloc(8);
    if (!p) return 2;
    free(p);
    free(p);
    return 0;
}
SRC

cat > "$TMP_DIR/memory_check_driver_default.c" <<'SRC'
#include <stdlib.h>

int main(void) {
    void *p = malloc(8);
    if (!p) return 2;
    free(p);
    return 0;
}
SRC

# A valid archive at the old project-relative location must never be selected.
PROJECT_RUNTIME=build/unsanitized/libfisics_memcheck_runtime.a
mkdir -p "$TMP_DIR/$(dirname "$PROJECT_RUNTIME")"
cp "$RUNTIME_LIB" "$TMP_DIR/$PROJECT_RUNTIME"
ln -s "$BIN" "$TMP_DIR/compiler-link"

cat > "$TMP_DIR/project.json" <<'JSON'
{
  "schema": "fisiCs.project",
  "version": 0,
  "name": "memory_check_runtime_trust",
  "root": ".",
  "build_dir": "manifest-build",
  "defaults": {
    "standard": "c99",
    "include_dirs": [],
    "defines": [],
    "overlays": ["memory-check"]
  },
  "translation_units": [
    {"source": "memory_check_driver_link.c"}
  ],
  "link": {
    "output": "manifest-build/memory_check_runtime_trust",
    "libraries": [],
    "library_dirs": [],
    "args": []
  }
}
JSON

fail() {
  echo "$1" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
}

check_link() {
  local program="$1" expected="$2"
  shift 2
  (cd "$TMP_DIR" && FISICS_MAX_PROCS=0 "$@") >"$TMP_OUTPUT" 2>&1 ||
    fail "memory-check link failed: $program"
  grep -F -- " $expected " "$TMP_OUTPUT" | grep -Eq '^\[(manifest-)?link\]' ||
    fail "link trace did not identify the expected runtime: $expected"
  "$TMP_DIR/$program" >"$TMP_OUTPUT" 2>&1 || fail "linked program failed: $program"
  grep -Fq "[fisics:memory-check] double free" "$TMP_OUTPUT" ||
    fail "linked program did not report the double free: $program"
}

check_reject() {
  local program="$1" override="$2" diagnostic="$3" status=0
  shift 3
  rm -f "$TMP_DIR/$program"
  (cd "$TMP_DIR" && FISICS_MAX_PROCS=0 FISICS_MEMCHECK_RUNTIME_LIB="$override" "$@") \
    >"$TMP_OUTPUT" 2>&1 || status=$?
  if [[ "$status" != 1 || -e "$TMP_DIR/$program" ]] ||
     grep -Eq '^\[(manifest-)?link\]' "$TMP_OUTPUT" ||
     ! grep -Fq "$diagnostic" "$TMP_OUTPUT"; then
    fail "memory-check link did not reject an invalid runtime: $override"
  fi
}

driver_args=(--overlay=memory-check memory_check_driver_link.c -o memory_check_driver_link)
manifest_args=(--build-manifest project.json)
manifest_output=manifest-build/memory_check_runtime_trust

check_link memory_check_driver_link "$RUNTIME_LIB" ./compiler-link "${driver_args[@]}"
check_link "$manifest_output" "$RUNTIME_LIB" "$BIN" "${manifest_args[@]}"

# An explicit absolute override is honored and logged as its canonical path.
mkdir "$TMP_DIR/override"
cp "$RUNTIME_LIB" "$TMP_DIR/override/runtime.a"
OVERRIDE_RUNTIME="$(cd "$TMP_DIR/override" && pwd -P)/runtime.a"
ln -s "$OVERRIDE_RUNTIME" "$TMP_DIR/override-link.a"
check_link memory_check_driver_link "$OVERRIDE_RUNTIME" \
  env FISICS_MEMCHECK_RUNTIME_LIB="$TMP_DIR/override-link.a" "$BIN" "${driver_args[@]}"
check_link "$manifest_output" "$OVERRIDE_RUNTIME" \
  env FISICS_MEMCHECK_RUNTIME_LIB="$TMP_DIR/override-link.a" "$BIN" "${manifest_args[@]}"

for override in "$PROJECT_RUNTIME" "$TMP_DIR/override" "$TMP_DIR/missing.a"; do
  case "$override" in
    "$PROJECT_RUNTIME") diagnostic='FISICS_MEMCHECK_RUNTIME_LIB must be an absolute path' ;;
    "$TMP_DIR/override") diagnostic='memory-check runtime override is not a regular file' ;;
    *) diagnostic='memory-check runtime override is unavailable' ;;
  esac
  check_reject memory_check_driver_link "$override" "$diagnostic" "$BIN" "${driver_args[@]}"
  check_reject "$manifest_output" "$override" "$diagnostic" "$BIN" "${manifest_args[@]}"
done


if ! FISICS_MAX_PROCS=0 "$BIN" --overlay=memory-check -c \
  "$TMP_DIR/memory_check_driver_link.c" \
  -o "$TMP_DIR/memory_check_driver_link.o" >"$TMP_OUTPUT" 2>&1; then
  echo "memory-check compile-only mode failed" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

for sym in __fisics_memcheck_malloc_site __fisics_memcheck_free_site; do
  if ! nm -u "$TMP_DIR/memory_check_driver_link.o" | grep -Fq "$sym"; then
    echo "compile-only object missing expected memory-check symbol: $sym" >&2
    nm -u "$TMP_DIR/memory_check_driver_link.o" >&2 || true
    exit 1
  fi
done

if ! FISICS_MAX_PROCS=0 "$BIN" \
  "$TMP_DIR/memory_check_driver_default.c" \
  -o "$TMP_DIR/memory_check_driver_default" >"$TMP_OUTPUT" 2>&1; then
  echo "default driver link failed" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if "$TMP_DIR/memory_check_driver_default" >"$TMP_OUTPUT" 2>&1; then
  if grep -Fq "[fisics:memory-check]" "$TMP_OUTPUT"; then
    echo "default executable unexpectedly emitted memory-check diagnostics" >&2
    cat "$TMP_OUTPUT" >&2
    exit 1
  fi
else
  echo "default executable failed" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! FISICS_MAX_PROCS=0 "$BIN" --overlay=all \
  "$TMP_DIR/memory_check_driver_default.c" \
  -o "$TMP_DIR/memory_check_driver_all" >"$TMP_OUTPUT" 2>&1; then
  echo "overlay=all driver link failed" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if "$TMP_DIR/memory_check_driver_all" >"$TMP_OUTPUT" 2>&1; then
  if grep -Fq "[fisics:memory-check]" "$TMP_OUTPUT"; then
    echo "overlay=all unexpectedly emitted memory-check diagnostics" >&2
    cat "$TMP_OUTPUT" >&2
    exit 1
  fi
else
  echo "overlay=all executable failed" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "memory_check_driver_auto_link integration test passed"
