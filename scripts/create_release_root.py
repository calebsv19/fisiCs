#!/usr/bin/env python3
"""Create one new, repository-contained release job root."""
from pathlib import Path
import sys


def create_root(value: str, base: Path) -> Path:
    path = Path(value)
    if path.is_absolute() or len(path.parts) < 3 or path.parts[0] != "build" or ".." in path.parts:
        raise ValueError("release root must be a job-scoped path beneath build")
    current = base
    for part in path.parts:
        current = current / part
        if current.is_symlink():
            raise ValueError("release root has a symlink component")
    if current.exists():
        raise ValueError("release root already exists")
    current.mkdir(parents=True, exist_ok=False)
    return current


if __name__ == "__main__":
    try:
        create_root(sys.argv[1], Path.cwd())
    except (ValueError, OSError) as exc:
        raise SystemExit(str(exc))
