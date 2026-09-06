#!/usr/bin/env python3
"""Hostile tests for deployment/observation receipt separation and raw binding."""

from __future__ import annotations

import json
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
FIXTURE = ROOT / "tests/fixtures/external_evidence_receipts/v1"
sys.path.insert(0, str(ROOT / "scripts"))
import validate_external_evidence as evidence


class ExternalEvidenceReceiptTests(unittest.TestCase):
    def fixture_copy(self, temporary: str) -> Path:
        root = Path(temporary) / "evidence"
        shutil.copytree(FIXTURE, root)
        return root

    def load(self, path: Path) -> dict:
        return json.loads(path.read_text(encoding="utf-8"))

    def store(self, path: Path, value: dict) -> None:
        path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    def run_validation(self, root: Path) -> dict:
        return evidence.validate_observation(root, root / "receipts/observation.json")

    def test_fixture_chain_passes_without_live_claims(self) -> None:
        result = self.run_validation(FIXTURE.resolve())
        self.assertEqual(result["status"], "pass")
        self.assertEqual(result["evidence_class"], "contract_fixture")
        self.assertEqual(result["deployments"][0]["deployment"], "fixture_only")
        self.assertEqual(result["claim_boundary"]["digital_runtime_observation"], "fixture_only")
        self.assertEqual(len(result["observation"]["assertions"]), 2)

    def test_rejects_candidate_manifest_digest_drift(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = self.fixture_copy(temporary)
            (root / "candidate/manifest.json").write_text("{}\n", encoding="utf-8")
            with self.assertRaisesRegex(RuntimeError, "manifest digest mismatch"):
                self.run_validation(root)

    def test_rejects_artifact_byte_drift(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = self.fixture_copy(temporary)
            with (root / "candidate/firmware.bin").open("ab") as handle:
                handle.write(b"drift")
            with self.assertRaisesRegex(RuntimeError, "artifact digest mismatch"):
                self.run_validation(root)

    def test_rejects_protocol_drift(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = self.fixture_copy(temporary)
            (root / "candidate/protocol.yaml").write_text("schema: drift\n", encoding="utf-8")
            with self.assertRaisesRegex(RuntimeError, "protocol digest mismatch"):
                self.run_validation(root)

    def test_rejects_deployment_receipt_digest_drift(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = self.fixture_copy(temporary)
            path = root / "receipts/deployment.json"
            value = self.load(path)
            value["target"]["board_profile"] = "other-board"
            self.store(path, value)
            with self.assertRaisesRegex(RuntimeError, r"deployment_receipts\[0\] digest mismatch"):
                self.run_validation(root)

    def test_rejects_fixture_deployment_promotion(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = self.fixture_copy(temporary)
            deployment_path = root / "receipts/deployment.json"
            deployment = self.load(deployment_path)
            deployment["claim_boundary"]["deployment"] = "established"
            self.store(deployment_path, deployment)
            observation_path = root / "receipts/observation.json"
            observation = self.load(observation_path)
            observation["deployment_receipts"][0]["sha256"] = evidence.sha256(deployment_path)
            self.store(observation_path, observation)
            with self.assertRaisesRegex(RuntimeError, "deployment claim does not match"):
                self.run_validation(root)

    def test_rejects_deployment_runtime_promotion(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = self.fixture_copy(temporary)
            deployment_path = root / "receipts/deployment.json"
            deployment = self.load(deployment_path)
            deployment["claim_boundary"]["runtime_observation"] = "established"
            self.store(deployment_path, deployment)
            observation_path = root / "receipts/observation.json"
            observation = self.load(observation_path)
            observation["deployment_receipts"][0]["sha256"] = evidence.sha256(deployment_path)
            self.store(observation_path, observation)
            with self.assertRaisesRegex(RuntimeError, "may not promote runtime"):
                self.run_validation(root)

    def test_rejects_raw_capture_digest_drift(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = self.fixture_copy(temporary)
            with (root / "captures/serial.txt").open("a", encoding="utf-8") as handle:
                handle.write("later line\n")
            with self.assertRaisesRegex(RuntimeError, "capture digest mismatch"):
                self.run_validation(root)

    def test_rejects_capture_before_deployment_completion(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = self.fixture_copy(temporary)
            path = root / "receipts/observation.json"
            value = self.load(path)
            value["capture"]["started_at_utc"] = "2025-12-31T23:59:00Z"
            self.store(path, value)
            with self.assertRaisesRegex(RuntimeError, "started before a referenced deployment completed"):
                self.run_validation(root)

    def test_rejects_assertion_absent_from_raw_capture(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = self.fixture_copy(temporary)
            path = root / "receipts/observation.json"
            value = self.load(path)
            value["assertions"][0]["contains"] = "invented runtime fact"
            self.store(path, value)
            with self.assertRaisesRegex(RuntimeError, "not found in raw capture"):
                self.run_validation(root)

    def test_rejects_receipt_id_mismatch(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = self.fixture_copy(temporary)
            path = root / "receipts/observation.json"
            value = self.load(path)
            value["deployment_receipts"][0]["receipt_id"] = "other.deployment"
            self.store(path, value)
            with self.assertRaisesRegex(RuntimeError, "receipt_id mismatch"):
                self.run_validation(root)

    def test_rejects_path_traversal(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = self.fixture_copy(temporary)
            path = root / "receipts/observation.json"
            value = self.load(path)
            value["capture"]["path"] = "../outside.txt"
            self.store(path, value)
            with self.assertRaisesRegex(RuntimeError, "normalized relative path"):
                self.run_validation(root)

    def test_cli_output_is_deterministic_and_relocatable(self) -> None:
        with tempfile.TemporaryDirectory() as first, tempfile.TemporaryDirectory() as second:
            first_root = self.fixture_copy(first)
            second_root = self.fixture_copy(second)
            first_output = Path(first) / "chain.json"
            second_output = Path(second) / "chain.json"
            command = [
                sys.executable,
                str(ROOT / "scripts/validate_external_evidence.py"),
                "--evidence-root", "ROOT",
                "--observation", "receipts/observation.json",
                "--output", "OUTPUT",
            ]
            for fixture_root, output in ((first_root, first_output), (second_root, second_output)):
                completed = subprocess.run(
                    [str(fixture_root) if item == "ROOT" else str(output) if item == "OUTPUT" else item for item in command],
                    check=False,
                    capture_output=True,
                    text=True,
                )
                self.assertEqual(completed.returncode, 0, completed.stderr)
            self.assertEqual(first_output.read_bytes(), second_output.read_bytes())
            self.assertNotIn(str(first_root).encode(), first_output.read_bytes())
            self.assertNotIn(str(second_root).encode(), second_output.read_bytes())

    def test_cli_failure_does_not_overwrite_output(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = self.fixture_copy(temporary)
            (root / "captures/serial.txt").write_text("drift\n", encoding="utf-8")
            output = Path(temporary) / "chain.json"
            output.write_text("retained\n", encoding="utf-8")
            completed = subprocess.run(
                [sys.executable, str(ROOT / "scripts/validate_external_evidence.py"), "--evidence-root", str(root), "--observation", "receipts/observation.json", "--output", str(output)],
                check=False,
                capture_output=True,
                text=True,
            )
            self.assertNotEqual(completed.returncode, 0)
            self.assertEqual(output.read_text(encoding="utf-8"), "retained\n")


if __name__ == "__main__":
    unittest.main()
