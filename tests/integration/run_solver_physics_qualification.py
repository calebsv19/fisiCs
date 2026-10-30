#!/usr/bin/env python3
"""Bounded solver-derived qualification; gaps never count as semantic passes.

--baseline exits zero for a complete inventory, including reported checking gaps.
Without it, any checking gap fails qualification. Commands, logs and source hashes
are retained in a new caller-selected directory. No customer source is needed.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import shutil
import subprocess
import time

ROOT = Path(__file__).resolve().parents[2]
FIXTURES = ROOT / 'tests/fixtures/solver_physics'
# Expectations are physical/analytical requirements, independent of observations.
CASES = [
    ('translation', 'translation.c', [], [1.25, 2.5], None),
    ('translation_bad_update', 'translation.c', ['BAD_UPDATE'], None, 'units'),
    ('translation_raw_field', 'translation.c', ['BAD_RAW_FIELD'], None, 'units'),
    ('translation_typed_state', 'translation.c', ['TYPED_STATE'], [1.25, 2.5], None),
    ('translation_typed_bad_field', 'translation.c', ['TYPED_STATE', 'BAD_RAW_FIELD'], None, 'units'),
    ('energy_bad_assignment', 'energy.c', ['BAD_ENERGY_ASSIGN'], None, 'units'),
    ('energy', 'energy.c', [], [25], None),
    ('energy_missing_factor', 'energy.c', ['BAD_ENERGY'], None, 'units'),
    ('energy_math_boundary', 'energy.c', ['BAD_MATH_BOUNDARY'], None, 'units'),
    ('structure', 'structure.c', [], [120000, 120, 106000], None),
    ('structure_bad_stress', 'structure.c', ['BAD_STRESS'], None, 'units'),
    ('structure_matrix_slot', 'structure.c', ['BAD_MATRIX_SLOT'], None, 'units'),
]
for mode in ('same_tu', 'forward', 'reverse'):
    for suffix, definitions, expected in [
        ('valid', [], [6]),
        ('argument_dim', ['BAD_ARGUMENT_DIM'], None),
        ('argument_unit', ['BAD_ARGUMENT_UNIT'], None),
        ('return_consumer', ['BAD_RETURN_CONSUMER'], None),
        ('return_assignment', ['BAD_RETURN_ASSIGN'], None),
    ]:
        CASES.append((f'interface_{mode}_{suffix}', 'interface.c',
                      definitions + (['SAME_TU'] if mode == 'same_tu' else []),
                      expected, 'units' if expected is None else None))


def require(value, message):
    if not value:
        raise RuntimeError(message)


def validate_runtime(code, output, expected, artifact_exists):
    require(artifact_exists, 'missing executable')
    require(code == 0, f'runtime exit {code}, expected 0')
    actual = [float(x) for x in output.split()]
    require(len(actual) == len(expected), 'runtime output shape mismatch')
    require(all(math.isfinite(a) and math.isclose(a, b, rel_tol=1e-10, abs_tol=1e-9)
                for a, b in zip(actual, expected)), f'runtime mismatch: {actual} != {expected}')


def rejection_state(code, output, executable_exists, needle):
    if code == 0:
        require(executable_exists, 'successful compile produced no executable')
        return 'checking_gap'
    require(code == 1, f'unexpected compiler exit {code}; expected 0 or 1')
    errors = [line.lower() for line in output.splitlines() if line.startswith('Error at ')]
    require(any(needle in line for line in errors), 'rejection lacks owning physical diagnostic')
    require(not executable_exists, 'rejected compile left an executable')
    return 'rejected_as_required'


def self_check():
    checks = [
        lambda: validate_runtime(1, '6', [6], True),
        lambda: validate_runtime(0, '7', [6], True),
        lambda: validate_runtime(0, '6', [6], False),
        lambda: rejection_state(2, 'units', False, 'units'),
        lambda: rejection_state(1, 'parser error', False, 'units'),
        lambda: rejection_state(0, '', False, 'units'),
        lambda: rejection_state(1, 'Error at (1:1): units', True, 'units'),
        lambda: rejection_state(1, 'Error at (1:1): syntax error\nUnits annotations:', False, 'units'),
    ]
    for i, check in enumerate(checks):
        try:
            check()
        except (RuntimeError, ValueError):
            continue
        raise RuntimeError(f'validator accepted injected failure {i}')
    validate_runtime(0, '6', [6], True)
    require(rejection_state(1, 'Error at (1:1): units mismatch', False, 'units') == 'rejected_as_required',
            'positive rejection control failed')
    require(rejection_state(0, '', True, 'units') == 'checking_gap', 'gap hidden')
    return len(checks)


def run(command, directory, label):
    start = time.monotonic()
    q = subprocess.run(command, cwd=ROOT, env={**os.environ, 'FISICS_MAX_PROCS': '0'},
                       text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=60)
    (directory / f'{label}.log').write_text(q.stdout)
    return {'command': command, 'exit': q.returncode,
            'seconds': time.monotonic()-start, 'log': f'{label}.log'}, q.stdout


def execute_case(case, compiler, clang, output):
    name, source, definitions, expected, needle = case
    if needle:
        needle = ('units argument mismatch' if 'BAD_ARGUMENT_DIM' in definitions
                  else 'implicit concrete unit conversion in argument' if 'BAD_ARGUMENT_UNIT' in definitions
                  else 'units addition mismatch' if 'BAD_UPDATE' in definitions or 'BAD_STRESS' in definitions
                  else 'units assignment mismatch')
    directory = output / name
    directory.mkdir()
    sources = [str(FIXTURES / source)]
    if name.startswith('interface_') and 'SAME_TU' not in definitions:
        sources.append(str(FIXTURES / 'helper.c'))
        if '_reverse_' in name:
            sources.reverse()
    common = ['-I'+str(ROOT/'include'), '-I'+str(FIXTURES)] + ['-D'+d for d in definitions]
    result = {'id': name, 'desired': 'reject' if needle else 'compile_and_run',
              'expected_values': expected, 'definitions': definitions, 'sources': sources,
              'tolerance': {'relative': 1e-10, 'absolute': 1e-9}, 'commands': []}
    try:
        # Mutants remain ordinary legal C. Their Clang runs are control evidence,
        # not physics-correct numerical expectations.
        cbin = directory/'clang'
        command = [clang, '-std=c99', '-Wall', '-Wextra', '-Werror', '-Wno-unused-variable',
                   '-ffp-contract=off', *common, *sources, '-lm', '-o', str(cbin)]
        rec, log = run(command, directory, 'clang_compile'); result['commands'].append(rec)
        require(rec['exit'] == 0 and cbin.is_file(), 'Clang control failed')
        rec, log = run([str(cbin)], directory, 'clang_run'); result['commands'].append(rec)
        require(rec['exit'] == 0, 'Clang control execution failed')
        if expected is not None:
            validate_runtime(rec['exit'], log, expected, cbin.is_file())
        fbin = directory/'fisics'
        command = [compiler, '--overlay=physics-units', '--dump-sema', '-ffp-contract=off',
                   *common, *sources, '-lm', '-o', str(fbin)]
        rec, log = run(command, directory, 'fisics_compile'); result['commands'].append(rec)
        if needle:
            result['state'] = rejection_state(rec['exit'], log, fbin.is_file(), needle)
            if result['state'] == 'checking_gap':
                rec, log = run([str(fbin)], directory, 'fisics_gap_run'); result['commands'].append(rec)
                require(rec['exit'] == 0, 'accepted mutant failed at runtime')
        else:
            require(rec['exit'] == 0, 'valid fisiCs compile failed')
            rec, log = run([str(fbin)], directory, 'fisics_run'); result['commands'].append(rec)
            validate_runtime(rec['exit'], log, expected, fbin.is_file())
            result['state'] = 'runtime_qualified'
    except (RuntimeError, ValueError, subprocess.TimeoutExpired, OSError) as exc:
        result['state'] = 'execution_failure'
        result['error'] = str(exc)
    return result


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--compiler', required=True)
    p.add_argument('--clang', default='clang')
    p.add_argument('--out', required=True, type=Path)
    p.add_argument('--baseline', action='store_true')
    args = p.parse_args()
    compiler = str(Path(args.compiler).resolve())
    clang = shutil.which(args.clang)
    require(Path(compiler).is_file() and clang, 'missing compiler')
    output = args.out.resolve()
    # Fresh output prevents stale artifacts from satisfying compile success.
    output.mkdir(parents=True, exist_ok=False)
    controls = self_check()
    results = [execute_case(case, compiler, clang, output) for case in CASES]
    gaps = sum(r['state'] == 'checking_gap' for r in results)
    failures = sum(r['state'] == 'execution_failure' for r in results)
    report = {'schema': 'fisiCs.solver_physics_qualification.v1',
              'compiler': compiler, 'compiler_sha256': hashlib.sha256(Path(compiler).read_bytes()).hexdigest(),
              'fixture_sha256': {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                                 for p in sorted(FIXTURES.iterdir()) if p.suffix in ('.c','.h')},
              'validator_failure_controls': controls, 'inventory_complete': failures == 0,
              'qualification_passed': failures == 0 and gaps == 0,
              'checking_gaps': gaps, 'execution_failures': failures, 'results': results}
    (output/'results.json').write_text(json.dumps(report, indent=2)+'\n')
    for r in results:
        print(f"{r['id']}: {r['state']}" + (': '+r['error'] if 'error' in r else ''))
    print(f'Inventory complete={failures == 0}; qualification passed={report["qualification_passed"]}; gaps={gaps}')
    return 1 if failures or (gaps and not args.baseline) else 0


if __name__ == '__main__':
    raise SystemExit(main())
