#!/usr/bin/env python3
"""Contract tests for the OS-P manifest and selector boundary."""

from __future__ import annotations

import json
import io
import sys
import tempfile
import unittest
from contextlib import redirect_stderr, redirect_stdout
from copy import deepcopy
from pathlib import Path
from unittest.mock import patch

import run_os_policy


def valid_manifest() -> dict:
    return {
        "schema_version": 2,
        "lane_id": "os-policy",
        "lane_version": "OS-P2",
        "target": "x86_64-unknown-none",
        "guest_harness": {
            "boot_source": "guest/boot.asm",
            "entry_source": "guest/entry.asm",
            "linker_script": "guest/linker.ld",
            "kernel_load_address": 65536,
            "max_kernel_sectors": 64,
            "qemu_machine": "pc,accel=tcg",
            "qemu_cpu": "qemu64",
            "memory_mb": 16,
            "smp": 1,
        },
        "cases": [
            {
                "id": "case_a",
                "description": "fixture",
                "source": "cases/osp0_core_policy.c",
                "runtime_driver": "cases/osp0_core_policy_driver.c",
                "expected_stdout": "expect/osp0_core_policy.stdout",
                "expected_exit": 0,
                "introduced_in": "OS-P0",
                "provenance": {},
                "object_contract": {},
                "guest_contract": {
                    "adapter_source": "cases/osp0_core_policy_guest.c",
                    "expected_serial": "expect/osp0_core_policy.serial",
                    "expected_exit": 85,
                    "debug_exit_value": 42,
                    "timeout_seconds": 10,
                    "repeat_runs": 2,
                    "parity_artifact": "serial_transcript",
                },
            }
        ],
    }


class ManifestContractTests(unittest.TestCase):
    def load(self, data: dict) -> dict:
        with tempfile.TemporaryDirectory(prefix="osp-contract-") as root:
            path = Path(root) / "manifest.json"
            path.write_text(json.dumps(data), encoding="utf-8")
            return run_os_policy.load_manifest(path)

    def test_valid_manifest_loads(self) -> None:
        loaded = self.load(valid_manifest())
        self.assertEqual(loaded["lane_version"], "OS-P2")

    def test_duplicate_case_ids_fail(self) -> None:
        data = valid_manifest()
        data["cases"].append(deepcopy(data["cases"][0]))
        with self.assertRaisesRegex(RuntimeError, "unique"):
            self.load(data)

    def test_missing_required_case_field_fails(self) -> None:
        data = valid_manifest()
        del data["cases"][0]["object_contract"]
        with self.assertRaisesRegex(RuntimeError, "missing fields"):
            self.load(data)

    def test_zero_selection_fails(self) -> None:
        with self.assertRaisesRegex(RuntimeError, "no OS-P cases selected"):
            run_os_policy.select_cases(valid_manifest(), "missing")

    def test_absolute_lane_path_fails(self) -> None:
        with self.assertRaisesRegex(RuntimeError, "must be relative"):
            run_os_policy.lane_path("/tmp/not-allowed")

    def test_parent_escape_lane_path_fails(self) -> None:
        with self.assertRaisesRegex(RuntimeError, "escapes lane root"):
            run_os_policy.lane_path("../README.md")

    def test_guest_repeat_must_prove_determinism(self) -> None:
        data = valid_manifest()
        data["cases"][0]["guest_contract"]["repeat_runs"] = 1
        with self.assertRaisesRegex(RuntimeError, "at least 2"):
            self.load(data)

    def test_guest_exit_must_match_debug_exit_protocol(self) -> None:
        data = valid_manifest()
        data["cases"][0]["guest_contract"]["expected_exit"] = 0
        with self.assertRaisesRegex(RuntimeError, "debug_exit_value"):
            self.load(data)

    def test_missing_guest_harness_field_fails(self) -> None:
        data = valid_manifest()
        del data["guest_harness"]["boot_source"]
        with self.assertRaisesRegex(RuntimeError, "missing fields"):
            self.load(data)

    def test_guest_scalar_sse2_must_be_boolean(self) -> None:
        data = valid_manifest()
        data["cases"][0]["guest_contract"]["scalar_sse2"] = "yes"
        with self.assertRaisesRegex(RuntimeError, "scalar_sse2"):
            self.load(data)

    def test_guest_scalar_sse2_selects_clang_mode(self) -> None:
        command = run_os_policy.guest_compile_command(
            "clang",
            compiler=Path("/tmp/fisics"),
            source=Path("/tmp/probe.c"),
            output=Path("/tmp/probe.o"),
            target="x86_64-unknown-none",
            scalar_sse2=True,
        )
        self.assertIn("-msse2", command)
        self.assertNotIn("-mno-sse", command)

    def test_guest_default_keeps_sse_disabled(self) -> None:
        command = run_os_policy.guest_compile_command(
            "clang",
            compiler=Path("/tmp/fisics"),
            source=Path("/tmp/probe.c"),
            output=Path("/tmp/probe.o"),
            target="x86_64-unknown-none",
        )
        self.assertIn("-mno-sse", command)
        self.assertNotIn("-msse2", command)

    def test_continue_on_failure_records_later_cases(self) -> None:
        data = valid_manifest()
        second = deepcopy(data["cases"][0])
        second["id"] = "case_b"
        data["cases"].append(second)
        with tempfile.TemporaryDirectory(prefix="osp-continue-") as root:
            build_root = Path(root)
            argv = [
                "run_os_policy.py",
                "--tier",
                "object",
                "--continue-on-failure",
                "--build-root",
                str(build_root),
            ]
            with (
                patch.object(sys, "argv", argv),
                patch.object(run_os_policy, "load_manifest", return_value=data),
                patch.object(
                    run_os_policy,
                    "llvm_tool_paths",
                    return_value=(Path("/tmp/readobj"), Path("/tmp/objdump"), "v"),
                ),
                patch.object(run_os_policy, "version_line", return_value="v"),
                patch.object(run_os_policy, "sha256", return_value="digest"),
                patch.object(
                    run_os_policy,
                    "run_object_case",
                    side_effect=[RuntimeError("first failed"), {"status": "pass"}],
                ),
                redirect_stdout(io.StringIO()),
                redirect_stderr(io.StringIO()),
            ):
                self.assertEqual(run_os_policy.main(), 1)
            report = json.loads(
                (build_root / "latest-object.json").read_text(encoding="utf-8")
            )
        self.assertEqual(report["summary"]["cases_passed"], 1)
        self.assertEqual(report["summary"]["cases_failed"], 1)
        self.assertEqual(report["cases"][0]["status"], "fail")
        self.assertEqual(report["cases"][1]["status"], "pass")


if __name__ == "__main__":
    unittest.main()
