#!/usr/bin/env bash
set -euo pipefail

SHADOW_BIN="${1:-./fisics_shim_shadow}"
SHIM_OVERLAY="${2:-../shared/sys_shims/overlay/include}"
SHIM_INCLUDE="${3:-../shared/sys_shims/include}"
PROFILE_ID="${4:-shim_profile_lang_frontend_shadow_v1}"
PROFILE_VERSION="${5:-1}"
SRC="tests/preprocessor/system_include_language_profile_contract.c"

if [ ! -x "$SHADOW_BIN" ]; then
  echo "shadow compiler not found: $SHADOW_BIN" >&2
  exit 1
fi

assert_success() {
  local include_paths="$1"
  local label="$2"
  local out_file
  out_file="$(mktemp)"
  trap 'rm -f "$out_file"' RETURN

  if ! FISICS_SHIM_PROFILE_ENFORCE=1 \
      FISICS_SHIM_PROFILE_ID="$PROFILE_ID" \
      FISICS_SHIM_PROFILE_VERSION="$PROFILE_VERSION" \
      FISICS_SHIM_PROFILE_EXPECT_ID="$PROFILE_ID" \
      FISICS_SHIM_PROFILE_EXPECT_VERSION="$PROFILE_VERSION" \
      FISICS_SHIM_PROFILE_EXPECT_OVERLAY="$SHIM_OVERLAY" \
      FISICS_SHIM_PROFILE_EXPECT_INCLUDE="$SHIM_INCLUDE" \
      SYSTEM_INCLUDE_PATHS="$include_paths" \
      "$SHADOW_BIN" "$SRC" >"$out_file" 2>&1; then
    echo "$label should have succeeded but failed" >&2
    cat "$out_file" >&2
    exit 1
  fi
  if grep -Eq "Error:|Error at" "$out_file"; then
    echo "$label should have succeeded but emitted errors" >&2
    cat "$out_file" >&2
    exit 1
  fi
}

assert_fail() {
  local include_paths="$1"
  local label="$2"
  local pattern="$3"
  local out_file
  out_file="$(mktemp)"
  trap 'rm -f "$out_file"' RETURN

  FISICS_SHIM_PROFILE_ENFORCE=1 \
    FISICS_SHIM_PROFILE_ID="$PROFILE_ID" \
    FISICS_SHIM_PROFILE_VERSION="$PROFILE_VERSION" \
    FISICS_SHIM_PROFILE_EXPECT_ID="$PROFILE_ID" \
    FISICS_SHIM_PROFILE_EXPECT_VERSION="$PROFILE_VERSION" \
    FISICS_SHIM_PROFILE_EXPECT_OVERLAY="$SHIM_OVERLAY" \
    FISICS_SHIM_PROFILE_EXPECT_INCLUDE="$SHIM_INCLUDE" \
    SYSTEM_INCLUDE_PATHS="$include_paths" \
    "$SHADOW_BIN" "$SRC" >"$out_file" 2>&1 || true
  if ! grep -Eq "Error:|Error at|shim profile contract failed" "$out_file"; then
    echo "$label should have failed with semantic/preprocessor errors" >&2
    cat "$out_file" >&2
    exit 1
  fi
  if ! grep -Eq "$pattern" "$out_file"; then
    echo "$label failed, but not with expected diagnostic pattern: $pattern" >&2
    cat "$out_file" >&2
    exit 1
  fi
}

# Control: valid profile include path ordering succeeds.
assert_success "$SHIM_OVERLAY:$SHIM_INCLUDE:${SYSTEM_INCLUDE_PATHS:-}" "full profile path"

# Negative case: overlay-only (missing shim include root) must fail.
assert_fail "$SHIM_OVERLAY:${SYSTEM_INCLUDE_PATHS:-}" \
  "overlay-only profile path" \
  "shim profile contract failed|could not resolve include 'sys_shims/shim_stdint.h'"

# Negative case: include-only (missing overlay root) must fail.
assert_fail "$SHIM_INCLUDE:${SYSTEM_INCLUDE_PATHS:-}" \
  "include-only profile path" \
  "shim profile contract failed|undeclared identifier|use of undeclared identifier|SHIM_INT32_MAX"

echo "shim language profile negative-path checks passed."
