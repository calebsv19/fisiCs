#!/usr/bin/env python3
"""Contract tests for external immutable OS-source canary assets."""

from __future__ import annotations

import json
import sys
import unittest
from copy import deepcopy
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import run_external_os_dev_canary as canary


class ExternalCanaryContractTests(unittest.TestCase):
    def test_checked_in_contracts_load(self) -> None:
        contracts = sorted((canary.LANE_ROOT / "canaries").glob("*_canary.json"))
        self.assertGreaterEqual(len(contracts), 3)
        for path in contracts:
            with self.subTest(contract=path.name):
                self.assertEqual(canary.load_contract(path)["schema_version"], 1)

    def test_runtime_driver_must_remain_in_canaries(self) -> None:
        path = canary.LANE_ROOT / "canaries/edu62_control_kernel_canary.json"
        data = json.loads(path.read_text(encoding="utf-8"))
        data = deepcopy(data)
        data["runtime_driver"] = "../run_os_policy.py"
        temporary = canary.LANE_ROOT / "canaries/.external-contract-test.json"
        try:
            temporary.write_text(json.dumps(data), encoding="utf-8")
            with self.assertRaisesRegex(RuntimeError, "canaries directory"):
                canary.load_contract(temporary)
        finally:
            temporary.unlink(missing_ok=True)


if __name__ == "__main__":
    unittest.main()
