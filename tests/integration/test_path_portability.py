#!/usr/bin/env python3
import re
import subprocess
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
USER_HOME_RE = re.compile(r"/Users/[^/\s]+/")
EXCLUDED_PREFIXES = (
    "tests/final/expect/",
    "third_party/",
)


def tracked_files():
    result = subprocess.run(
        ["git", "ls-files", "-z"],
        cwd=REPO_ROOT,
        check=True,
        stdout=subprocess.PIPE,
    )
    return [Path(raw.decode("utf-8")) for raw in result.stdout.split(b"\0") if raw]


class PathPortabilityTests(unittest.TestCase):
    def test_first_party_text_does_not_embed_mac_user_homes(self):
        violations = []
        for relative_path in tracked_files():
            relative_text = relative_path.as_posix()
            if relative_text.startswith(EXCLUDED_PREFIXES):
                continue
            path = REPO_ROOT / relative_path
            try:
                text = path.read_text(encoding="utf-8")
            except (OSError, UnicodeDecodeError):
                continue
            for line_number, line in enumerate(text.splitlines(), start=1):
                if USER_HOME_RE.search(line):
                    violations.append(f"{relative_text}:{line_number}: {line.strip()}")

        self.assertEqual(
            violations,
            [],
            "first-party tracked files contain checkout-specific macOS user paths:\n"
            + "\n".join(violations),
        )


if __name__ == "__main__":
    unittest.main()
