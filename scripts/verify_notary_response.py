#!/usr/bin/env python3
"""Fail closed unless an Apple notarytool response is accepted."""

from __future__ import annotations

import json
import sys
from pathlib import Path


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: verify_notary_response.py <response.json>", file=sys.stderr)
        return 2

    path = Path(sys.argv[1])
    try:
        response = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        print(f"ERROR: cannot read notary response {path}: {exc}", file=sys.stderr)
        return 2

    status = response.get("status")
    if status != "Accepted":
        job_id = response.get("id", "unknown")
        print(f"ERROR: notarization {job_id} finished with status {status!r}", file=sys.stderr)
        return 1

    print(f"notarization accepted: {response.get('id', 'unknown')}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
