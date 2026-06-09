#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

ROOT="$(mktemp -d "${TMPDIR:-/tmp}/fisics_manifest_execute.XXXXXX")"
trap 'rm -rf "$ROOT"' EXIT

mkdir -p "$ROOT/src" "$ROOT/include"
cat > "$ROOT/include/value.h" <<'EOF'
#define FISICS_MANIFEST_EXPECTED 17
EOF
cat > "$ROOT/src/main.c" <<'EOF'
#include "value.h"
int lib_value(void);
int main(void) { return lib_value() == FISICS_MANIFEST_EXPECTED ? 0 : 1; }
EOF
cat > "$ROOT/src/lib.c" <<'EOF'
#include "value.h"
int lib_value(void) { return FISICS_MANIFEST_EXPECTED; }
EOF

cat > "$ROOT/project.json" <<'EOF'
{
  "schema": "fisiCs.project",
  "version": 0,
  "name": "manifest_exec",
  "root": ".",
  "build_dir": "build/fisics",
  "defaults": {
    "standard": "c99",
    "include_dirs": ["include"],
    "defines": ["FISICS_MANIFEST_EXEC=1"],
    "overlays": []
  },
  "translation_units": [
    {"source": "src/main.c", "object": "build/fisics/main.o"},
    {"source": "src/lib.c"}
  ],
  "link": {
    "output": "build/fisics/manifest_exec",
    "libraries": [],
    "library_dirs": [],
    "args": []
  }
}
EOF

"$BIN" --build-manifest "$ROOT/project.json" >/dev/null

if [ ! -f "$ROOT/build/fisics/main.o" ]; then
  echo "manifest execution did not write explicit object" >&2
  exit 1
fi

if [ ! -f "$ROOT/build/fisics/lib.o" ]; then
  echo "manifest execution did not write derived object" >&2
  exit 1
fi

if [ ! -x "$ROOT/build/fisics/manifest_exec" ]; then
  echo "manifest execution did not write linked executable" >&2
  exit 1
fi

"$ROOT/build/fisics/manifest_exec"

cat > "$ROOT/object_only.json" <<'EOF'
{
  "schema": "fisiCs.project",
  "version": 0,
  "name": "manifest_object_only",
  "root": ".",
  "build_dir": "build/fisics_obj_only",
  "defaults": {
    "standard": "c99",
    "include_dirs": ["include"],
    "defines": [],
    "overlays": []
  },
  "translation_units": [
    {"source": "src/lib.c"}
  ]
}
EOF

"$BIN" --build-manifest "$ROOT/object_only.json" >/dev/null

if [ ! -f "$ROOT/build/fisics_obj_only/lib.o" ]; then
  echo "manifest object-only execution did not write derived object" >&2
  exit 1
fi

echo "build manifest execution test passed."
