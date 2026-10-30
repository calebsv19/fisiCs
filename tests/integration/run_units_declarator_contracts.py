#!/usr/bin/env python3
"""Per-declarator units: exact diagnostics, metadata identity, and C parity."""
import argparse
import json
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'tests/final/cases/15__units_declarator_isolation.c'
EXPECTED = '120.0 202.0 3.0 40.0 200.0 200.0 200.0\n'


def check(compiler, out):
    rows = []
    env = {**os.environ, 'FISICS_MAX_PROCS': '0'}

    def run(label, command, expected):
        proc = subprocess.run([str(x) for x in command], cwd=ROOT, env=env,
                              text=True, capture_output=True, timeout=60)
        (out / (label + '.log')).write_text(proc.stdout + proc.stderr)
        rows.append(dict(label=label, command=[str(x) for x in command],
                         exit=proc.returncode, expected_exit=expected))
        assert proc.returncode == expected, (label, proc.returncode, proc.stderr)
        return proc.stdout

    for variant in range(10):
        diag, exe = out / f'diag-{variant}.json', out / f'candidate-{variant}'
        code = 0 if variant == 0 else 4107 if variant == 3 else 4101 if variant == 9 else 4103
        command = [compiler, '--overlay=physics-units', f'-DFAIL_KIND={variant}',
                   '--emit-diags-json', diag]
        if variant == 0:
            command += ['--emit-units-metadata-json', out / 'units.json']
        run(f'compile-{variant}', command + [SOURCE, '-o', exe], int(bool(code)))
        errors = [d for d in json.loads(diag.read_text())['diagnostics'] if d['severity_name'] == 'error']
        assert len(errors) == int(bool(code)), (variant, errors)
        if code:
            assert errors[0]['code_id'] == code and errors[0]['category_name'] == 'extension', errors
            assert errors[0]['line'] > 0 and errors[0]['column'] > 0 and not exe.exists(), errors
        else:
            assert run('runtime', [exe], 0) == EXPECTED
            exe.unlink()
        control = out / f'clang-{variant}'
        run(f'clang-{variant}', ['clang', '-std=c99', '-pedantic-errors', '-I', ROOT / 'include',
                               f'-DFAIL_KIND={variant}', SOURCE, '-o', control], 0)
        if variant == 0:
            assert run('clang-runtime', [control], 0) == EXPECTED
        control.unlink()

    metadata = json.loads((out / 'units.json').read_text())
    assert metadata['complete'] and metadata['overlay_enabled'], metadata
    attachments = metadata['attachments']
    force = [1, 1, -2, 0, 0, 0, 0, 0]
    moment = [2, 1, -2, 0, 0, 0, 0, 0]
    length = [1, 0, 0, 0, 0, 0, 0, 0]
    expected = {'force': (force, 2), 'moment': (moment, 2),
                'global_force': (force, 1), 'global_moment': (moment, 1),
                'f': (force, 1), 'm': (moment, 1), 'fp': (force, 1), 'mp': (moment, 1),
                'meters': (length, 1), 'feet': (length, 1), 'copied_feet': (length, 1),
                'torque': (moment, 1), 'a': (force, 1), 'b': (moment, 1),
                'rotation_work': (moment, 1)}
    assert len(attachments) == sum(count for _, count in expected.values()), attachments
    assert len({a['binding_id'] for a in attachments}) == len(attachments)
    for name, (dimension, count) in expected.items():
        matches = [a for a in attachments if a['symbol_name'] == name]
        assert len(matches) == count, (name, matches)
        for attachment in matches:
            assert attachment['dimension']['vector'] == dimension, attachment
            assert attachment['source']['start_column'] > 0, attachment
            if name in ('meters', 'feet', 'copied_feet'):
                assert attachment['unit']['name'] == ('meter' if name == 'meters' else 'foot'), attachment
    (out / 'results.json').write_text(json.dumps(dict(passed=True, commands=rows,
        physical_rejections=9, clang_legal_controls=10, metadata_attachments=len(attachments)), indent=2)+'\n')
    print(f'Declarator contracts: PASS ({len(rows)} commands, 9 rejections, {len(attachments)} exact attachments)')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', required=True, type=Path)
    parser.add_argument('--out', required=True, type=Path)
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=False)
    check(args.compiler.resolve(), args.out.resolve())


if __name__ == '__main__':
    main()
