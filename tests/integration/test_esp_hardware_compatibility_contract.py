#!/usr/bin/env python3
"""Hostile contract tests for the fixed E016 Slice D hardware lane."""

from __future__ import annotations

import json
import sys
import tempfile
import unittest
from copy import deepcopy
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import run_esp_hardware_compatibility as hardware


class EspHardwareCompatibilityContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.original = json.loads(hardware.DEFAULT_CONTRACT.read_text(encoding="utf-8"))

    def load_modified(self, data: dict) -> dict:
        with tempfile.TemporaryDirectory(prefix="fisics-e016-hardware-contract-") as temporary:
            path = Path(temporary) / "contract.json"
            path.write_text(json.dumps(data), encoding="utf-8")
            return hardware.load_hardware_contract(path)

    def assert_rejected(self, mutate, pattern: str) -> None:
        data = deepcopy(self.original)
        mutate(data)
        with self.assertRaisesRegex(RuntimeError, pattern):
            self.load_modified(data)

    def test_checked_in_contract_and_inputs_load(self) -> None:
        contract = hardware.load_hardware_contract(hardware.DEFAULT_CONTRACT)
        authored = hardware.verify_pinned_files(hardware.FISICS_ROOT, contract["authored_inputs"], "authored")
        hardware.verify_authored_inputs(authored)

    def test_rejects_unknown_top_level_field(self) -> None:
        self.assert_rejected(lambda data: data.update({"runtime": {}}), "fields mismatch")

    def test_rejects_missing_external_input(self) -> None:
        self.assert_rejected(lambda data: data.update({"external_inputs": data["external_inputs"][:-1]}), "exactly 6")

    def test_rejects_duplicate_authored_role(self) -> None:
        self.assert_rejected(lambda data: data["authored_inputs"][1].update({"role": data["authored_inputs"][0]["role"]}), "duplicate authored_inputs")

    def test_rejects_escaping_path(self) -> None:
        self.assert_rejected(lambda data: data["authored_inputs"][0].update({"path": "../catalog.json"}), "repository-relative")

    def test_rejects_malformed_slice_c_digest(self) -> None:
        self.assert_rejected(lambda data: data["slice_c"].update({"physical_graph_sha256": "0"}), "lowercase SHA-256")

    def test_rejects_board_scope_drift(self) -> None:
        self.assert_rejected(lambda data: data["declared_scope"].update({"board_profile": "board.other"}), "declared hardware scope drift")

    def test_rejects_gpio_scope_drift(self) -> None:
        self.assert_rejected(lambda data: data["declared_scope"].update({"twai_tx_endpoint": "gpio9"}), "declared hardware scope drift")

    def test_rejects_bus_master_claim(self) -> None:
        self.assert_rejected(lambda data: data["declared_scope"].update({"can_network_anchor": "bus_master"}), "declared hardware scope drift")

    def test_rejects_electrical_claim_removal(self) -> None:
        self.assert_rejected(lambda data: data["declared_scope"]["electrical_declarations_outside_graph_v1"].pop(), "declared hardware scope drift")

    def test_rejects_hardware_presence_claim(self) -> None:
        self.assert_rejected(lambda data: data["expected_graph"].update({"hardware_presence": "present"}), "graph expectation drift")

    def test_rejects_wiring_claim(self) -> None:
        self.assert_rejected(lambda data: data["expected_graph"].update({"wiring": "verified"}), "graph expectation drift")

    def test_rejects_physical_acceptance_claim(self) -> None:
        self.assert_rejected(lambda data: data["expected_graph"].update({"physical_acceptance": "passed"}), "graph expectation drift")

    def test_rejects_local_physical_evidence_promotion(self) -> None:
        self.assert_rejected(lambda data: data["expected_graph"].update({"physical_evidence_state": "source_compatible"}), "graph expectation drift")

    def test_rejects_dishonest_catalog_with_updated_digest(self) -> None:
        build_parent = hardware.FISICS_ROOT / "build"
        build_parent.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="e016-hardware-hostile-", dir=build_parent) as temporary:
            root = Path(temporary)
            contract = deepcopy(self.original)
            for item in contract["authored_inputs"]:
                source = hardware.FISICS_ROOT / item["path"]
                target = root / source.name
                data = json.loads(source.read_text(encoding="utf-8"))
                if item["role"] == "hardware_component_catalog":
                    board = data["board_profiles"][0]
                    next(endpoint for endpoint in board["endpoints"] if endpoint["endpoint_id"] == "gpio11")["capability_ids"] = ["endpoint.digital.output"]
                target.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
                item["path"] = target.relative_to(hardware.FISICS_ROOT).as_posix()
                item["sha256"] = hardware.intake.sha256_path(target)
            loaded = hardware.load_hardware_contract_from_data(contract)
            paths = hardware.verify_pinned_files(hardware.FISICS_ROOT, loaded["authored_inputs"], "hostile authored")
            with self.assertRaisesRegex(RuntimeError, "GPIO11 protection capability drift"):
                hardware.verify_authored_inputs(paths)

    def test_rejects_duplicate_json_key(self) -> None:
        with tempfile.TemporaryDirectory(prefix="fisics-e016-hardware-contract-") as temporary:
            path = Path(temporary) / "contract.json"
            path.write_text('{"schema":"one","schema":"two"}', encoding="utf-8")
            with self.assertRaisesRegex(RuntimeError, "duplicate JSON key"):
                hardware.load_hardware_contract(path)


if __name__ == "__main__":
    unittest.main()
