#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname "$0")" && pwd)"
FISICS_ROOT="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"

DEFAULT_LAUNCHER="${HOME}/Desktop/fisics_program_binaries/mem_console/clang/eCho.app/Contents/MacOS/mem-console-launcher"
LAUNCHER="${MEM_CONSOLE_CLANG_LAUNCHER:-$DEFAULT_LAUNCHER}"

LOG_ROOT="${HOME}/Desktop/fisics_program_binaries/logs/mem_console/clang"
RUNTIME_ROOT="${HOME}/Desktop/fisics_program_binaries/runtime/mem_console"
DEFAULT_RUNTIME_DIR="${RUNTIME_ROOT}/clang"
RUNTIME_DIR="${MEM_CONSOLE_RUNTIME_DIR:-$DEFAULT_RUNTIME_DIR}"
STAMP="$(date +%Y%m%dT%H%M%S)"
OUT_DIR="${LOG_ROOT}/${STAMP}"
LAUNCHER_LOG="${HOME}/Library/Logs/MemConsole/launcher.log"
RAW_LOG="${OUT_DIR}/launcher.full.log"
FILTERED_LOG="${OUT_DIR}/launcher.filtered.log"
PREV_LOG="${OUT_DIR}/launcher.preexisting.log"

PATTERN='\[ir1\]|\[rs1\]|body edit|search filter|FULL|FNL|focus'

mkdir -p "$OUT_DIR"
mkdir -p "$(dirname "$LAUNCHER_LOG")"
mkdir -p "$RUNTIME_DIR"

if [[ ! -x "$LAUNCHER" ]]; then
    echo "Missing launcher: $LAUNCHER" >&2
    echo "Override with MEM_CONSOLE_CLANG_LAUNCHER=/abs/path/to/mem-console-launcher" >&2
    exit 2
fi

if [[ -f "$LAUNCHER_LOG" ]]; then
    cp "$LAUNCHER_LOG" "$PREV_LOG" 2>/dev/null || true
fi
: > "$LAUNCHER_LOG"

echo "=== mem_console clang capture ==="
echo "launcher: $LAUNCHER"
echo "output dir: $OUT_DIR"
echo "Close the app when done reproducing baseline behavior."

MEM_CONSOLE_IR1_DIAG=1 \
MEM_CONSOLE_RS1_DIAG=1 \
MEM_CONSOLE_RUNTIME_DIR="$RUNTIME_DIR" \
"$LAUNCHER" "$@"

cp "$LAUNCHER_LOG" "$RAW_LOG" 2>/dev/null || true

if command -v rg >/dev/null 2>&1; then
    rg -n "$PATTERN" "$RAW_LOG" > "$FILTERED_LOG" || true
else
    grep -En "$PATTERN" "$RAW_LOG" > "$FILTERED_LOG" || true
fi

echo
echo "Capture complete."
echo "full log:     $RAW_LOG"
echo "filtered log: $FILTERED_LOG"
if [[ -f "$PREV_LOG" ]]; then
    echo "previous log: $PREV_LOG"
fi
