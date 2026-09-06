#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BIN="${1:-./fisics}"
cd "$ROOT"

tmpdir="$(mktemp -d "${TMPDIR:-/tmp}/fisics-units-metadata.XXXXXX")"
trap 'rm -rf "$tmpdir"' EXIT
source_path="tests/fixtures/semantic_metadata_sm7/physical_values.c"

FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units,semantic-metadata \
  --emit-units-metadata-json "$tmpdir/one.json" \
  -c "$source_path" -o "$tmpdir/one.o" >/dev/null
FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units,semantic-metadata \
  --emit-units-metadata-json "$tmpdir/two.json" \
  -c "$source_path" -o "$tmpdir/two.o" >/dev/null
FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units,semantic-metadata \
  -c "$source_path" -o "$tmpdir/no-emission.o" >/dev/null
cmp "$tmpdir/one.json" "$tmpdir/two.json"
cmp "$tmpdir/one.o" "$tmpdir/two.o"
cmp "$tmpdir/one.o" "$tmpdir/no-emission.o"

python3 - "$tmpdir/one.json" "$source_path" <<'PY'
import json, pathlib, sys
artifact = pathlib.Path(sys.argv[1])
source = pathlib.Path(sys.argv[2]).resolve()
raw = artifact.read_bytes()
if not raw.endswith(b"\n") or raw.endswith(b"\n\n"):
    raise SystemExit("units artifact must have exactly one final newline")
data = json.loads(raw)
expected = ["schema", "schema_version", "producer", "analysis_contract", "source",
            "dimension_system", "unit_registry", "overlay_enabled", "complete", "attachments"]
if list(data) != expected or data["schema"] != "fisiCs.units_metadata" or data["schema_version"] != 1:
    raise SystemExit("unexpected units metadata root contract")
if data["source"]["path"] != str(source) or not data["complete"] or not data["overlay_enabled"]:
    raise SystemExit("unexpected units metadata provenance")
if len(data["unit_registry"]["fingerprint_fnv1a64"]) != 16:
    raise SystemExit("invalid registry fingerprint")
attachments = {item["symbol_name"]: item for item in data["attachments"]}
expected_names = {"cabin_temperature", "cabin_temperature_delta", "cabin_velocity",
                  "stored_energy", "shaft_torque", "fallback_distance", "internal_duration"}
if set(attachments) != expected_names:
    raise SystemExit("unexpected units attachment set")
if attachments["cabin_velocity"]["unit"]["name"] != "foot_per_second":
    raise SystemExit("canonical velocity unit was not retained")
if attachments["shaft_torque"]["unit"] is not None:
    raise SystemExit("dimension-only evidence unexpectedly gained a unit")
if attachments["internal_duration"]["unit"]["name"] != "second":
    raise SystemExit("local units evidence was not retained")
ids = [item["binding_id"] for item in data["attachments"]]
if len(ids) != len(set(ids)):
    raise SystemExit("binding IDs are not unique")
for name, item in attachments.items():
    if name == "internal_duration":
        if item["symbol_stable_id"] is not None:
            raise SystemExit("local attachment unexpectedly gained exported identity")
    elif item["symbol_stable_id"] is None:
        raise SystemExit(f"exported attachment lacks stable identity: {name}")
PY

printf 'preserve-units\n' >"$tmpdir/preserved.json"
if FISICS_TEST_FAIL_UNITS_METADATA_WRITE=1 FISICS_MAX_PROCS=0 "$BIN" \
     --overlay=physics-units,semantic-metadata --emit-units-metadata-json "$tmpdir/preserved.json" \
     -c "$source_path" -o "$tmpdir/fail.o" >"$tmpdir/fail.stdout" 2>"$tmpdir/fail.stderr"; then
  echo "ERROR: injected units artifact write failure unexpectedly succeeded" >&2
  exit 1
fi
grep -q 'failed to write units metadata JSON' "$tmpdir/fail.stderr"
grep -qx 'preserve-units' "$tmpdir/preserved.json"
if find "$tmpdir" -name 'preserved.json.tmp.*' -print -quit | grep -q .; then
  echo "ERROR: units artifact temporary file leaked" >&2
  exit 1
fi

expect_cli_failure() {
  local label="$1" needle="$2"
  shift 2
  if FISICS_MAX_PROCS=0 "$BIN" "$@" >"$tmpdir/$label.stdout" 2>"$tmpdir/$label.stderr"; then
    echo "ERROR: $label unexpectedly succeeded" >&2
    exit 1
  fi
  grep -q -- "$needle" "$tmpdir/$label.stderr"
}
expect_cli_failure no-overlay 'requires --overlay=physics-units' \
  --emit-units-metadata-json "$tmpdir/no-overlay.json" -c "$source_path" -o "$tmpdir/no-overlay.o"
expect_cli_failure stdout-path "filesystem path; '-' is not supported" \
  --overlay=physics-units --emit-units-metadata-json - -c "$source_path" -o "$tmpdir/stdout.o"
expect_cli_failure missing-path 'requires a path' --overlay=physics-units --emit-units-metadata-json

cp "$source_path" "$tmpdir/source-alias.c"
cp "$tmpdir/source-alias.c" "$tmpdir/source-alias.before.c"
if FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units,semantic-metadata \
     --emit-units-metadata-json "$tmpdir/source-alias.c" \
     -c "$tmpdir/source-alias.c" -o "$tmpdir/source-alias.o" \
     >"$tmpdir/source-alias.stdout" 2>"$tmpdir/source-alias.stderr"; then
  echo "ERROR: source-aliasing units output unexpectedly succeeded" >&2
  exit 1
fi
cmp "$tmpdir/source-alias.before.c" "$tmpdir/source-alias.c"

expect_cli_failure output-collision 'output must be distinct from other outputs' \
  --overlay=physics-units,semantic-metadata \
  --emit-semantic-metadata-json "$tmpdir/collision.json" \
  --emit-units-metadata-json "$tmpdir/collision.json" \
  -c "$source_path" -o "$tmpdir/collision.o"

echo "units metadata JSON integration passed"
