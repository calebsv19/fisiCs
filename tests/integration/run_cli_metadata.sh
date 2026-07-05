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
if grep -Eq '(^|[[:space:]])hello\.c([[:space:]]|$)|main\.c util\.c' "$tmpdir/help.out"; then
  echo "ERROR: --help should not advertise placeholder source filenames" >&2
  exit 1
fi

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

FISICS_MAX_PROCS=0 "$BIN" -c examples/hello_world.c -o "$tmpdir/help_hello_world.o" > "$tmpdir/help_compile.out" 2> "$tmpdir/help_compile.err"
test -s "$tmpdir/help_hello_world.o"

FISICS_MAX_PROCS=0 "$BIN" compilation/multi_main.c compilation/multi_helper.c -o "$tmpdir/help_multi_bin" > "$tmpdir/help_multi_compile.out" 2> "$tmpdir/help_multi_compile.err"
"$tmpdir/help_multi_bin" > "$tmpdir/help_multi_run.out"
grep -q 'multi-file smoke: helper=7 total=12' "$tmpdir/help_multi_run.out"

FISICS_MAX_PROCS=0 "$BIN" examples/hello_world.c -o "$tmpdir/help_hello_world" > "$tmpdir/help_hello_link.out" 2> "$tmpdir/help_hello_link.err"
"$tmpdir/help_hello_world" > "$tmpdir/help_hello_run.out"
grep -q 'Hello from fisiCs.' "$tmpdir/help_hello_run.out"

FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units --dump-sema -c examples/physics_units/ballistics_valid.c -o "$tmpdir/help_ballistics_valid.o" > "$tmpdir/help_ballistics.out" 2> "$tmpdir/help_ballistics.err"
grep -q 'Semantic analysis: no issues found.' "$tmpdir/help_ballistics.out"
test -s "$tmpdir/help_ballistics_valid.o"

echo "CLI metadata contract passed"
