#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
SRC="${2:-test_files/missing_expr_semicolon.c}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

tmpdir="$(mktemp -d)"
cleanup() {
  rm -rf "$tmpdir"
}
trap cleanup EXIT

pack_path="$tmpdir/diag.pack"
json_path="$tmpdir/diag.json"
inspector_c="$tmpdir/inspect_pack.c"
inspector_bin="$tmpdir/inspect_pack"

"$BIN" --emit-diags-pack "$pack_path" --emit-diags-json "$json_path" "$SRC" >/dev/null 2>&1

if [ ! -s "$pack_path" ]; then
  echo "expected diagnostics pack to be created: $pack_path" >&2
  exit 1
fi

if [ ! -s "$json_path" ]; then
  echo "expected diagnostics json to be created: $json_path" >&2
  exit 1
fi

cat >"$inspector_c" <<'EOF'
#include "core_pack.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct FisicsDiagPackHeaderV1 {
    uint32_t schema_version;
    uint32_t diag_count;
    uint32_t error_count;
    uint32_t warning_count;
    uint32_t note_count;
} FisicsDiagPackHeaderV1;

typedef struct FisicsDiagPackRowV1 {
    uint32_t line;
    uint32_t column;
    uint32_t length;
    uint32_t kind;
    int64_t code;
    uint32_t flags;
} FisicsDiagPackRowV1;

static bool read_chunk(CorePackReader* reader, const char type[4], CorePackChunkInfo* out) {
    CoreResult r = core_pack_reader_find_chunk(reader, type, 0, out);
    return r.code == CORE_OK;
}

int main(int argc, char** argv) {
    CorePackReader reader;
    CorePackChunkInfo header_chunk;
    CorePackChunkInfo meta_chunk;
    CorePackChunkInfo row_chunk;
    FisicsDiagPackHeaderV1 header = {0};
    const char* pack_path;
    CoreResult r;

    if (argc < 2) {
      fprintf(stderr, "usage: %s <diag.pack>\n", argv[0]);
      return 1;
    }
    pack_path = argv[1];

    r = core_pack_reader_open(pack_path, &reader);
    if (r.code != CORE_OK) {
      fprintf(stderr, "failed to open pack: %s\n", r.message);
      return 1;
    }

    if (!read_chunk(&reader, "FDHD", &header_chunk) ||
        !read_chunk(&reader, "FDMJ", &meta_chunk) ||
        !read_chunk(&reader, "FDRW", &row_chunk)) {
      fprintf(stderr, "missing required diagnostics chunks\n");
      (void)core_pack_reader_close(&reader);
      return 1;
    }

    if (header_chunk.size != sizeof(header)) {
      fprintf(stderr, "invalid FDHD size: %llu\n", (unsigned long long)header_chunk.size);
      (void)core_pack_reader_close(&reader);
      return 1;
    }

    r = core_pack_reader_read_chunk_data(&reader, &header_chunk, &header, sizeof(header));
    if (r.code != CORE_OK) {
      fprintf(stderr, "failed to read FDHD: %s\n", r.message);
      (void)core_pack_reader_close(&reader);
      return 1;
    }

    if (header.schema_version != 1u) {
      fprintf(stderr, "unexpected schema_version=%u\n", header.schema_version);
      (void)core_pack_reader_close(&reader);
      return 1;
    }
    if (header.diag_count == 0u || header.error_count == 0u) {
      fprintf(stderr, "expected at least one diagnostic/error (diag=%u err=%u)\n",
              header.diag_count, header.error_count);
      (void)core_pack_reader_close(&reader);
      return 1;
    }
    if (row_chunk.size % sizeof(FisicsDiagPackRowV1) != 0u) {
      fprintf(stderr, "FDRW chunk size is misaligned: %llu\n", (unsigned long long)row_chunk.size);
      (void)core_pack_reader_close(&reader);
      return 1;
    }
    if ((row_chunk.size / sizeof(FisicsDiagPackRowV1)) < header.diag_count) {
      fprintf(stderr, "FDRW row count smaller than header diag_count\n");
      (void)core_pack_reader_close(&reader);
      return 1;
    }
    if (meta_chunk.size == 0u) {
      fprintf(stderr, "FDMJ metadata chunk is empty\n");
      (void)core_pack_reader_close(&reader);
      return 1;
    }

    (void)core_pack_reader_close(&reader);
    return 0;
}
EOF

cc -std=c11 -Wall -Wextra -Wpedantic \
  -I../shared/core/core_pack/include -I../shared/core/core_base/include \
  "$inspector_c" ../shared/core/core_pack/src/core_pack.c ../shared/core/core_base/src/core_base.c \
  -o "$inspector_bin"

"$inspector_bin" "$pack_path"
echo "emit_diags_pack integration test passed."
