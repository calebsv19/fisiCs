# Executable AHT20 graph binding

Status: bounded backend adapter implemented; native Main Edit retained-result
inspection adopted. In-app graph preparation/execution remains unimplemented.

Verified 2026-09-12: the executable gate completed two deterministic profile
executions and 134 bound records across two ports. Nine focused test groups
cover preflight, projection and retained-result tampering. The existing full
CAN protocol/session regression also passed unchanged. These are software
results, not new bench evidence.

## Contract and ownership

`tests/fixtures/esp_aht20_binding_v1/binding.json` selects the exact
`device.can.aht20` graph device, its temperature/humidity ports, and the
`environment.aht20.v1` host profile. Adapter `aht20_rear_left_v1` selects
instance 12, `rear_left`, within the four-sensor environment scenario. It does
not instantiate arbitrary graph devices or infer models from names.

The compiler-owned integration adapter orchestrates graph validation and
delegates execution to ESP_Work's existing `control_data` plan/run interface.
ESP_Work retains ownership of the model, acquisition/runtime behavior, build
provenance and immutable consumer artifacts. No C++ parsing or compiler-core
change is involved; the native app's binding support states are unchanged.

## Preflight and execution

Declaration validation is now owned by `tools/aht20_declaration.py`, which
applies the exact AHT20 witness policy and delegates graph joins to
`tools/attachment_handoff.py`. The runner retains its `validate` compatibility
entry point and normalizes verifier errors to `SimulationError`. Its temporary
handoff bundle is discarded; no new sidecar is required in retained results.
The migrated execution was requalified twice with the same 134 records and
unchanged graph/run pins. This does not migrate native execution-result imports
or upgrade this legacy witness to optional provenance v2.

The runner regenerates the static topology with the current compiler, verifies
the existing graph pins, hashes the actual graph bytes, and checks the selected
device/model, compatible physical-port joins, semantic subjects, quantities,
scalar representation and unit policy. It validates the exact adapter version,
sensor selection and admitted profile digest before executing a model.

The existing graph and run pins remain owned by the CAN simulation contract;
this adapter does not repin them or copy its expected results into a new oracle.
Unsupported assignments fail rather than falling back to a similar model.

The backend executes twice and must produce identical plan and run bytes plus
the expected run digest. The adapter preserves each selected sample's raw
value, quality, validity, sequence, session and receive time. Temperature
conversion is explicit: `raw / 100 + 273.15` kelvin. Humidity is explicitly
`raw / 10000` fraction; its graph contract remains dimension-only, not an
invented concrete compiler unit. A value is usable only when the source sample
is valid and has positive quality, matching the existing projection rule.

## Run and inspect

From the fisiCs checkout, choose a new output directory on every execution:

```bash
make integration-esp-aht20-binding \
  ESP_WORK_ROOT=/absolute/path/to/ESP_Work \
  BINDING_OUT=/absolute/path/to/new-aht20-result

python3 -B tests/integration/run_esp_aht20_binding.py \
  --verify /absolute/path/to/new-aht20-result
```

The Make gate also runs focused rejection/projection tests. The full CAN
regression remains `make integration-esp-can-protocol-session ESP_WORK_ROOT=...`.

`result.json` joins the binding, all three graph digests, profile digest,
plan/run digests and selected device-port samples. The directory retains the
graphs, binding, static receipt, projection contract, raw run and both original
backend executions with their evidence objects and consumer artifacts. Original
embedded source/temp paths remain provenance, not portable lookup instructions.
Read retained artifacts relative to this bundle. Readback checks their hashes,
recomputes the projection without running a model, and requires the v1 result
claims to remain `executions_completed=2`, `evidence=host_simulation`,
`physical_hardware=false`, and `native_adoption=not_implemented`.
It also requires `profile_sha256` to match the pinned value in the retained
binding, and `plan_sha256` to match both top-level plans and both run-local plan
copies. The profile bytes live in the external ESP source tree and are checked
at execution time; detached readback does not independently reconstruct those
source bytes.

Existing output directories are refused. A failed write can leave an incomplete
directory; only successful final verification establishes completion. Choose a
new destination for a retry, preserving the failure evidence. Hashes establish
unsigned integrity, not authenticity or physical acceptance.

## Remaining boundary

Native Main Edit consumer-v2 inspection now exposes the verified device/port/model
join and compares values, quality and freshness; its
`docs/compiler-bound-inspection.md` owns commands and proof. Native display uses
receiver usability, which includes freshness, and retains this adapter's
validity/quality eligibility separately as `source_usable`. The historical
`native_adoption` field in existing result bundles records their production-time
state; it is not a capability negotiation field and must not be rewritten.
In-app graph prepare/run remains a next boundary. This is separate from the older CAN
replay adapter. A second model requires explicit qualification before expanding
the adapter contract. No firmware, hardware, calibration, release or installed
application acceptance is established by this software gate.

## Isolated compiler contract gate

Run `make test-esp-aht20-binding-contract` for fresh compiler-generated topology
and declaration/projection/tamper tests without ESP model execution. The gate
uses temporary output and the selected `BIN`. This proves compiler and adapter
contracts; historical external execution pins and their required source snapshot
remain governed by the CAN closeout page.
