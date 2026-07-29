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
            mixed_inputs = [
                _display_path(path)
                for path in (getattr(probe, "mixed_clang_inputs", None) or ())
            ]
            all_paths = [probe.source, *(probe.inputs or ()), *(getattr(probe, "mixed_clang_inputs", None) or ())]
            rows.append(
                {
                    "family": family,
                    "probe_id": probe.probe_id,
                    "bucket": probe.probe_id.split("__", 1)[0],
                    "source": _display_path(probe.source),
                    "inputs": inputs,
                    "mixed_inputs": mixed_inputs,
                    "missing_inputs": [
                        _display_path(path) for path in all_paths if not path.is_file()
                    ],
                    "note": probe.note,
                    "promoted_test_id": getattr(probe, "promoted_test_id", None),
                    **(
                        {
                            "promotion_disposition":
                                probe.promotion_disposition
                        }
                        if getattr(probe, "promotion_disposition", None)
                        else {}
                    ),
                }
            )
    return rows


def _final_rows():
    rows = []
    for manifest_path in iter_manifest_files():
        manifest = load_json(manifest_path)
        for test in manifest.get("tests", []):
            rel_inputs = [test.get("input", ""), *(test.get("inputs") or [])]
            rows.append(
                {
                    "id": test["id"],
                    "manifest": manifest_path.name,
                    "bucket": test.get("bucket", ""),
                    "input": test.get("input", ""),
                    "inputs": list(test.get("inputs") or []),
                    "tags": list(test.get("tags") or []),
                    "status": test.get("status", ""),
                    "run": bool(test.get("run", False)),
                    "missing_inputs": [
                        rel_path
                        for rel_path in rel_inputs
                        if rel_path and not (FINAL_ROOT / rel_path).is_file()
                    ],
                    "shape_key": json.dumps(
                        {
                            key: test.get(key)
                            for key in (
                                "input",
                                "inputs",
                                "expects",
                                "run",
                                "expected_stdout",
                                "expected_stderr",
                                "expect_exit",
                                "expect_compile_exit",
                                "args",
                                "env",
                                "standard",
                                "differential",
                                "differential_compiler",
                                "mixed_clang_inputs",
                                "mixed_clang_compiler",
                            )
                        },
                        sort_keys=True,
                    ),
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
        if tail.startswith("diagjson_"):
            semantic_tail = tail[len("diagjson_"):]
            variants.add(f"{bucket}__diagjson__{semantic_tail}")
            variants.add(f"{bucket}__line_directive_{semantic_tail}")
            if semantic_tail.endswith("_strict"):
                variants.add(
                    f"{bucket}__line_directive_"
                    f"{semantic_tail[:-len('_strict')]}_diagjson_strict"
                )
            if semantic_tail.endswith("_current_empty"):
                variants.add(
                    f"{bucket}__line_directive_"
                    f"{semantic_tail[:-len('_current_empty')]}_diagjson_current_empty"
                )
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
    promoted_test_id = probe_row.get("promoted_test_id")
    if promoted_test_id:
        row = by_id.get(promoted_test_id)
        if not row:
            return None
        return {
            "match_kind": "explicit-id",
            "final_tests": [
                {
                    "id": row["id"],
                    "manifest": row["manifest"],
                    "input": row["input"],
                    "inputs": row["inputs"],
                    "tags": row["tags"],
                    "status": row["status"],
                }
            ],
        }

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
    best_rank = kind_rank[matches[0][1]]
    matches = [item for item in matches if kind_rank[item[1]] == best_rank]
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
    if probe_row.get("promotion_disposition") == "probe-only":
        return "explicit probe-only disposition"
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
        probe_only_reason = _probe_only_reason(probe_row)
        if probe_only_reason == "explicit probe-only disposition":
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
    probe_id_counts = Counter(row["probe_id"] for row in probe_rows)
    final_id_counts = Counter(row["id"] for row in final_rows)
    missing_probe_inputs = [
        {
            "probe_id": row["probe_id"],
            "family": row["family"],
            "paths": row["missing_inputs"],
        }
        for row in probe_rows
        if row["missing_inputs"]
    ]
    missing_final_inputs = [
        {
            "id": row["id"],
            "manifest": row["manifest"],
            "paths": row["missing_inputs"],
        }
        for row in final_rows
        if row["missing_inputs"]
    ]
    missing_explicit_promotions = [
        {
            "probe_id": row["probe_id"],
            "promoted_test_id": row["promoted_test_id"],
        }
        for row in probe_rows
        if row.get("promoted_test_id") and row["promoted_test_id"] not in by_id
    ]
    promoted_non_ok = []
    ambiguous_stem_matches = []
    ambiguous_best_rank_matches = []
    for row in records:
        if row["classification"] != "promoted":
            continue
        non_ok = [test for test in row["matched_final_tests"] if test["status"] != "ok"]
        if non_ok:
            promoted_non_ok.append(
                {
                    "probe_id": row["probe_id"],
                    "matches": non_ok,
                }
            )
        if row["match_kind"] == "stem" and len(row["matched_final_tests"]) > 1:
            ambiguous_stem_matches.append(
                {
                    "probe_id": row["probe_id"],
                    "matches": row["matched_final_tests"],
                }
            )
        if len(row["matched_final_tests"]) > 1:
            ambiguous_best_rank_matches.append(
                {
                    "probe_id": row["probe_id"],
                    "match_kind": row["match_kind"],
                    "matches": row["matched_final_tests"],
                }
            )

    shape_groups = defaultdict(list)
    for row in final_rows:
        shape_key = row.get("shape_key")
        if shape_key:
            shape_groups[shape_key].append(
                {"id": row["id"], "manifest": row["manifest"]}
            )
    duplicate_final_shapes = [
        {"shape_key": shape_key, "tests": tests}
        for shape_key, tests in sorted(shape_groups.items())
        if len(tests) > 1
    ]

    probe_paths = set()
    for row in probe_rows:
        probe_paths.update(
            path for path in [row["source"], *row["inputs"], *row["mixed_inputs"]]
            if path.startswith("probes/")
        )
    final_paths = set()
    for row in final_rows:
        final_paths.update(
            path for path in [row["input"], *row["inputs"]]
            if path.startswith("probes/")
        )
    disk_probe_paths = {
        str(path.relative_to(FINAL_ROOT))
        for path in (FINAL_ROOT / "probes").rglob("*.c")
    }
    orphan_probe_assets = []
    for path in sorted(disk_probe_paths):
        in_probe = path in probe_paths
        in_final = path in final_paths
        if in_probe and in_final:
            continue
        if in_probe:
            classification = "inventory-only"
        elif in_final:
            classification = "stable-only"
        else:
            classification = "unowned"
        orphan_probe_assets.append(
            {"path": path, "classification": classification}
        )
    integrity = {
        "duplicate_probe_ids": sorted(
            probe_id for probe_id, count in probe_id_counts.items() if count > 1
        ),
        "duplicate_final_ids": sorted(
            test_id for test_id, count in final_id_counts.items() if count > 1
        ),
        "missing_probe_inputs": missing_probe_inputs,
        "missing_final_inputs": missing_final_inputs,
        "missing_explicit_promotions": missing_explicit_promotions,
        "promoted_non_ok": promoted_non_ok,
        "ambiguous_stem_matches": ambiguous_stem_matches,
        "ambiguous_best_rank_matches": ambiguous_best_rank_matches,
        "duplicate_final_shapes": duplicate_final_shapes,
        "orphan_probe_assets": orphan_probe_assets,
    }
    integrity["critical_error_count"] = sum(
        len(integrity[key])
        for key in (
            "duplicate_probe_ids",
            "duplicate_final_ids",
            "missing_probe_inputs",
            "missing_final_inputs",
            "missing_explicit_promotions",
            "promoted_non_ok",
            "ambiguous_stem_matches",
        )
    )
    return {
        "summary": _build_summary(records, final_rows),
        "integrity": integrity,
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
    integrity = audit["integrity"]

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
        f"- Critical integrity errors: `{integrity['critical_error_count']}`",
        f"- Ambiguous stem matches: `{len(integrity['ambiguous_stem_matches'])}`",
        f"- Multi-owner best-rank matches: `{len(integrity['ambiguous_best_rank_matches'])}`",
        f"- Duplicate stable semantic shapes: `{len(integrity['duplicate_final_shapes'])}`",
        f"- Probe assets outside both lanes: `{sum(1 for row in integrity['orphan_probe_assets'] if row['classification'] == 'unowned')}`",
        "",
        "## Integrity",
        "",
    ]

    for label, key in (
        ("Duplicate probe IDs", "duplicate_probe_ids"),
        ("Duplicate final IDs", "duplicate_final_ids"),
        ("Missing probe inputs", "missing_probe_inputs"),
        ("Missing final inputs", "missing_final_inputs"),
        ("Missing explicit promotion owners", "missing_explicit_promotions"),
        ("Promoted non-ok matches", "promoted_non_ok"),
    ):
        lines.append(f"- {label}: `{len(integrity[key])}`")
    lines.append(
        "- Ambiguous stem ownership requiring review: "
        f"`{len(integrity['ambiguous_stem_matches'])}`"
    )
    lines.append(
        "- Multi-owner best-rank matches requiring explicit-owner migration: "
        f"`{len(integrity['ambiguous_best_rank_matches'])}`"
    )
    lines.append(
        "- Duplicate stable semantic shapes: "
        f"`{len(integrity['duplicate_final_shapes'])}`"
    )
    for classification in ("stable-only", "inventory-only", "unowned"):
        count = sum(
            1 for row in integrity["orphan_probe_assets"]
            if row["classification"] == classification
        )
        lines.append(f"- Probe assets `{classification}`: `{count}`")

    lines.extend([
        "",
        "### Promoted Match Evidence",
        "",
    ])

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
    print(
        "promotion-integrity:"
        f" critical={audit['integrity']['critical_error_count']}"
        f" ambiguous_stem={len(audit['integrity']['ambiguous_stem_matches'])}"
    )
    print(f"json={args.json_out}")
    print(f"markdown={args.md_out}")
    return 1 if audit["integrity"]["critical_error_count"] else 0


if __name__ == "__main__":
    raise SystemExit(main())
