#!/usr/bin/env python3
"""Run fixed release checks in an owned writable copy of frozen inputs.

The Registry's outer process sandbox enforces the write boundary. This helper
preserves the ordinary compiler graph, including tests with relative outputs.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

ALLOWED = {'release-contract', 'integration-cli-metadata', 'dev-smoke',
           'os-policy-contract', 'release-root-contract-test',
           'preparation-producer-contract-test', 'release-artifact-disposable',
           'release-notarize'}


def checked_output(path, source):
    path = Path(path)
    if not path.is_absolute() or '..' in path.parts:
        raise ValueError('output must be an absolute contained path')
    if any(p.is_symlink() for p in (path, *path.parents)):
        raise ValueError('output has a symlink component')
    if path == source or path.is_relative_to(source) or source.is_relative_to(path):
        raise ValueError('output must be disjoint from source')
    if path.exists():
        raise ValueError('output already exists')
    return path


def inputs(source):
    names = subprocess.check_output(['git', '-C', str(source), 'ls-files', '-z',
                                    '--cached', '--others', '--exclude-standard']).split(b'\0')
    rows = []
    for name in sorted(set(names) - {b''}):
        relative = Path(os.fsdecode(name))
        if relative.is_absolute() or '..' in relative.parts or '.git' in relative.parts:
            raise ValueError('unsafe source input')
        path = source / relative
        if any(p.is_symlink() for p in (path, *path.parents)) or not path.is_file():
            raise ValueError('source inputs must be ordinary files')
        rows.append((path, relative))
    if not rows:
        raise ValueError('empty source input set')
    return rows


def prepare(source, root):
    source = Path(source).resolve()
    destination = checked_output(Path(root) / 'producer-workspace/fisiCs', source)
    rows = inputs(source)
    destination.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run(['git', 'clone', '--quiet', '--no-checkout', '--shared', '--',
                    str(source), str(destination)], check=True)
    subprocess.run(['git', '-C', str(destination), 'remote', 'remove', 'origin'], check=True)
    manifest = []
    for path, relative in rows:
        target = destination / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, target)
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        if hashlib.sha256(target.read_bytes()).hexdigest() != digest:
            raise ValueError('copied source input changed')
        manifest.append({'path': str(relative), 'sha256': digest})
    subprocess.run(['git', '-C', str(destination), 'read-tree', 'HEAD'], check=True)
    (Path(root) / 'producer-copy.json').write_text(json.dumps(manifest, indent=2) + '\n')
    return destination


def export_package(source, output):
    output = checked_output(output, source.resolve())
    rows = list(source.rglob('*'))
    if any(p.is_symlink() or not (p.is_dir() or p.is_file()) for p in rows):
        raise ValueError('package contains a non-ordinary entry')
    output.parent.mkdir(parents=True, exist_ok=True)
    shutil.copytree(source, output)
    for p in rows:
        if p.is_file() and hashlib.sha256(p.read_bytes()).digest() != hashlib.sha256((output / p.relative_to(source)).read_bytes()).digest():
            raise ValueError('exported package changed')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('root', 'make', 'compiler', 'release-root'):
        parser.add_argument('--' + name, required=True)
    parser.add_argument('goals', nargs=argparse.REMAINDER)
    args = parser.parse_args()
    goals = args.goals[1:] if args.goals[:1] == ['--'] else args.goals
    if not goals or set(goals) - ALLOWED:
        parser.error('unsupported contained preparation goal')
    authentication = goals == ['release-notarize']
    if 'release-notarize' in goals and not authentication:
        parser.error('authentication cannot mix with preparation goals')
    source = Path(__file__).resolve().parents[1]
    root = Path(args.root)
    if not root.is_absolute() or '..' in root.parts:
        parser.error('preparation root must be absolute')
    package = 'release-artifact-disposable' in goals
    if package or authentication:
        checked_output(Path(args.release_root), source)
    destination = prepare(source, root)
    local_release = Path(args.release_root) if authentication else Path('build/release-preparation/package')
    command = [args.make, '-j2', '-C', str(destination), 'PREPARATION_ROOT=',
               'CC=' + args.compiler, 'RELEASE_ROOT=' + str(local_release), *goals]
    status = subprocess.call(command, env={**os.environ, 'FISICS_MAX_PROCS': '0'})
    if status == 0 and package:
        export_package(destination / local_release, Path(args.release_root))
    return status


if __name__ == '__main__':
    raise SystemExit(main())
