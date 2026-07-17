#!/usr/bin/env python3
"""Validate and materialize the frozen 2026-07 promotion-closure roster."""

from __future__ import annotations

import json
from collections import Counter
from pathlib import Path

from inventory.promotion_closure_owners import (
    PROMOTION_CLOSURE_OWNERS,
    PROMOTION_CLOSURE_RETAINED,
)


FINAL_ROOT = Path(__file__).resolve().parents[1]
AUDIT_PATH = FINAL_ROOT / "probes/reports/promotion_audit.json"
OUTPUT_PATH = FINAL_ROOT / "probes/reports/promotion_closure_20260714.json"


def main() -> int:
    audit = json.loads(AUDIT_PATH.read_text(encoding="utf-8"))
    records_by_id = {record["probe_id"]: record for record in audit["records"]}
    promoted_ids = set(PROMOTION_CLOSURE_OWNERS)
    retained_ids = set(PROMOTION_CLOSURE_RETAINED)
    frozen_ids = promoted_ids | retained_ids

    errors: list[str] = []
    if promoted_ids & retained_ids:
        errors.append("promoted and retained rosters overlap")
    if len(promoted_ids) != 183:
        errors.append(f"expected 183 promoted records, found {len(promoted_ids)}")
    if len(retained_ids) != 15:
        errors.append(f"expected 15 retained records, found {len(retained_ids)}")
    if len(frozen_ids) != 198:
        errors.append(f"expected 198 frozen records, found {len(frozen_ids)}")

    missing_records = sorted(frozen_ids - records_by_id.keys())
    if missing_records:
        errors.append("audit missing frozen ids: " + ", ".join(missing_records))

    dispositions = []
    for probe_id in sorted(frozen_ids):
        record = records_by_id.get(probe_id)
        if record is None:
            continue
        if probe_id in promoted_ids:
            expected_owner = PROMOTION_CLOSURE_OWNERS[probe_id]
            if record.get("classification") != "promoted":
                errors.append(f"{probe_id}: expected promoted classification")
            if record.get("promoted_test_id") != expected_owner:
                errors.append(
                    f"{probe_id}: owner {record.get('promoted_test_id')!r} "
                    f"does not match {expected_owner!r}"
                )
            disposition = "promoted"
            rationale = "stable final ownership recorded"
        else:
            expected_owner = None
            if record.get("classification") != "probe-only":
                errors.append(f"{probe_id}: expected probe-only classification")
            disposition = "retained-probe-only"
            rationale = PROMOTION_CLOSURE_RETAINED[probe_id]
        dispositions.append(
            {
                "probe_id": probe_id,
                "bucket": record["bucket"],
                "family": record["family"],
                "disposition": disposition,
                "stable_owner": expected_owner,
                "rationale": rationale,
            }
        )

    expected_buckets = {
        "04": 5, "05": 8, "06": 36, "08": 42, "09": 21,
        "10": 25, "11": 46, "14": 1, "15": 14,
    }
    actual_buckets = dict(sorted(Counter(r["bucket"] for r in dispositions).items()))
    if actual_buckets != expected_buckets:
        errors.append(f"bucket roster mismatch: {actual_buckets!r}")

    report = {
        "schema_version": 1,
        "frozen_baseline": {
            "date": "2026-07-14",
            "total_records": 198,
            "bucket_counts": expected_buckets,
        },
        "closure": {
            "promoted": len(promoted_ids),
            "retained_probe_only": len(retained_ids),
            "overlap": len(promoted_ids & retained_ids),
            "audit_summary": audit["summary"],
            "integrity": audit["integrity"],
            "errors": errors,
        },
        "records": dispositions,
    }
    OUTPUT_PATH.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(
        f"promotion-closure: promoted={len(promoted_ids)} "
        f"retained={len(retained_ids)} errors={len(errors)}"
    )
    print(f"json={OUTPUT_PATH}")
    return 1 if errors else 0


if __name__ == "__main__":
    raise SystemExit(main())
