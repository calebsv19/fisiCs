import argparse
import json
import sys
from collections import Counter, defaultdict
from pathlib import Path

from inventory.registry import DIAG_JSON_PROBES, DIAG_PROBES, RUNTIME_PROBES

FINAL_ROOT = Path(__file__).resolve().parents[2]
REPO_ROOT = FINAL_ROOT.parents[1]
if str(FINAL_ROOT) not in sys.path:
    sys.path.insert(0, str(FINAL_ROOT))

from run_final import iter_manifest_files, load_json


REPORTS_DIR = FINAL_ROOT / "probes" / "reports"
DEFAULT_JSON_OUT = REPORTS_DIR / "promotion_audit.json"
DEFAULT_MD_OUT = REPORTS_DIR / "promotion_audit_summary.md"

PROBE_ONLY_PREFIX_MARKERS = (
    "strict frontier",
    "current threshold",
    "current semantics",
    "current parser only",
    "semantic only",
    "reduced threshold",
    "reduced",
    "frontier lane",
    "control lane",
    "regression guard",
    "text parity guard",
)

PROBE_ONLY_NAME_MARKERS = (
    "seeded",
    "fuzz",
    "smoke",
    "corpus",
    "pathological",
    "policy",
)


def _display_path(path):
    for root in (FINAL_ROOT, REPO_ROOT):
        try:
            return str(path.relative_to(root))
        except ValueError:
            continue
    return str(path)


def _probe_rows():
    rows = []
    for family, probes in (
        ("runtime", RUNTIME_PROBES),
        ("diagnostic", DIAG_PROBES),
        ("diagnostic-json", DIAG_JSON_PROBES),
    ):
        for probe in probes:
            inputs = [_display_path(path) for path in (probe.inputs or ())]
            rows.append(
                {
                    "family": family,
                    "probe_id": probe.probe_id,
                    "bucket": probe.probe_id.split("__", 1)[0],
                    "source": _display_path(probe.source),
                    "inputs": inputs,
                    "note": probe.note,
                }
            )
    return rows


def _final_rows():
    rows = []
    for manifest_path in iter_manifest_files():
        manifest = load_json(manifest_path)
        for test in manifest.get("tests", []):
            rows.append(
                {
                    "id": test["id"],
                    "manifest": manifest_path.name,
                    "bucket": test.get("bucket", ""),
                    "input": test.get("input", ""),
                    "inputs": list(test.get("inputs") or []),
                    "tags": list(test.get("tags") or []),
                    "status": test.get("status", ""),
                }
            )
    return rows


def _family_id_variants(probe_id, family):
    variants = {probe_id}
    if "__probe_" not in probe_id:
        return variants
    bucket, tail = probe_id.split("__probe_", 1)
    variants.add(probe_id.replace("__probe_", "__", 1))
    if family == "runtime":
        variants.add(f"{bucket}__runtime_{tail}")
        variants.add(f"{bucket}__torture__{tail}")
    elif family == "diagnostic":
        variants.add(f"{bucket}__diag__{tail}")
    else:
        variants.add(f"{bucket}__diagjson__{tail}")
    return variants


def _stem_variants(stem, family):
    variants = set(_family_id_variants(stem, family))
    for base in list(variants):
        for suffix in (
            "_runtime",
            "_reject",
            "_ok",
            "_pass",
            "_main",
            "_lib",
            "_liba",
            "_libb",
            "_aux",
            "_shared",
        ):
            if base.endswith(suffix):
                variants.add(base[: -len(suffix)])
    return variants


def _build_final_indexes(final_rows):
    by_id = {}
    by_path = defaultdict(list)
    by_stem = defaultdict(list)
    for row in final_rows:
        by_id[row["id"]] = row
        for rel_path in [row["input"], *row["inputs"]]:
            if not rel_path:
                continue
            by_path[rel_path].append(row)
            by_stem[Path(rel_path).stem].append(row)
    return by_id, by_path, by_stem


def _match_promoted_final(probe_row, by_id, by_path, by_stem):
    matches = []
    seen = set()

    for variant in sorted(_family_id_variants(probe_row["probe_id"], probe_row["family"])):
        row = by_id.get(variant)
        if row and row["id"] not in seen:
            matches.append((row, "id"))
            seen.add(row["id"])

    for rel_path in [probe_row["source"], *probe_row["inputs"]]:
        for row in by_path.get(rel_path, ()):
            if row["id"] not in seen:
                matches.append((row, "path"))
                seen.add(row["id"])

    for rel_path in [probe_row["source"], *probe_row["inputs"]]:
        for variant in sorted(_stem_variants(Path(rel_path).stem, probe_row["family"])):
            for row in by_stem.get(variant, ()):
                if row["id"] not in seen:
                    matches.append((row, "stem"))
                    seen.add(row["id"])

    if not matches:
        return None

    kind_rank = {"id": 0, "path": 1, "stem": 2}
    matches.sort(key=lambda item: (kind_rank[item[1]], item[0]["manifest"], item[0]["id"]))
    return {
        "match_kind": matches[0][1],
        "final_tests": [
            {
                "id": row["id"],
                "manifest": row["manifest"],
                "input": row["input"],
                "inputs": row["inputs"],
                "tags": row["tags"],
                "status": row["status"],
            }
            for row, _ in matches
        ],
    }


def _probe_only_reason(probe_row):
    note = probe_row["note"].strip().lower()
    prefix = note.split(":", 1)[0].strip() if ":" in note else ""
    for marker in PROBE_ONLY_PREFIX_MARKERS:
        if marker in prefix:
            return f"explicit probe-only note prefix: {prefix}"

    searchable = " ".join((probe_row["probe_id"], probe_row["source"], note)).lower()
    for marker in PROBE_ONLY_NAME_MARKERS:
        if marker in searchable:
            return f"explicit probe-only family marker: {marker}"

    return None


def build_audit():
    probe_rows = _probe_rows()
    final_rows = _final_rows()
    by_id, by_path, by_stem = _build_final_indexes(final_rows)

    records = []
    for probe_row in probe_rows:
        match = _match_promoted_final(probe_row, by_id, by_path, by_stem)
        if match:
            records.append(
                {
                    **probe_row,
                    "classification": "promoted",
                    "reason": f"stable final inventory match via {match['match_kind']}",
                    "match_kind": match["match_kind"],
                    "matched_final_tests": match["final_tests"],
                }
            )
            continue

        probe_only_reason = _probe_only_reason(probe_row)
        if probe_only_reason:
            records.append(
                {
                    **probe_row,
                    "classification": "probe-only",
                    "reason": probe_only_reason,
                    "match_kind": None,
                    "matched_final_tests": [],
                }
            )
            continue

        records.append(
            {
                **probe_row,
                "classification": "missing-promotion-candidate",
                "reason": "no promoted final inventory match and no explicit probe-only marker",
                "match_kind": None,
                "matched_final_tests": [],
            }
        )

    records.sort(key=lambda row: (row["classification"], row["bucket"], row["family"], row["probe_id"]))
    return {
        "summary": _build_summary(records, final_rows),
        "records": records,
    }


def _build_summary(records, final_rows):
    classifications = Counter(row["classification"] for row in records)
    by_bucket = defaultdict(Counter)
    by_family = defaultdict(Counter)
    promoted_match_kinds = Counter()
    probe_only_reasons = Counter()

    for row in records:
        by_bucket[row["bucket"]][row["classification"]] += 1
        by_family[row["family"]][row["classification"]] += 1
        if row["classification"] == "promoted" and row["match_kind"]:
            promoted_match_kinds[row["match_kind"]] += 1
        if row["classification"] == "probe-only":
            probe_only_reasons[row["reason"]] += 1

    bucket_rows = []
    for bucket in sorted(by_bucket):
        counts = by_bucket[bucket]
        bucket_rows.append(
            {
                "bucket": bucket,
                "promoted": counts["promoted"],
                "probe_only": counts["probe-only"],
                "missing_promotion_candidate": counts["missing-promotion-candidate"],
            }
        )

    family_rows = []
    for family in sorted(by_family):
        counts = by_family[family]
        family_rows.append(
            {
                "family": family,
                "promoted": counts["promoted"],
                "probe_only": counts["probe-only"],
                "missing_promotion_candidate": counts["missing-promotion-candidate"],
            }
        )

    return {
        "total_probes": len(records),
        "total_final_tests": len(final_rows),
        "promoted": classifications["promoted"],
        "probe_only": classifications["probe-only"],
        "missing_promotion_candidate": classifications["missing-promotion-candidate"],
        "promoted_match_kinds": dict(sorted(promoted_match_kinds.items())),
        "probe_only_reasons": dict(sorted(probe_only_reasons.items())),
        "by_bucket": bucket_rows,
        "by_family": family_rows,
    }


def _write_json(path, audit):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(audit, indent=2, sort_keys=False) + "\n", encoding="utf-8")


def _write_markdown(path, audit):
    summary = audit["summary"]
    records = audit["records"]

    promoted = [row for row in records if row["classification"] == "promoted"]
    probe_only = [row for row in records if row["classification"] == "probe-only"]
    missing = [row for row in records if row["classification"] == "missing-promotion-candidate"]

    lines = [
        "# Probe Promotion Audit",
        "",
        "## Summary",
        "",
        f"- Total resolved probe inventory audited: `{summary['total_probes']}`",
        f"- Promoted stable coverage: `{summary['promoted']}`",
        f"- Intentional probe-only coverage: `{summary['probe_only']}`",
        f"- Missing promotion candidates: `{summary['missing_promotion_candidate']}`",
        f"- Stable final tests scanned: `{summary['total_final_tests']}`",
        "",
        "### Promoted Match Evidence",
        "",
    ]

    for kind, count in summary["promoted_match_kinds"].items():
        lines.append(f"- `{kind}`: `{count}`")

    lines.extend(["", "### Explicit Probe-Only Reasons", ""])
    for reason, count in summary["probe_only_reasons"].items():
        lines.append(f"- `{count}`: {reason}")

    lines.extend(["", "### Bucket Breakdown", ""])
    for row in summary["by_bucket"]:
        lines.append(
            f"- Bucket `{row['bucket']}`: promoted `{row['promoted']}`, "
            f"probe-only `{row['probe_only']}`, missing `{row['missing_promotion_candidate']}`"
        )

    lines.extend(["", "## Missing Promotion Candidates", ""])
    if not missing:
        lines.append("- None.")
    else:
        current_bucket = None
        for row in missing:
            if row["bucket"] != current_bucket:
                current_bucket = row["bucket"]
                lines.extend(["", f"### Bucket {current_bucket}", ""])
            lines.append(
                f"- `{row['probe_id']}` (`{row['family']}`) from `{row['source']}`"
                f" - {row['note']}"
            )

    lines.extend(["", "## Intentional Probe-Only Coverage", ""])
    if not probe_only:
        lines.append("- None.")
    else:
        current_reason = None
        for row in probe_only:
            if row["reason"] != current_reason:
                current_reason = row["reason"]
                lines.extend(["", f"### {current_reason}", ""])
            lines.append(
                f"- `{row['probe_id']}` (`{row['family']}`) from `{row['source']}`"
                f" - {row['note']}"
            )

    lines.extend(["", "## Promoted Coverage Sample", ""])
    for row in promoted[:40]:
        targets = ", ".join(test["id"] for test in row["matched_final_tests"][:3])
        lines.append(f"- `{row['probe_id']}` via `{row['match_kind']}` -> {targets}")

    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def parse_args(argv=None):
    parser = argparse.ArgumentParser(
        description="Audit resolved final probes against promoted stable final coverage."
    )
    parser.add_argument("--json-out", type=Path, default=DEFAULT_JSON_OUT)
    parser.add_argument("--md-out", type=Path, default=DEFAULT_MD_OUT)
    return parser.parse_args(argv)


def main(argv=None):
    args = parse_args(argv)
    audit = build_audit()
    _write_json(args.json_out, audit)
    _write_markdown(args.md_out, audit)

    summary = audit["summary"]
    print(
        "promotion-audit:"
        f" promoted={summary['promoted']}"
        f" probe_only={summary['probe_only']}"
        f" missing={summary['missing_promotion_candidate']}"
    )
    print(f"json={args.json_out}")
    print(f"markdown={args.md_out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
