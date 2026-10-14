#!/usr/bin/env python3
"""Stable CLI ledger checks: growth, TU teardown, ordering and no-link refusal."""
import argparse, json, subprocess, tempfile
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]

def check(compiler, baseline, out):
    manifest = json.loads((ROOT / 'tests/final/meta/14-runtime-surface-cross-tu-index.json').read_text())
    rows = []
    for case in manifest['tests']:
        fixtures = out / case['id']
        fixtures.mkdir()
        (fixtures / 'padding.h').write_text(''.join((f'extern int ledger_padding_{n};\n' for n in range(256))))
        inputs = []
        for n in case['inputs']:
            src = ROOT / 'tests/final' / n
            dst = fixtures / src.name
            dst.write_text('#include "padding.h"\n' + src.read_text())
            inputs.append(dst)
        for reverse in [False, True]:
            pair = []
            for label, bin in ([('baseline', baseline)] if baseline else []) + [('candidate', compiler)]:
                run = fixtures / (label + ('-reverse' if reverse else ''))
                run.mkdir()
                marker = run / 'linked'
                linker = run / 'linker'
                linker.write_text('#!/bin/sh\ntouch "' + str(marker) + '"\nexit 99\n')
                linker.chmod(448)
                ordered = list(reversed(inputs)) if reverse else inputs
                cmd = [str(bin), '-std=c99', '--emit-diags-json', str(run / 'diags.json'), *[str(x) for x in ordered], '-o', str(run / 'app')]
                if case['expect_compile_exit']:
                    cmd.append('--linker=' + str(linker))
                result = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, timeout=20)
                (run / 'compile.log').write_text(result.stdout + result.stderr)
                assert result.returncode == case['expect_compile_exit'], (case['id'], result.stdout, result.stderr)
                diag = json.loads((run / 'diags.json').read_text())
                record = {'label': label, 'command': cmd, 'exit': result.returncode, 'diags': diag}
                if result.returncode:
                    assert not marker.exists() and (not (run / 'app').exists())
                    ds = diag['diagnostics']
                    assert len(ds) == 1 and ds[0]['code'] == 2000 and ('Conflicting types' in ds[0]['message'])
                    if not reverse:
                        expected_prior = {'array_late': 1, 'prototype_late': 1, 'extern_definition_late': 1, 'first_conflict': 0, 'fnptr_late': 1}
                        for suffix, index in expected_prior.items():
                            if case['id'] == '14__cross_tu_index_' + suffix:
                                assert str(inputs[index]) in ds[0]['hint'], ds[0]
                        if case['id'].endswith('line_provenance'):
                            assert ds[0]['file'] == 'late_virtual.c' and ds[0]['line'] == 91 and ('11:' in ds[0]['hint']), ds[0]
                else:
                    z = subprocess.run([str(run / 'app')], capture_output=True, text=True, timeout=3)
                    assert z.returncode == 0
                    record['stdout'] = z.stdout
                pair.append(record)
            if baseline:
                assert pair[0]['diags'] == pair[1]['diags'] and pair[0].get('stdout') == pair[1].get('stdout'), case['id']
            rows.append({'id': case['id'], 'reverse': reverse, 'runs': pair})
    (out / 'results.json').write_text(json.dumps({'status': 'PASS', 'cases': len(rows), 'results': rows}, indent=2))
    print(f'cross-TU ledger CLI contract: {len(rows)} PASS')

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--compiler', type=Path, required=True)
    p.add_argument('--baseline', type=Path)
    p.add_argument('--out', type=Path)
    a = p.parse_args()
    compiler = a.compiler.resolve()
    baseline = a.baseline.resolve() if a.baseline else None
    if a.out:
        a.out.mkdir(parents=True, exist_ok=False)
        check(compiler, baseline, a.out.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix='fisics-ledger-contract-') as tmp:
            check(compiler, baseline, Path(tmp))
if __name__ == '__main__':
    main()
