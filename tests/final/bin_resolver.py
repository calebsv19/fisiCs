import os
import shutil
import tempfile
from pathlib import Path


class StagedBinary:
    def __init__(self, requested_path, resolved_path, staged_path, used_fallback, tempdir):
        self.requested_path = str(requested_path)
        self.resolved_path = Path(resolved_path)
        self.staged_path = Path(staged_path)
        self.used_fallback = bool(used_fallback)
        self._tempdir = tempdir

    def cleanup(self):
        self._tempdir.cleanup()


def _find_numbered_sibling(repo_root, binary_name):
    fallback_prefix = f"{binary_name} "
    fallback_candidates = sorted(
        [
            candidate
            for candidate in Path(repo_root).iterdir()
            if candidate.is_file()
            and candidate.name.startswith(fallback_prefix)
            and os.access(candidate, os.X_OK)
        ],
        key=lambda candidate: (candidate.stat().st_mtime, len(candidate.name)),
        reverse=True,
    )
    if fallback_candidates:
        return fallback_candidates[0].resolve()
    return None


def resolve_bin_path(raw_path, repo_root):
    repo_root = Path(repo_root)
    path = Path(raw_path)
    if path.is_absolute():
        return path.resolve(), False

    requested = (repo_root / path).resolve()
    if requested.exists():
        return requested, False

    fallback = _find_numbered_sibling(repo_root, path.name)
    if fallback is not None:
        return fallback, True

    return requested, False


def stage_bin_copy(raw_path, repo_root, *, prefix):
    resolved_path, used_fallback = resolve_bin_path(raw_path, repo_root)
    if not resolved_path.exists():
        raise FileNotFoundError(f"fisics binary not found at {resolved_path}")

    tempdir = tempfile.TemporaryDirectory(prefix=prefix)
    staged_name = resolved_path.name.replace(" ", "_")
    staged_path = Path(tempdir.name) / staged_name
    shutil.copy2(resolved_path, staged_path)
    staged_path.chmod(staged_path.stat().st_mode | 0o111)

    return StagedBinary(
        requested_path=raw_path,
        resolved_path=resolved_path,
        staged_path=staged_path,
        used_fallback=used_fallback,
        tempdir=tempdir,
    )
