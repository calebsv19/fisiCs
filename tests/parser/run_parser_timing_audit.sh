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
    tests/parser/bench_parser_cast_group_dense.c \
    tests/parser/bench_parser_lookahead_dense.c \
    tests/parser/bench_parser_decl_routing_dense.c \
    tests/parser/bench_parser_type_forms.c
fi

tmp_dir="$(mktemp -d)"
trap 'rm -rf "$tmp_dir"' EXIT

echo "parser timing audit"
echo "  bin: $BIN"
echo "  tmp: $tmp_dir"

for src in "$@"; do
  stem="$(basename "$src" .c)"
  csv="$tmp_dir/${stem}.csv"

  echo
  echo "[fisics profiled] $src"
  /usr/bin/time -p env DISABLE_CODEGEN=1 FISICS_PROFILE=1 FISICS_PROFILE_STREAM=1 FISICS_PROFILE_PATH="$csv" "$BIN" "$src" >/dev/null
  awk -F, 'NR>1 { if ($1 ~ /^parser_count_/) sum[$1]+=$2; else sum[$1]+=$3; } END {for (k in sum) if (k ~ /^(parse|semantic|parser_lookahead_type_decl|parser_lookahead_cast_type|parser_lookahead_compound_literal|parser_parse_type_ctx|parser_pratt_lparen_type_probe|parser_type_decl_site_program_toplevel|parser_type_decl_site_statement|parser_type_decl_site_for_init|parser_type_decl_site_sync_recovery|parser_type_decl_site_builtin_offsetof|parser_type_decl_site_builtin_va_arg|parser_count_clone_parser|parser_count_parse_type_ctx|parser_count_parse_type_ctx_strict|parser_count_parse_type_ctx_declaration|parser_count_type_decl_fast_token|parser_count_type_decl_fast_identifier_negative|parser_count_type_decl_typedef_visible|parser_count_type_decl_clone_probe|parser_count_type_decl_clone_probe_identifier|parser_count_type_decl_clone_probe_other|parser_count_type_decl_clone_probe_positive|parser_count_type_decl_clone_probe_negative|parser_count_type_decl_clone_probe_identifier_negative|parser_count_cast_type_probe|parser_count_compound_literal_probe|parser_count_pratt_lparen_type_probe|parser_count_type_decl_site_program_toplevel|parser_count_type_decl_site_program_toplevel_positive|parser_count_type_decl_site_program_toplevel_negative|parser_count_type_decl_site_statement|parser_count_type_decl_site_statement_positive|parser_count_type_decl_site_statement_negative|parser_count_type_decl_site_for_init|parser_count_type_decl_site_for_init_positive|parser_count_type_decl_site_for_init_negative|parser_count_type_decl_site_sync_recovery|parser_count_type_decl_site_sync_recovery_positive|parser_count_type_decl_site_sync_recovery_negative|parser_count_type_decl_site_builtin_offsetof|parser_count_type_decl_site_builtin_offsetof_positive|parser_count_type_decl_site_builtin_offsetof_negative|parser_count_type_decl_site_builtin_va_arg|parser_count_type_decl_site_builtin_va_arg_positive|parser_count_type_decl_site_builtin_va_arg_negative)$/) { if (k ~ /^parser_count_/) printf "  %s,%.0f\n", k, sum[k]; else printf "  %s,%.3f\n", k, sum[k]; }}' "$csv" | sort

  echo "[fisics non-profiled] $src"
  /usr/bin/time -p env DISABLE_CODEGEN=1 "$BIN" "$src" >/dev/null

  echo "[clang syntax-only] $src"
  /usr/bin/time -p clang -std=c99 -fsyntax-only "$src" >/dev/null
done
