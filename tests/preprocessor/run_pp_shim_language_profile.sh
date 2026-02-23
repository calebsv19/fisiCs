#!/usr/bin/env bash
set -euo pipefail

BASE_BIN="${1:-./fisics}"
SHADOW_BIN="${2:-./fisics_shim_shadow}"
SHIM_OVERLAY="${3:-../shared/sys_shims/overlay/include}"
SHIM_INCLUDE="${4:-../shared/sys_shims/include}"
PROFILE_ID="${5:-shim_profile_lang_frontend_shadow_v1}"
PROFILE_VERSION="${6:-1}"
SRC="tests/preprocessor/system_include_language_profile_mix.c"

if [ ! -x "$BASE_BIN" ]; then
  echo "baseline compiler not found: $BASE_BIN" >&2
  exit 1
fi
if [ ! -x "$SHADOW_BIN" ]; then
  echo "shadow compiler not found: $SHADOW_BIN" >&2
  exit 1
fi

BASE_OUT="$(mktemp)"
SHADOW_OUT="$(mktemp)"
trap 'rm -f "$BASE_OUT" "$SHADOW_OUT"' EXIT

"$BASE_BIN" "$SRC" >"$BASE_OUT" 2>&1
FISICS_SHIM_PROFILE_ENFORCE=1 \
FISICS_SHIM_PROFILE_ID="$PROFILE_ID" \
FISICS_SHIM_PROFILE_VERSION="$PROFILE_VERSION" \
FISICS_SHIM_PROFILE_EXPECT_ID="$PROFILE_ID" \
FISICS_SHIM_PROFILE_EXPECT_VERSION="$PROFILE_VERSION" \
FISICS_SHIM_PROFILE_EXPECT_OVERLAY="$SHIM_OVERLAY" \
FISICS_SHIM_PROFILE_EXPECT_INCLUDE="$SHIM_INCLUDE" \
SYSTEM_INCLUDE_PATHS="$SHIM_OVERLAY:$SHIM_INCLUDE:${SYSTEM_INCLUDE_PATHS:-}" \
  "$SHADOW_BIN" "$SRC" >"$SHADOW_OUT" 2>&1

if grep -Fq "Error:" "$BASE_OUT"; then
  echo "baseline language-profile fixture failed" >&2
  cat "$BASE_OUT" >&2
  exit 1
fi
if grep -Fq "Error:" "$SHADOW_OUT"; then
  echo "shadow language-profile fixture failed" >&2
  cat "$SHADOW_OUT" >&2
  exit 1
fi

assert_local_shadow_value() {
  local file="$1"
  local mode="$2"
  if grep -Fq "store i32 99, ptr %local" "$file"; then
    echo "$mode did not preserve local include precedence (resolved to 99)" >&2
    cat "$file" >&2
    exit 1
  fi
  if ! grep -Fq "store i32 7, ptr %local" "$file"; then
    echo "$mode did not preserve local include precedence (missing local=7 assignment)" >&2
    cat "$file" >&2
    exit 1
  fi
}

assert_local_shadow_value "$BASE_OUT" "baseline"
assert_local_shadow_value "$SHADOW_OUT" "shadow"

echo "shim language profile mixed include fixture passed."
