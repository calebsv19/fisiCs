#!/usr/bin/env python3
"""Read-only external Fire canary, with compiler-owned outputs and exact FP checks.

Run with PYTHONDONTWRITEBYTECODE=1; requires a clean, pinned GrowthSim checkout.
No installed app, published checkpoint, or stream identity is changed.
"""
import argparse
import base64
import copy
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys

PIN = '4890c632fb45d03e314b5be4ca7612b2716e01ff'
SOURCES = (
    'tools/growth_sim_fire_source_worker.c',
    'src/core/rules/growth_sim_fire_rules.c',
    'src/core/rules/growth_sim_rule_validation.c',
    'src/core/world/growth_sim_fire_world.c',
    'src/core/world/growth_sim_fire_wind.c',
    'src/core/space/growth_sim_simulation_space_descriptor.c',
    'src/core/space/growth_sim_simulation_space_bridge.c',
)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def run(cmd, env=None):
    return subprocess.run(list(map(str, cmd)), check=True, capture_output=True,
                          env=env, timeout=120)


def git(root, *args):
    return run(['git', '-C', root, *args]).stdout.decode().strip()


def raw_state(bundle):
    return base64.b64decode(bundle['checkpoint']['native_state_b64'])


def numeric_bytes(value):
    # JSON equality alone would conflate -0.0 and +0.0.
    if isinstance(value, dict):
        return b''.join(numeric_bytes(value[k]) for k in sorted(value))
    if isinstance(value, list):
        return b''.join(numeric_bytes(v) for v in value)
    if isinstance(value, (float, int)):
        return struct.pack('<d', value)
    return str(value).encode()


def field_diff(a, b, decode):
    aa, bb = decode(a), decode(b)
    result = {}
    for name, values in aa['fields'].items():
        mismatches = [(i, struct.pack('<f', x).hex(), struct.pack('<f', y).hex())
                      for i, (x, y) in enumerate(zip(values, bb['fields'][name]))
                      if struct.pack('<f', x) != struct.pack('<f', y)]
        result[name] = {'nodes': len(mismatches), 'first_le_bytes': mismatches[:1]}
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--growth-root', type=Path, required=True)
    parser.add_argument('--fisics', type=Path, required=True)
    parser.add_argument('--clang', default='clang')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    root, compiler, out = args.growth_root.resolve(), args.fisics.resolve(), args.output.resolve()
    if out == root or root in out.parents:
        parser.error('output must be outside the read-only GrowthSim checkout')
    if sys.dont_write_bytecode is False:
        parser.error('run with PYTHONDONTWRITEBYTECODE=1 or python3 -B')
    if git(root, 'rev-parse', 'HEAD') != PIN or git(root, 'status', '--porcelain'):
        parser.error('GrowthSim must be clean at the pinned source revision')
    out.mkdir(parents=True, exist_ok=True)
    sys.path.insert(0, str(root / 'scripts'))
    from fire_source.contract import strict_load, state_decode
    from fire_source.producer import initial, advance, native

    shared = root / 'third_party/codework_shared/core'
    libraries = [shared / name / 'build' / ('lib' + name + '.a')
                 for name in ('core_space', 'core_units', 'core_base')]
    # Freeze existing link inputs in this task's directory; never build them.
    frozen = []
    for lib in libraries:
        dest = out / lib.name
        shutil.copyfile(lib, dest)
        frozen.append(dest)
    includes = ['-I' + str(root / 'include'), '-I' + str(root / 'src'),
                '-I' + str(Path(__file__).resolve().parents[3] / 'include')]
    includes += ['-I' + str(shared / name / 'include')
                 for name in ('core_space', 'core_units', 'core_base')]
    source_inputs = [root / name for name in SOURCES]
    inputs = source_inputs + libraries + list((root / 'include').rglob('*.h'))
    inputs += [root / 'tests/fixtures/fire_source/config.json', root / 'Makefile']
    inputs += list((root / 'make').glob('*.mk'))
    inputs += list((root / 'scripts/fire_source').glob('*.py'))
    for name in ('core_space', 'core_units', 'core_base'):
        inputs += list((shared / name / 'include').rglob('*.h'))
    hashes = {str(p.relative_to(root)): sha(p.read_bytes()) for p in inputs}
    report = {'source_commit': PIN, 'input_sha256': hashes,
              'fisics_sha256': sha(compiler.read_bytes()),
              'versions': {c: run([c, '--version']).stdout.decode()
                           for c in (str(compiler), args.clang)},
              'policy': 'contraction off, source precision, default round-to-nearest; same libSystem/libm and shared libraries',
              'builds': {}, 'cases': []}
    env = os.environ.copy()
    for key in ('FISICS_LLVM_OPT_LEVEL', 'FISICS_LLVM_GLOBAL_ISEL', 'FISICS_LLVM_FAST_ISEL',
                'FISICS_SKIP_GLOBALDCE', 'FISICS_OVERLAY', 'FISICS_COMPAT', 'FISICS_EXTENSIONS'):
        env.pop(key, None)
    env['FISICS_VERIFY_IR'] = '1'
    workers = {}
    for name, tool, level, policy in [('clang-on', args.clang, 2, 'on')] + [
            (f'{label}-off-O{level}', tool, level, 'off')
            for label, tool in [('clang', args.clang), ('fisics', str(compiler))]
            for level in (0, 2, 3)]:
        cmd = [tool]
        build_env = dict(env)
        if tool == str(compiler):
            cmd += ['--overlay=physics-units', '-ffp-contract=' + policy]
            build_env['FISICS_LLVM_OPT_LEVEL'] = str(level)
        else:
            cmd += ['-std=c11', '-O' + str(level), '-fno-fast-math', '-ffp-contract=' + policy]
        worker = out / name
        cmd += includes + list(map(str, source_inputs + frozen)) + ['-lm', '-o', str(worker)]
        result = run(cmd, env=build_env)
        (out / (name + '.compile.log')).write_bytes(result.stdout + result.stderr)
        report['builds'][name] = {'command': cmd, 'optimization': level,
                                  'worker_sha256': sha(worker.read_bytes()),
                                  'linkage': run(['otool', '-L', worker]).stdout.decode() if sys.platform == 'darwin' else ''}
        workers[name] = worker
    cfg = strict_load(root / 'tests/fixtures/fire_source/config.json')
    for width, height, seed in [(8, 8, 42), (2, 2, 0), (8, 8, 1),
                                (13, 9, 1024), (17, 16, 0xdeadbeef), (9, 13, 2**32-1)]:
        c = copy.deepcopy(cfg)
        c.update(width=width, height=height, seed=seed)
        bundles = {k: initial(c, w, 'init') for k, w in workers.items()}
        reference = raw_state(bundles['clang-off-O2'])
        row = {'width': width, 'height': height, 'seed': seed,
               'seed_state_sha256': sha(reference), 'matched_workers': {},
               'unmatched_policy_seed': field_diff(raw_state(bundles['clang-on']), reference, state_decode)}
        same_state, same_obs = native(workers['clang-off-O2'], 'step', reference, 100)
        on_state, on_obs = native(workers['clang-on'], 'step', reference, 100)
        row['unmatched_policy_same_input_100'] = field_diff(on_state, same_state, state_decode)
        expected_evolutions = []
        parent = bundles['clang-off-O2']
        for i in (1, 2):
            parent = advance(parent, workers['clang-off-O2'], 100, 'step-' + str(i))
            expected_evolutions.append(parent)
        for k, worker in workers.items():
            if k == 'clang-on':
                continue
            seeded = raw_state(bundles[k])
            assert seeded == reference, (k, width, height, seed, 'seed', field_diff(seeded, reference, state_decode))
            state, obs = native(worker, 'step', reference, 100)
            assert state == same_state and numeric_bytes(obs) == numeric_bytes(same_obs), (k, 'identical input evolution')
            one, _ = native(worker, 'step', reference, 1)
            split, _ = native(worker, 'step', one, 99)
            assert split == same_state, (k, '1+99 continuation')
            parent, receipts = bundles[k], []
            for i, expected in enumerate(expected_evolutions, 1):
                parent = advance(parent, worker, 100, 'step-' + str(i))
                assert raw_state(parent) == raw_state(expected), (k, 'state', i)
                for field in ('cells', 'totals', 'diagnostics'):
                    assert numeric_bytes(parent['frame'][field]) == numeric_bytes(expected['frame'][field]), (k, field, i)
                receipts.append({'tick': i * 100, 'state_sha256': sha(raw_state(parent)),
                                 'accounting_sha256': sha(numeric_bytes(parent['frame']['cells']) + numeric_bytes(parent['frame']['totals'])),
                                 'totals': parent['frame']['totals']})
            row['matched_workers'][k] = receipts
        report['cases'].append(row)
        print(f'PASS {width}x{height} seed={seed}: seed, identical-input 100, 1+99, 100+100, accounting; six matched workers', flush=True)
    assert hashes == {str(p.relative_to(root)): sha(p.read_bytes()) for p in inputs}, 'read-only inputs changed during run'
    assert sha(compiler.read_bytes()) == report['fisics_sha256'], 'compiler changed during run'
    assert git(root, 'rev-parse', 'HEAD') == PIN and not git(root, 'status', '--porcelain'), 'source state changed'
    report['status'] = 'exact_parity_passed'
    (out / 'verification.json').write_text(json.dumps(report, indent=2) + '\n')
    print('PASS exact parity; report:', out / 'verification.json')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
