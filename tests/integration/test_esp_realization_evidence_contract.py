#!/usr/bin/env python3
"""Hostile contract tests for the fixed E016 Slice F evidence lane."""

from __future__ import annotations

import json
import sys
import tempfile
import unittest
from copy import deepcopy
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import run_esp_realization_evidence as realization


class EspRealizationEvidenceContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.original = json.loads(realization.DEFAULT_CONTRACT.read_text(encoding="utf-8"))

    def assert_rejected(self, mutate, pattern: str) -> None:
        data = deepcopy(self.original)
        mutate(data)
        with tempfile.TemporaryDirectory(prefix="fisics-e016-realization-contract-") as temporary:
            path = Path(temporary) / "contract.json"
            path.write_text(json.dumps(data), encoding="utf-8")
            with self.assertRaisesRegex(RuntimeError, pattern):
                realization.load_realization_contract(path)

    def test_checked_in_contract_loads(self) -> None:
        realization.load_realization_contract(realization.DEFAULT_CONTRACT)

    def test_rejects_unknown_field(self) -> None:
        self.assert_rejected(lambda data: data.update({"telemetry": {}}), "fields mismatch")

    def test_rejects_wrong_slice_e_path(self) -> None:
        self.assert_rejected(lambda data: data["slice_e"].update({"contract_path": "tests/other.json"}), "fixed Slice E")

    def test_rejects_missing_evidence_source(self) -> None:
        self.assert_rejected(lambda data: data["external_evidence"].pop(), "evidence count drift")

    def test_rejects_duplicate_candidate_role(self) -> None:
        self.assert_rejected(lambda data: data["candidates"][1].update({"role": data["candidates"][0]["role"]}), "duplicate candidate role")

    def test_rejects_malformed_static_digest(self) -> None:
        self.assert_rejected(lambda data: data["static_artifacts"].update({"semantic_project_graph_sha256": "0"}), "lowercase SHA-256")

    def test_rejects_hidden_deployment_resolution(self) -> None:
        self.assert_rejected(lambda data: data["expected_projection"].update({"bench_b_selected_candidate_upload": "documented_consistent"}), "projection drift")

    def test_rejects_fresh_runtime_promotion(self) -> None:
        self.assert_rejected(lambda data: data["expected_projection"].update({"fresh_runtime_state": "observed"}), "projection drift")


if __name__ == "__main__":
    unittest.main()
