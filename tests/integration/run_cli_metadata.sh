#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BIN="${1:-./fisics}"

cd "$ROOT"

if [ ! -x "$BIN" ]; then
  echo "ERROR: compiler binary is not executable: $BIN" >&2
  exit 2
fi

tmpdir="$(mktemp -d "${TMPDIR:-/tmp}/fisics-cli-metadata.XXXXXX")"
trap 'rm -rf "$tmpdir"' EXIT

expected_version="$(tr -d '[:space:]' < VERSION)"

FISICS_MAX_PROCS=0 "$BIN" --version > "$tmpdir/version.out"
grep -Eq '^fisiCs [0-9]+\.[0-9]+\.[0-9]+' "$tmpdir/version.out"
grep -q "fisiCs $expected_version" "$tmpdir/version.out"
if grep -q 'include/test.txt' "$tmpdir/version.out"; then
  echo "ERROR: --version should not reference include/test.txt" >&2
  exit 1
fi

FISICS_MAX_PROCS=0 "$BIN" --help > "$tmpdir/help.out"
grep -q '^Usage:' "$tmpdir/help.out"
grep -q -- '--version' "$tmpdir/help.out"
grep -q -- '--build-manifest' "$tmpdir/help.out"
grep -q -- '--overlay=<name>' "$tmpdir/help.out"

if FISICS_MAX_PROCS=0 "$BIN" > "$tmpdir/noarg.out" 2> "$tmpdir/noarg.err"; then
  echo "ERROR: no-argument invocation should fail with usage" >&2
  exit 1
fi
grep -q 'Error: no input files.' "$tmpdir/noarg.err"
grep -q '^Usage:' "$tmpdir/noarg.err"
if grep -q 'include/test.txt' "$tmpdir/noarg.err"; then
  echo "ERROR: no-argument invocation should not load include/test.txt" >&2
  exit 1
fi

if FISICS_MAX_PROCS=0 "$BIN" --not-a-real-flag > "$tmpdir/unknown.out" 2> "$tmpdir/unknown.err"; then
  echo "ERROR: unknown option should fail" >&2
  exit 1
fi
grep -q "Error: unknown option '--not-a-real-flag'" "$tmpdir/unknown.err"
grep -Eq "Run '.+ --help' for usage\\." "$tmpdir/unknown.err"
if grep -q 'include/test.txt' "$tmpdir/unknown.err"; then
  echo "ERROR: unknown option should not load include/test.txt" >&2
  exit 1
fi

test -f include/test.txt

cat > "$tmpdir/hello.c" <<'C'
#include <stdio.h>
int main(void) {
    puts("fisiCs cli metadata smoke ok");
    return 0;
}
C

FISICS_MAX_PROCS=0 "$BIN" "$tmpdir/hello.c" -o "$tmpdir/hello" > "$tmpdir/compile.out" 2> "$tmpdir/compile.err"
"$tmpdir/hello" > "$tmpdir/run.out"
grep -q 'fisiCs cli metadata smoke ok' "$tmpdir/run.out"

echo "CLI metadata contract passed"
