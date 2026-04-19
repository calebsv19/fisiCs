#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
shift || true

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

if [ "$#" -eq 0 ]; then
  set -- \
    tests/codegen/bench_codegen_control_dense.c \
    tests/codegen/bench_codegen_global_init_dense.c \
    tests/codegen/bench_codegen_struct_layout_dense.c \
    tests/codegen/bench_codegen_calls_aggregate_dense.c
fi

tmp_dir="$(mktemp -d)"
trap 'rm -rf "$tmp_dir"' EXIT

echo "codegen timing audit"
echo "  bin: $BIN"
echo "  tmp: $tmp_dir"

for src in "$@"; do
  stem="$(basename "$src" .c)"
  csv="$tmp_dir/${stem}.csv"
  ll="$tmp_dir/${stem}.ll"

  echo
  echo "[fisics profiled] $src"
  /usr/bin/time -p sh -c 'env FISICS_PROFILE=1 FISICS_PROFILE_STREAM=1 FISICS_PROFILE_PATH="$1" "$2" "$3" >/dev/null 2>&1' _ "$csv" "$BIN" "$src"
  awk -F, 'NR>1 { if ($1 ~ /^codegen_count_/) sum[$1]+=$2; else sum[$1]+=$3; } END {for (k in sum) if (k ~ /^(parse|semantic|codegen|codegen_context_create|codegen_generate_root|codegen_predeclare_globals|codegen_program|codegen_function_definition|codegen_variable_declaration|codegen_function_call|codegen_type_from_parsed|codegen_struct_definition|codegen_const_initializer|codegen_store_initializer_expression|codegen_count_function_definition|codegen_count_variable_declaration|codegen_count_function_call|codegen_count_type_from_parsed|codegen_count_struct_definition|codegen_count_const_initializer|codegen_count_store_initializer_expression)$/) { if (k ~ /^codegen_count_/) printf "  %s,%.0f\n", k, sum[k]; else printf "  %s,%.3f\n", k, sum[k]; }}' "$csv" | sort

  echo "[fisics non-profiled] $src"
  /usr/bin/time -p sh -c '"$1" "$2" >/dev/null 2>&1' _ "$BIN" "$src"

  echo "[fisics frontend-only] $src"
  /usr/bin/time -p sh -c 'env DISABLE_CODEGEN=1 "$1" "$2" >/dev/null 2>&1' _ "$BIN" "$src"

  echo "[clang emit-llvm] $src"
  /usr/bin/time -p clang -std=c99 -S -emit-llvm -O0 -o "$ll" "$src" >/dev/null

  echo "[clang syntax-only] $src"
  /usr/bin/time -p clang -std=c99 -fsyntax-only "$src" >/dev/null
done
