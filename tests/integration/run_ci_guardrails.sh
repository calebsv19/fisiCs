#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

echo "[guardrail] frontend archive refresh"
make frontend-rebuild-ide

echo "[guardrail] frontend contract bucket"
make frontend-contract-test

echo "[guardrail] contract capability lane presence"
if [ ! -f "tests/unit/frontend_api_contract_capabilities.c" ]; then
  echo "guardrail failure: required contract capability test source missing (tests/unit/frontend_api_contract_capabilities.c)" >&2
  exit 1
fi
if ! grep -q "frontend_api_contract_capabilities" "tests/unit/frontend_api_contract_bucket.md"; then
  echo "guardrail failure: contract bucket matrix missing frontend_api_contract_capabilities entry" >&2
  exit 1
fi

echo "[guardrail] frontend api suite"
make frontend-api-test

if [ ! -f "libfisics_frontend.a" ]; then
  echo "guardrail failure: libfisics_frontend.a missing after frontend-rebuild-ide" >&2
  exit 1
fi

echo "[guardrail] frontend ABI symbol presence"
nm_symbols="$(nm -g "libfisics_frontend.a" || true)"
if ! grep -Eq '[[:space:]][Tt][[:space:]]_?fisics_analyze_buffer$' <<<"$nm_symbols"; then
  echo "guardrail failure: fisics_analyze_buffer symbol missing from libfisics_frontend.a" >&2
  exit 1
fi
if ! grep -Eq '[[:space:]][Tt][[:space:]]_?fisics_free_analysis_result$' <<<"$nm_symbols"; then
  echo "guardrail failure: fisics_free_analysis_result symbol missing from libfisics_frontend.a" >&2
  exit 1
fi

echo "[guardrail] release package safety"
make release-contract
make release-archive
make release-verify

if ! ls build/release/*.manifest.txt >/dev/null 2>&1; then
  echo "guardrail failure: release manifest missing under build/release/" >&2
  exit 1
fi
if ! ls build/release/*.zip >/dev/null 2>&1; then
  echo "guardrail failure: release zip artifact missing under build/release/" >&2
  exit 1
fi
if ! ls build/release/*.tar.gz >/dev/null 2>&1; then
  echo "guardrail failure: release tar.gz artifact missing under build/release/" >&2
  exit 1
fi
if ! ls build/release/*.sha256 >/dev/null 2>&1; then
  echo "guardrail failure: release sha256 file missing under build/release/" >&2
  exit 1
fi

echo "ci guardrails: PASS"
