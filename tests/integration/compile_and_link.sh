#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./compiler}"
OUT="${2:-/tmp/mycc-integration-a.out}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_DIR=$(mktemp -d)
trap 'rm -rf "$TMP_DIR" "$OUT"' EXIT

cat > "$TMP_DIR/main.c" <<'EOF'
int helper(void);
int main(void) { return helper(); }
EOF

cat > "$TMP_DIR/helper.c" <<'EOF'
int helper(void) { return 0; }
EOF

"$BIN" "$TMP_DIR/main.c" "$TMP_DIR/helper.c" -o "$OUT"

if [ ! -x "$OUT" ]; then
  echo "Expected executable missing: $OUT" >&2
  exit 1
fi

"$OUT"

echo "compile_and_link integration test passed: $OUT"
