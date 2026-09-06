#!/usr/bin/env python3
"""Hostile contract tests for the fixed ESP portable C intake selection."""

from __future__ import annotations

import json
import sys
import tempfile
import unittest
from copy import deepcopy
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import run_esp_portable_c as intake


class EspPortableCContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.original = json.loads(intake.DEFAULT_CONTRACT.read_text(encoding="utf-8"))

    def load_modified(self, data: dict) -> dict:
        with tempfile.TemporaryDirectory(prefix="fisics-esp-intake-contract-") as temporary:
            path = Path(temporary) / "contract.json"
            path.write_text(json.dumps(data), encoding="utf-8")
            return intake.load_contract(path)

    def test_checked_in_contract_loads(self) -> None:
        loaded = intake.load_contract(intake.DEFAULT_CONTRACT)
        self.assertEqual(loaded["lane_id"], "esp.normalized_signal_policy.v1")
        self.assertEqual(len(loaded["external_inputs"]), 10)
        self.assertEqual(len(loaded["metadata"]["records"]), 2)
        self.assertEqual(len(loaded["metadata"]["edges"]), 2)
        self.assertEqual(len(loaded["candidates"]), 2)

    def assert_rejected(self, mutate, pattern: str) -> None:
        data = deepcopy(self.original)
        mutate(data)
        with self.assertRaisesRegex(RuntimeError, pattern):
            self.load_modified(data)

    def test_rejects_unknown_top_level_field(self) -> None:
        self.assert_rejected(lambda data: data.update({"route": "caller-selected"}), "fields mismatch")

    def test_rejects_zero_external_selection(self) -> None:
        self.assert_rejected(lambda data: data.update({"external_inputs": []}), "at least one")

    def test_rejects_zero_vector_count(self) -> None:
        self.assert_rejected(lambda data: data["behavior"].update({"normalization_cases": 0}), "frozen value 6")

    def test_rejects_zero_runs(self) -> None:
        self.assert_rejected(lambda data: data["behavior"].update({"runs_per_compiler": 0}), "frozen value 2")

    def test_rejects_rewritten_expected_oracle(self) -> None:
        self.assert_rejected(lambda data: data["behavior"].update({"expected_stdout": "PASS\n"}), "frozen oracle")

    def test_rejects_empty_metadata_records(self) -> None:
        self.assert_rejected(lambda data: data["metadata"].update({"records": []}), "must not be empty")

    def test_rejects_empty_metadata_edges(self) -> None:
        self.assert_rejected(lambda data: data["metadata"].update({"edges": []}), "must not be empty")

    def test_rejects_no_candidate_receipts(self) -> None:
        self.assert_rejected(lambda data: data.update({"candidates": []}), "at least one retained")

    def test_rejects_escaping_external_path(self) -> None:
        self.assert_rejected(lambda data: data["external_inputs"][0].update({"path": "../outside.c"}), "repository-relative")

    def test_rejects_absolute_candidate_path(self) -> None:
        self.assert_rejected(lambda data: data["candidates"][0].update({"manifest_path": "/tmp/manifest.json"}), "repository-relative")

    def test_rejects_duplicate_input_role(self) -> None:
        self.assert_rejected(lambda data: data["external_inputs"][1].update({"role": data["external_inputs"][0]["role"]}), "duplicate external input role")

    def test_rejects_duplicate_input_path(self) -> None:
        self.assert_rejected(lambda data: data["external_inputs"][1].update({"path": data["external_inputs"][0]["path"]}), "duplicate external input path")

    def test_rejects_unknown_behavior_role(self) -> None:
        self.assert_rejected(lambda data: data["behavior"].update({"source_role": "unknown"}), "unknown role")

    def test_rejects_non_external_metadata_edge(self) -> None:
        self.assert_rejected(lambda data: data["metadata"]["edges"][0].update({"resolution": "resolved"}), "must be external")

    def test_rejects_metadata_target_divergence(self) -> None:
        self.assert_rejected(lambda data: data["metadata"]["edges"][0].update({"target_identity": "signal.other.target"}), "must equal protocol target")

    def test_rejects_malformed_digest(self) -> None:
        self.assert_rejected(lambda data: data["external_inputs"][0].update({"sha256": "0"}), "lowercase SHA-256")

    def test_rejects_duplicate_json_key(self) -> None:
        with tempfile.TemporaryDirectory(prefix="fisics-esp-intake-contract-") as temporary:
            path = Path(temporary) / "contract.json"
            path.write_text('{"schema":"one","schema":"two"}', encoding="utf-8")
            with self.assertRaisesRegex(RuntimeError, "duplicate JSON key"):
                intake.load_contract(path)


if __name__ == "__main__":
    unittest.main()
