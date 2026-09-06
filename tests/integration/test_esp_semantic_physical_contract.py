#!/usr/bin/env python3
"""Hostile contract tests for the fixed E016 Slice C composition."""

from __future__ import annotations

import json
import sys
import tempfile
import unittest
from copy import deepcopy
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import run_esp_semantic_physical as composition


class EspSemanticPhysicalContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.original = json.loads(composition.DEFAULT_CONTRACT.read_text(encoding="utf-8"))

    def load_modified(self, data: dict) -> dict:
        with tempfile.TemporaryDirectory(prefix="fisics-e016-composition-contract-") as temporary:
            path = Path(temporary) / "contract.json"
            path.write_text(json.dumps(data), encoding="utf-8")
            return composition.load_composition_contract(path)

    def assert_rejected(self, mutate, pattern: str) -> None:
        data = deepcopy(self.original)
        mutate(data)
        with self.assertRaisesRegex(RuntimeError, pattern):
            self.load_modified(data)

    def test_checked_in_contract_and_authored_inputs_load(self) -> None:
        contract = composition.load_composition_contract(composition.DEFAULT_CONTRACT)
        paths = composition.verify_authored_inputs(contract)
        self.assertEqual(set(paths), {
            "semantic_project_manifest", "physical_contract_catalog", "physical_project_manifest"
        })

    def test_rejects_unknown_top_level_field(self) -> None:
        self.assert_rejected(lambda data: data.update({"hardware": {}}), "fields mismatch")

    def test_rejects_escaping_authored_path(self) -> None:
        self.assert_rejected(lambda data: data["authored_inputs"][0].update({"path": "../manifest.json"}), "repository-relative")

    def test_rejects_missing_authored_role(self) -> None:
        self.assert_rejected(lambda data: data.update({"authored_inputs": data["authored_inputs"][:2]}), "exactly three")

    def test_rejects_malformed_digest(self) -> None:
        self.assert_rejected(lambda data: data["intake_contract"].update({"sha256": "0"}), "lowercase SHA-256")

    def test_rejects_changed_numeric_range(self) -> None:
        self.assert_rejected(lambda data: data["representation"].update({"integer_maximum": 100}), "dishonest or drifted")

    def test_rejects_voltage_interpretation(self) -> None:
        self.assert_rejected(lambda data: data["representation"].update({"dimension": "voltage"}), "dishonest or drifted")

    def test_rejects_calibrated_claim(self) -> None:
        self.assert_rejected(lambda data: data["representation"].update({"calibration": "calibrated"}), "dishonest or drifted")

    def test_rejects_concrete_unit(self) -> None:
        self.assert_rejected(lambda data: data["representation"].update({"unit_policy": "exact_unit"}), "dishonest or drifted")

    def test_rejects_vector_form(self) -> None:
        self.assert_rejected(lambda data: data["representation"].update({"value_form": "vector"}), "dishonest or drifted")

    def test_rejects_local_resolution_expectation(self) -> None:
        self.assert_rejected(lambda data: data["expected_graphs"].update({"semantic_edge_resolution": "resolved"}), "graph expectation drift")

    def verify_mutated_authored_file_rejected(self, role: str, mutate, pattern: str) -> None:
        build_parent = composition.FISICS_ROOT / "build"
        build_parent.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="e016-hostile-", dir=build_parent) as temporary:
            root = Path(temporary)
            contract = deepcopy(self.original)
            for item in contract["authored_inputs"]:
                source = composition.FISICS_ROOT / item["path"]
                target = root / source.name
                data = json.loads(source.read_text(encoding="utf-8"))
                if item["role"] == role:
                    mutate(data)
                target.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
                item["path"] = target.relative_to(composition.FISICS_ROOT).as_posix()
                item["sha256"] = composition.intake.sha256_path(target)
            loaded = composition.load_composition_contract_from_data(contract)
            with self.assertRaisesRegex(RuntimeError, pattern):
                composition.verify_authored_inputs(loaded)

    def test_rejects_unapproved_external_target_even_with_updated_digest(self) -> None:
        self.verify_mutated_authored_file_rejected(
            "semantic_project_manifest",
            lambda data: data.update({"allowed_external_targets": []}),
            "explicitly allow only",
        )

    def test_rejects_dishonest_physical_catalog_even_with_updated_digest(self) -> None:
        self.verify_mutated_authored_file_rejected(
            "physical_contract_catalog",
            lambda data: data["quantity_kinds"][0].update({
                "dimension_vector": [2, 1, -3, -1, 0, 0, 0, 0],
                "dimension_text": "voltage",
            }),
            "dishonest or drifted dimensionless",
        )

    def test_rejects_duplicate_json_key(self) -> None:
        with tempfile.TemporaryDirectory(prefix="fisics-e016-composition-contract-") as temporary:
            path = Path(temporary) / "contract.json"
            path.write_text('{"schema":"one","schema":"two"}', encoding="utf-8")
            with self.assertRaisesRegex(RuntimeError, "duplicate JSON key"):
                composition.load_composition_contract(path)


if __name__ == "__main__":
    unittest.main()
