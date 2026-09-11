#!/usr/bin/env python3
"""Behavior checks for release root containment and single-use semantics."""
import importlib.util
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("release_roots", Path(__file__).resolve().parents[2] / "scripts/create_release_root.py")
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)

class ReleaseRootTests(unittest.TestCase):
    def test_rejects_unsafe_and_reused_roots(self):
        with tempfile.TemporaryDirectory() as temp:
            base = Path(temp)
            for value in ("/tmp/job", "build", "build/release", "build/release/../outside", "source/release/job"):
                with self.subTest(value=value), self.assertRaises(ValueError):
                    m.create_root(value, base)
            root = m.create_root("build/release-preparations/job", base)
            (root / "evidence").write_text("preserve")
            with self.assertRaises(ValueError):
                m.create_root("build/release-preparations/job", base)
            self.assertEqual((root / "evidence").read_text(), "preserve")

    def test_rejects_symlink_ancestors(self):
        with tempfile.TemporaryDirectory() as temp:
            base = Path(temp)
            (base / "outside").mkdir()
            (base / "build").symlink_to(base / "outside", target_is_directory=True)
            with self.assertRaises(ValueError):
                m.create_root("build/release/job", base)
            self.assertEqual(list((base / "outside").iterdir()), [])

if __name__ == "__main__":
    unittest.main()
