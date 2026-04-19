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
    tests/semantic/bench_semantic_expr_dense.c \
    tests/semantic/bench_semantic_consteval_dense.c \
    tests/semantic/bench_semantic_decl_layout_dense.c \
    tests/semantic/bench_semantic_flow_dense.c
fi

tmp_dir="$(mktemp -d)"
trap 'rm -rf "$tmp_dir"' EXIT

echo "semantic timing audit"
echo "  bin: $BIN"
echo "  tmp: $tmp_dir"

for src in "$@"; do
  stem="$(basename "$src" .c)"
  csv="$tmp_dir/${stem}.csv"

  echo
  echo "[fisics profiled] $src"
  /usr/bin/time -p env DISABLE_CODEGEN=1 FISICS_PROFILE=1 FISICS_PROFILE_STREAM=1 FISICS_PROFILE_PATH="$csv" "$BIN" "$src" >/dev/null
  awk -F, 'NR>1 { if ($1 ~ /^semantic_count_/) sum[$1]+=$2; else sum[$1]+=$3; } END {for (k in sum) if (k ~ /^(semantic|semantic_run_analysis|semantic_seed_builtins|semantic_analyze_dispatch|semantic_analyze_declaration|semantic_analyze_statement|semantic_control_flow|semantic_const_eval_integer|semantic_type_info_from_parsed_type|semantic_count_run_analysis|semantic_count_seed_builtins|semantic_count_analyze_node|semantic_count_analyze_declaration|semantic_count_analyze_statement|semantic_count_analyze_expression|semantic_count_control_flow|semantic_count_const_eval_integer|semantic_count_type_info_from_parsed_type|semantic_count_create_scope|semantic_count_scope_add|semantic_count_scope_resolve_chain|semantic_count_scope_resolve_chain_hit|semantic_count_symbol_lookup|semantic_count_symbol_lookup_hit|semantic_count_symbol_insert)$/) { if (k ~ /^semantic_count_/) printf "  %s,%.0f\n", k, sum[k]; else printf "  %s,%.3f\n", k, sum[k]; }}' "$csv" | sort

  echo "[fisics non-profiled] $src"
  /usr/bin/time -p env DISABLE_CODEGEN=1 "$BIN" "$src" >/dev/null

  echo "[clang syntax-only] $src"
  /usr/bin/time -p clang -std=c99 -fsyntax-only "$src" >/dev/null
done
