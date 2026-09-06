#!/usr/bin/env python3
"""Hostile contract tests for the fixed E016 Slice E proof lane."""

from __future__ import annotations

import json
import sys
import tempfile
import unittest
from copy import deepcopy
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import run_esp_deterministic_proof as proof


class EspDeterministicProofContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.original = json.loads(proof.DEFAULT_CONTRACT.read_text(encoding="utf-8"))

    def load_modified(self, data: dict) -> dict:
        with tempfile.TemporaryDirectory(prefix="fisics-e016-proof-contract-") as temporary:
            path = Path(temporary) / "contract.json"
            path.write_text(json.dumps(data), encoding="utf-8")
            return proof.load_proof_contract(path)

    def assert_rejected(self, mutate, pattern: str) -> None:
        data = deepcopy(self.original)
        mutate(data)
        with self.assertRaisesRegex(RuntimeError, pattern):
            self.load_modified(data)

    def test_checked_in_contract_loads(self) -> None:
        proof.load_proof_contract(proof.DEFAULT_CONTRACT)

    def test_rejects_unknown_field(self) -> None:
        self.assert_rejected(lambda data: data.update({"bench": {}}), "fields mismatch")

    def test_rejects_prerequisite_path_drift(self) -> None:
        self.assert_rejected(lambda data: data["prerequisite_contracts"][0].update({"path": "tests/other.json"}), "role/path drift")

    def test_rejects_terminal_graph_removal(self) -> None:
        self.assert_rejected(lambda data: data["terminal_graphs"].pop(), "count drift")

    def test_rejects_duplicate_negative_class(self) -> None:
        self.assert_rejected(lambda data: data["required_negative_classes"].append(data["required_negative_classes"][0]), "unique array")

    def test_rejects_missing_negative_class(self) -> None:
        self.assert_rejected(lambda data: data["required_negative_classes"].pop(), "coverage drift")

    def test_rejects_live_evidence_promotion(self) -> None:
        self.assert_rejected(lambda data: data["expected_corpus"].update({"live_evidence_state": "passed"}), "live-evidence promotion")

    def test_rejects_duplicate_json_key(self) -> None:
        with tempfile.TemporaryDirectory(prefix="fisics-e016-proof-contract-") as temporary:
            path = Path(temporary) / "contract.json"
            path.write_text('{"schema":"one","schema":"two"}', encoding="utf-8")
            with self.assertRaisesRegex(RuntimeError, "duplicate JSON key"):
                proof.load_proof_contract(path)


if __name__ == "__main__":
    unittest.main()
