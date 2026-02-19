#!/usr/bin/env bash
set -euo pipefail

BASE_BIN="${1:-./fisics}"
SHADOW_BIN="${2:-./fisics_shim_shadow}"
SHIM_OVERLAY="${3:-../shared/sys_shims/overlay/include}"
SHIM_INCLUDE="${4:-../shared/sys_shims/include}"

if [ ! -x "$BASE_BIN" ]; then
  echo "baseline compiler not found: $BASE_BIN" >&2
  exit 1
fi
if [ ! -x "$SHADOW_BIN" ]; then
  echo "shadow compiler not found: $SHADOW_BIN" >&2
  exit 1
fi

run_case() {
  local bin="$1"
  local src="$2"
  local out_file="$3"
  local use_shadow_env="$4"

  if [ "$use_shadow_env" = "1" ]; then
    DISABLE_CODEGEN=1 SYSTEM_INCLUDE_PATHS="$SHIM_OVERLAY:$SHIM_INCLUDE:${SYSTEM_INCLUDE_PATHS:-}" "$bin" "$src" >"$out_file" 2>&1
  else
    DISABLE_CODEGEN=1 "$bin" "$src" >"$out_file" 2>&1
  fi
}

assert_contains() {
  local file="$1"
  local needle="$2"
  local context="$3"
  if ! grep -Fq "$needle" "$file"; then
    echo "parity check failed: missing '$needle' in $context" >&2
    cat "$file" >&2
    exit 1
  fi
}

assert_not_contains() {
  local file="$1"
  local needle="$2"
  local context="$3"
  if grep -Fq "$needle" "$file"; then
    echo "parity check failed: unexpected '$needle' in $context" >&2
    cat "$file" >&2
    exit 1
  fi
}

assert_semantic_ok() {
  local file="$1"
  local context="$2"
  assert_contains "$file" "Semantic analysis: no issues found." "$context"
  assert_not_contains "$file" "Error:" "$context"
}

TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

BASE_SYS="$TMP_DIR/base_system.log"
SHADOW_SYS="$TMP_DIR/shadow_system.log"
BASE_SEARCH="$TMP_DIR/base_search.log"
SHADOW_SEARCH="$TMP_DIR/shadow_search.log"
BASE_DOMAINS="$TMP_DIR/base_domains.log"
SHADOW_DOMAINS="$TMP_DIR/shadow_domains.log"

run_case "$BASE_BIN" "tests/preprocessor/system_include_bool.c" "$BASE_SYS" 0
run_case "$SHADOW_BIN" "tests/preprocessor/system_include_bool.c" "$SHADOW_SYS" 1
assert_semantic_ok "$BASE_SYS" "baseline system include"
assert_semantic_ok "$SHADOW_SYS" "shadow system include"

run_case "$BASE_BIN" "tests/preprocessor/include_search_order.c" "$BASE_SEARCH" 0
run_case "$SHADOW_BIN" "tests/preprocessor/include_search_order.c" "$SHADOW_SEARCH" 1
assert_semantic_ok "$BASE_SEARCH" "baseline include search"
assert_semantic_ok "$SHADOW_SEARCH" "shadow include search"

run_case "$BASE_BIN" "tests/preprocessor/system_include_shim_domains.c" "$BASE_DOMAINS" 0
run_case "$SHADOW_BIN" "tests/preprocessor/system_include_shim_domains.c" "$SHADOW_DOMAINS" 1
assert_semantic_ok "$BASE_DOMAINS" "baseline shim domain includes"
assert_semantic_ok "$SHADOW_DOMAINS" "shadow shim domain includes"

echo "shim quiet parity passed: baseline and shadow include behavior matches semantic checks."
