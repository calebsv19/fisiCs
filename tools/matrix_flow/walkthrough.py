#!/usr/bin/env python3
"""Exercise ordinary C, physical acceptance, wrong-kind rejection and unknown reporting."""
import argparse
import json
import os
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('compiler-root', 'compiler', 'out'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    root, compiler, out = args.compiler_root.resolve(), args.compiler.resolve(), args.out.resolve()
    out.mkdir(parents=True, exist_ok=False)
    fixture = root / 'tests/fixtures/physical_semantics'
    prefix = '#include "authoring.h"\n'
    cases = [
        ('valid', 'ENERGY double add(double a ENERGY,double b ENERGY){return a+b;}', 0, 'quantity.energy'),
        ('wrong-kind', 'ENERGY double wrong(double a TORQUE){return a;}', 1, 'expected quantity.energy'),
        ('unknown', 'ENERGY double raw(double x){return x;}', 0, 'actual=<unknown> state=unknown'),
    ]
    rows = []
    for name, source, expected, text in cases:
        path = out / (name + '.c')
        path.write_text(prefix + source + '\n')
        command = [str(compiler), '-I' + str(root / 'include'), '-I' + str(fixture),
                   '--overlay=physics-semantics', '--physical-semantics-policy', str(fixture / 'policy.json'),
                   '--dump-sema', str(path)]
        run = subprocess.run(command, cwd=out, text=True, capture_output=True)
        output = run.stdout + run.stderr
        (out / (name + '.log')).write_text(output)
        rows.append(dict(id=name, argv=command, exit=run.returncode,
                         passed=run.returncode == expected and text in output))
    source = out / 'ordinary.c'
    source.write_text('int main(void){return 6*7==42?0:1;}\n')
    executable = out / 'ordinary'
    command = [str(compiler), str(source), '-o', str(executable)]
    run = subprocess.run(command, cwd=out, text=True, capture_output=True)
    status = subprocess.run([str(executable)], cwd=out).returncode if run.returncode == 0 else None
    (out / 'ordinary.log').write_text(run.stdout + run.stderr)
    rows.append(dict(id='ordinary', argv=command, exit=run.returncode, runtime_exit=status,
                     passed=run.returncode == 0 and status == 0))
    result = dict(passed=all(row['passed'] for row in rows), cases=rows,
                  scope='Static source compatibility and ordinary runtime smoke; unknown is visible, not proved.')
    (out / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))
    return 0 if result['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
