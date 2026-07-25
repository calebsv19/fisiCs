#!/usr/bin/env python3
"""Focused source contract tests for the fisiCs authenticated CLI adapter."""

from __future__ import annotations

import importlib.util
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "finalize_authenticated_cli_release",
    ROOT / "scripts/finalize_authenticated_cli_release.py",
)
assert SPEC and SPEC.loader
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class ReleaseAdapterContractTests(unittest.TestCase):
    def test_version_is_a_compiler_object_dependency(self):
        makefile = (ROOT / "Makefile").read_text(encoding="utf-8")
        for rule in (
            "$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(RELEASE_VERSION_FILE)",
            "$(BUILD_DIR)/core_base/%.o: $(CORE_BASE_DIR)/src/%.c $(RELEASE_VERSION_FILE)",
            "$(BUILD_DIR)/core_io/%.o: $(CORE_IO_DIR)/src/%.c $(RELEASE_VERSION_FILE)",
            "$(BUILD_DIR)/core_data/%.o: $(CORE_DATA_DIR)/src/%.c $(RELEASE_VERSION_FILE)",
            "$(BUILD_DIR)/core_pack/%.o: $(CORE_PACK_DIR)/src/%.c $(RELEASE_VERSION_FILE)",
        ):
            self.assertIn(rule, makefile)
        release_build = makefile.split("release-build:", 1)[1].split("\n\n", 1)[0]
        self.assertIn("release-build-clean", release_build)
        self.assertNotIn("$(MAKE) clean", release_build)

    def test_authenticated_manifest_is_format_specific_and_exact(self):
        text = MODULE.manifest_text(
            artifact=Path("fisiCs-1.2.3-macOS-arm64-stable.tar.gz"),
            artifact_sha="a" * 64, fmt="tar.gz", version="1.2.3",
            platform="macOS", arch="arm64", channel="stable",
            identity="Developer ID Application: Fixture (TEAMID1234)",
            team_id="TEAMID1234", submission_id="notary-001",
        )
        fields = MODULE.parse_manifest_text(text)
        self.assertEqual(fields["format"], "tar.gz")
        self.assertEqual(fields["sha256"], "a" * 64)
        self.assertEqual(fields["signed"], "1")
        self.assertEqual(fields["notarized"], "1")
        self.assertEqual(fields["stapling"], "not_applicable")

    def test_checksum_requires_exact_artifact_basename(self):
        with tempfile.TemporaryDirectory(prefix="fisics-release-contract-") as temp:
            root = Path(temp)
            artifact = root / "fisiCs-1.2.3.zip"; artifact.write_bytes(b"artifact")
            sidecar = root / "artifact.sha256"
            digest = MODULE.sha256(artifact)
            sidecar.write_text(f"{digest}  wrong.zip\n", encoding="utf-8")
            with self.assertRaises(SystemExit):
                MODULE.read_checksum(sidecar, artifact)


if __name__ == "__main__":
    unittest.main()
