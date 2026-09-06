# External Deployment And Observation Receipts

Status: schema v1 validator and hostile contract corpus implemented

This contract keeps firmware candidates, deployment operations, raw runtime
captures, operator observations, and compiler static graphs as separate facts.
It is an intake and validation surface. It does not upload firmware, open a
serial port, operate CAN, or turn a static graph into hardware proof.

## Why The Receipts Are Separate

A candidate manifest answers, "Which source and artifact bytes were built?"
A deployment receipt answers, "Was that artifact flashed to this exact USB
identity under recorded authority?" An observation receipt answers, "Which raw
capture bytes were produced after which deployments, and which exact lines
support the summary?"

The chain is intentionally one-way:

```text
source/protocol -> candidate manifest -> deployment receipt
                                      -> raw capture -> observation receipt
static graphs ------------------------------------------------> comparison
```

No later receipt rewrites an earlier one. A new deployment creates a new
deployment receipt. A new capture creates a new observation receipt. Static
graph hashes may be compared with the chain, but they never inherit deployment
or runtime authority.

## Evidence Classes

Schema v1 accepts two evidence classes:

- `retained_external`: real evidence authored and retained by the program that
  owns the target, uploader, device, and capture;
- `contract_fixture`: synthetic validator input that must keep deployment,
  capture, and runtime claims at `fixture_only`.

The checked-in fixture is deliberately non-deployable. Passing its validator
test proves contract mechanics only.

## Deployment Receipt V1

Schema identity: `fisiCs.external_deployment_receipt`, version `1`.

The receipt binds:

- an immutable candidate manifest and source-set digest;
- one selected firmware artifact, size, and digest;
- the protocol digest present in the candidate's source inputs;
- board profile and uppercase six-byte USB identity;
- uploader identity, start/end time, identity check, result, and flash
  verification;
- the authority kind and durable authority reference; and
- an explicit claim boundary.

A passing `retained_external` deployment may claim only `deployment:
established`. Runtime observation and physical acceptance must remain
`not_assessed`. A passing fixture must claim `deployment: fixture_only`.

Candidate manifests remain compile-time records. They must not be edited after
an upload to add deployment state; the deployment receipt references their
unchanged SHA-256 instead.

## Observation Receipt V1

Schema identity: `fisiCs.external_observation_receipt`, version `1`.

The receipt binds:

- one or more deployment receipts by role, receipt ID, path, and SHA-256;
- one retained UTF-8 serial or CAN text capture by path, size, SHA-256, and
  capture interval;
- unique typed assertions whose exact line fragment must occur in the raw
  capture;
- a separately labeled operator observation; and
- an explicit runtime, physical, and calibration claim boundary.

For `retained_external` evidence, the strongest digital claim is
`raw_capture_bound`. That means the receipt and exact assertion text are bound
to retained bytes; it does not establish calibration or a human-observed
physical response. Operator evidence is labeled `documented_external`, never
inferred from serial text. Calibration is intentionally unsupported in v1.

## Validate A Chain

Arrange the evidence under one retained root and use normalized relative paths
inside receipts:

```text
evidence-root/
  candidate/manifest.json
  candidate/firmware.bin
  candidate/protocol.yaml
  receipts/deployment-node-a.json
  receipts/deployment-node-b.json
  captures/serial.txt
  receipts/observation.json
```

Then run:

```bash
python3 scripts/validate_external_evidence.py \
  --evidence-root /absolute/path/to/evidence-root \
  --observation receipts/observation.json \
  --output build/external-evidence-receipts/chain.receipt.json
```

The output contains only relative paths and content identities, so identical
evidence copied to another filesystem root produces byte-identical output.
Output is written atomically after all checks pass. Validation failures leave
an existing output untouched.

Run the permanent positive and hostile corpus with:

```bash
make integration-external-evidence-receipts
```

The corpus rejects manifest, artifact, protocol, deployment-receipt, and raw
capture drift; path traversal; receipt-ID mismatch; assertions absent from raw
bytes; a capture beginning before its referenced deployments completed; and
claim promotion from fixture or deployment-only evidence.

## Retention Rule

Store the raw capture beside the summary and receipt for every observation
that may later be used as runtime evidence. A Markdown report without its raw
capture remains documentary summary. It can explain history, but it cannot be
upgraded later into `raw_capture_bound` evidence.

Protocol paths such as `protocol_v1.yaml` may be moving aliases. Each candidate
and deployment receipt must bind the exact protocol digest it used. Preserve an
immutable protocol snapshot with the evidence bundle or ensure the referenced
candidate retains those exact bytes. A newer current protocol does not
invalidate historical evidence, but it prevents regeneration from being called
an exact replay unless the old protocol bytes are available.

## Hardware Boundary

No bench run is needed to validate the schema or its fixture. A physical run is
justified only when a program owner wants a new `retained_external` chain and a
specific required input is absent—for example, the exact selected candidate's
deployment receipt or the raw capture following that deployment. The validator
reports those gaps; it never fills them by inference.
