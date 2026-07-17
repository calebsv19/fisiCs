#!/usr/bin/env python3

import unittest
from unittest.mock import patch

from lib.promotion_audit import _family_id_variants, build_audit


def probe_row(probe_id="04__probe_contract", missing_inputs=None, promoted_test_id=None):
    return {
        "family": "runtime",
        "probe_id": probe_id,
        "bucket": "04",
        "source": "probes/runtime/04__probe_contract.c",
        "inputs": [],
        "mixed_inputs": [],
        "missing_inputs": list(missing_inputs or []),
        "note": "contract canary",
        "promoted_test_id": promoted_test_id,
    }


def final_row(
    test_id="04__runtime_contract",
    status="ok",
    manifest="04-contract.json",
    input_path="probes/runtime/04__probe_contract.c",
):
    return {
        "id": test_id,
        "manifest": manifest,
        "bucket": "declarations",
        "input": input_path,
        "inputs": [],
        "tags": ["contract"],
        "status": status,
        "run": True,
        "missing_inputs": [],
        "shape_key": f"shape:{test_id}",
    }


class PromotionAuditContractTests(unittest.TestCase):
    def build(self, probes, finals):
        with patch("lib.promotion_audit._probe_rows", return_value=probes):
            with patch("lib.promotion_audit._final_rows", return_value=finals):
                return build_audit()

    def test_missing_probe_input_is_counted_once(self):
        audit = self.build(
            [probe_row(missing_inputs=["probes/runtime/missing.c"])],
            [final_row()],
        )
        self.assertEqual(len(audit["integrity"]["missing_probe_inputs"]), 1)
        self.assertEqual(audit["integrity"]["critical_error_count"], 1)

    def test_duplicate_probe_id_is_critical(self):
        audit = self.build([probe_row(), probe_row()], [final_row()])
        self.assertEqual(
            audit["integrity"]["duplicate_probe_ids"],
            ["04__probe_contract"],
        )
        self.assertEqual(audit["integrity"]["critical_error_count"], 1)

    def test_duplicate_final_id_is_critical(self):
        audit = self.build([probe_row()], [final_row(), final_row()])
        self.assertEqual(
            audit["integrity"]["duplicate_final_ids"],
            ["04__runtime_contract"],
        )
        self.assertEqual(audit["integrity"]["critical_error_count"], 1)

    def test_promoted_non_ok_match_is_critical(self):
        audit = self.build([probe_row()], [final_row(status="blocked")])
        self.assertEqual(len(audit["integrity"]["promoted_non_ok"]), 1)
        self.assertEqual(audit["integrity"]["critical_error_count"], 1)

    def test_exact_id_ownership_excludes_lower_confidence_path_matches(self):
        audit = self.build(
            [probe_row()],
            [
                final_row(),
                final_row(test_id="04__runtime_unrelated_contract"),
            ],
        )
        record = audit["records"][0]
        self.assertEqual(record["match_kind"], "id")
        self.assertEqual(
            [test["id"] for test in record["matched_final_tests"]],
            ["04__runtime_contract"],
        )

    def test_diagjson_variants_include_canonical_and_current_empty_ids(self):
        canonical = _family_id_variants(
            "07__probe_diagjson_agg_arrow_ptr_to_scalar_reject",
            "diagnostic-json",
        )
        self.assertIn("07__diagjson__agg_arrow_ptr_to_scalar_reject", canonical)

        current_empty = _family_id_variants(
            "10__probe_diagjson_wave67_include_block_extern_function_object_current_empty",
            "diagnostic-json",
        )
        self.assertIn(
            "10__line_directive_wave67_include_block_extern_function_object_diagjson_current_empty",
            current_empty,
        )

    def test_ambiguous_stem_ownership_is_critical(self):
        probe = probe_row(probe_id="04__probe_contract_alias")
        probe["source"] = "probes/runtime/04__contract.c"
        audit = self.build(
            [probe],
            [
                final_row(test_id="04__first", input_path="cases/04__contract.c"),
                final_row(
                    test_id="04__second",
                    manifest="04-contract-second.json",
                    input_path="cases/04__contract.c",
                ),
            ],
        )
        self.assertEqual(len(audit["integrity"]["ambiguous_stem_matches"]), 1)
        self.assertEqual(audit["integrity"]["critical_error_count"], 1)

    def test_explicit_owner_resolves_same_stem_collision(self):
        audit = self.build(
            [probe_row(promoted_test_id="04__second")],
            [
                final_row(test_id="04__first"),
                final_row(test_id="04__second", manifest="04-contract-second.json"),
            ],
        )
        record = audit["records"][0]
        self.assertEqual(record["match_kind"], "explicit-id")
        self.assertEqual(record["matched_final_tests"][0]["id"], "04__second")
        self.assertEqual(audit["integrity"]["ambiguous_stem_matches"], [])

    def test_missing_explicit_owner_is_critical(self):
        audit = self.build(
            [probe_row(promoted_test_id="04__missing")],
            [final_row()],
        )
        self.assertEqual(len(audit["integrity"]["missing_explicit_promotions"]), 1)
        self.assertEqual(audit["integrity"]["critical_error_count"], 1)

    def test_same_rank_multi_owner_is_reported_even_for_path_matches(self):
        probe = probe_row(probe_id="04__probe_unmapped")
        audit = self.build(
            [probe],
            [
                final_row(test_id="04__first"),
                final_row(
                    test_id="04__second",
                    manifest="04-contract-second.json",
                ),
            ],
        )
        self.assertEqual(
            len(audit["integrity"]["ambiguous_best_rank_matches"]),
            1,
        )


if __name__ == "__main__":
    unittest.main()
