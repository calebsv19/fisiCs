#!/usr/bin/env python3
"""Package only the source inputs needed for the explicit optional developer route."""
import argparse
import gzip
import hashlib
import io
import json
from pathlib import Path
import tarfile

from provenance import native_inputs, require, sha


def package(root, out):
    root, out = root.resolve(), out.resolve()
    require(not out.exists(), 'developer-kit output must be a fresh directory')
    native = native_inputs(root)
    names = set(native)
    names.add('LICENSE')
    for directory, suffixes in [('tools/matrix_flow', {'.c', '.py', '.json', '.md', ''}),
                                ('examples/physics_units', {'.c', '.md'}),
                                ('docs', {'.md'})]:
        for path in (root / directory).iterdir():
            if path.is_file() and path.suffix in suffixes:
                names.add(str(path.relative_to(root)))
    names.update(['tests/fixtures/physical_semantics/catalog.json',
                  'tests/fixtures/physical_semantics/policy.json',
                  'tests/fixtures/physical_semantics/authoring.h'])
    for path in (root / 'third_party/codework_shared').rglob('LICENSE*'):
        if path.is_file():
            names.add(str(path.relative_to(root)))
    data = {}
    for name in sorted(names):
        path = root / name
        require(path.resolve().is_relative_to(root), 'source-kit input escapes compiler root: ' + name)
        data[name] = path.read_bytes()
    files = {name: hashlib.sha256(raw).hexdigest() for name, raw in data.items()}
    identity = hashlib.sha256(json.dumps(files, sort_keys=True).encode()).hexdigest()
    version = (root / 'VERSION').read_text().strip()
    prefix = 'fisiCs-matrix-flow-source-' + identity[:12]
    manifest = dict(schema='fisiCs.experimental.developer_kit.v1', source_version=version,
                    source_identity=identity, files=files, native_inputs=len(native),
                    scope='matching compiler/frontend/adapter source and optional tools; no solver fixture or SDK redistribution')
    data['KIT_MANIFEST.json'] = (json.dumps(manifest, indent=2) + '\n').encode()
    data['SOURCE_KIT.md'] = b'''# Optional physics developer source kit

This is a bounded source kit, not a full contributor checkout or a bundled SDK.
It contains the matching compiler/frontend and experimental matrix-flow tools.
It does not contain the reviewed Structural solver fixture or system headers.
See tools/matrix_flow/README.md for the exact model and remaining trust.

Select LLVM22's bin directory in PATH and install the required Apple SDK and
Python3.10 or later. Run `python3 -B tools/matrix_flow/preflight.py` first.
Run `python3 -B tools/matrix_flow/build_frontend.py --compiler-root "$PWD"
--out /new/absolute/build --with-compiler`, then
`python3 -B tools/matrix_flow/walkthrough.py --compiler-root "$PWD"
--compiler /new/absolute/build/fisics --out /new/absolute/walkthrough`.

The maintained matrix-flow `check` route additionally requires the exact
separately supplied reviewed core_structural fixture, plus its sibling core_base.
Use `make -f tools/matrix_flow/Makefile check COMPILER_ROOT="$PWD"
CORE_ROOT=/reviewed/core_structural OUT=/new/absolute/check`.
Fresh outputs are mandatory. Do not update reviewed hashes from failing inputs.
Ordinary compilation never invokes the qualifier. This kit changes no default
compiler, installs no application and establishes no runtime conservation proof.
'''
    out.mkdir(parents=True, exist_ok=False)
    archive = out / (prefix + '.tar.gz')
    with archive.open('wb') as stream, gzip.GzipFile(filename='', mode='wb', fileobj=stream, mtime=0) as zipped:
        with tarfile.open(fileobj=zipped, mode='w') as tar:
            for name, raw in sorted(data.items()):
                info = tarfile.TarInfo(prefix + '/' + name)
                info.size, info.mode, info.mtime = len(raw), 0o644, 0
                tar.addfile(info, io.BytesIO(raw))
    require(native == native_inputs(root), 'native inputs changed while packaging developer kit')
    require(all(sha(root / name) == digest for name, digest in files.items()),
            'developer-kit source changed during packaging')
    (out / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    (out / 'README.md').write_text('Optional source kit: extract ' + archive.name +
        ' into a fresh directory and start at SOURCE_KIT.md. Source identity: ' + identity + '\n')
    result = dict(passed=True, archive=archive.name, archive_sha256=sha(archive),
                  source_identity=identity, files=len(files), source_version=version)
    (out / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler-root', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    try:
        package(args.compiler_root, args.out)
        return 0
    except (OSError, ValueError, tarfile.TarError) as error:
        print(str(error))
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
