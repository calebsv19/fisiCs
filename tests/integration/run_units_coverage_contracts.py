#!/usr/bin/env python3
"""P3.2 scope, dependency, loss, deterministic export and publication controls."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'tests/fixtures/solver_physics/coverage_reporting.c'


def check(compiler, out):
    rows = []
    env = {**os.environ, 'FISICS_MAX_PROCS': '0'}

    def run(label, command, code=0, extra=None):
        proc = subprocess.run([str(x) for x in command], cwd=ROOT,
                              env={**env, **(extra or {})}, capture_output=True, text=True, timeout=60)
        (out / (label + '.log')).write_text(proc.stdout + proc.stderr)
        rows.append(dict(label=label, command=[str(x) for x in command], exit=proc.returncode,
                         expected_exit=code, env_overrides=extra or {}))
        assert proc.returncode == code, (label, proc.returncode, proc.stdout, proc.stderr)

    def report(label, function, source=SOURCE, defines=(), code=0):
        artifact, text = out / (label + '.json'), out / (label + '.txt')
        command = [compiler, '--overlay=physics-units', '--units-coverage-function', function,
                   '--emit-units-coverage-json', artifact, '--emit-units-coverage-text', text]
        command += ['-D' + value for value in defines]
        command += [source]
        run(label, command, code)
        d = json.loads(artifact.read_text())
        assert d['schema'] == 'fisiCs.units_coverage' and d['schema_version'] == 1
        assert d['scope']['name'] == function and d['strict_enforcement'] is False
        summary = d['denominator']
        assert summary['total'] == len(d['obligations'])
        assert summary['total'] == sum(summary[k] for k in ('checked','trusted','unchecked','contradictory'))
        assert len({x['id'] for x in d['obligations']}) == len(d['obligations'])
        ids = {x['id'] for x in d['expressions']}
        for x in d['expressions']:
            assert {edge['expression'] for edge in x['dependencies']} <= ids
            assert all(edge['role'] in ('value','storage_base','index','condition','argument') for edge in x['dependencies'])
        for x in d['obligations']:
            assert x['result'] is None or x['result'] in ids
            assert x['boundary'] in ('initializer','assignment','argument','return','operation')
            assert x['site']['line'] > 0 and x['site']['column'] > 0 and x['site']['file'], x
        assert 'Selected obligations: ' + str(summary['total']) in text.read_text()
        return d

    # A selected valid path has one multiplication, one initializer and one return.
    good = report('travel', 'travel')
    assert good['scope']['enumeration_complete'] and good['denominator']['total'] == 3
    assert good['denominator']['checked'] == 3 and not good['denominator']['unchecked']
    again = report('travel-repeat', 'travel')
    assert good == again
    assert (out/'travel.json').read_bytes() == (out/'travel-repeat.json').read_bytes()
    assert (out/'travel.txt').read_bytes() == (out/'travel-repeat.txt').read_bytes()
    stripped = report('travel-stripped', 'travel', defines=('STRIP_METADATA=1',))
    assert stripped['analysis_configuration']['macro_defines'] == ['STRIP_METADATA=1']
    assert stripped['denominator']['total'] == good['denominator']['total']
    assert stripped['denominator']['unchecked'] == stripped['denominator']['total']
    field = report('field', 'field_update')
    assert field['denominator']['checked'] == field['denominator']['total']
    assert any(x['fact']['origin_location'] and x['fact']['origin_location']['line'] == next(i+1 for i,line in enumerate(SOURCE.read_text().splitlines()) if line.startswith('struct Body {'))
               for x in field['expressions'])
    broken = report('field-bad', 'field_update', defines=('BAD_UPDATE=1',), code=1)
    assert broken['compilation_errors'] and broken['denominator']['contradictory']
    math = report('math', 'norm')
    assert math['denominator']['checked'] == math['denominator']['total']
    assert any(x['operation'] == 'math_rule' for x in math['obligations'])
    trusted = report('trusted', 'trust_path')
    assert trusted['denominator']['trusted'] >= 2
    arguments = report('arguments', 'arguments')
    assert any(x['operation'] == 'argument' and x['state'] == 'checked' for x in arguments['obligations'])
    wrong_argument = report('arguments-bad', 'arguments', defines=('BAD_ARGUMENT=1',), code=1)
    assert any(x['operation'] == 'argument' and x['state'] == 'contradictory' for x in wrong_argument['obligations'])
    missing = report('missing', 'missing_path')
    assert missing['denominator']['unchecked'] >= 2
    assert any(x['fact']['reason'] == 'uncontracted_call' for x in missing['expressions'])
    compound = report('compound', 'compound')
    assert not compound['denominator']['contradictory']
    assert compound['denominator']['unchecked'] >= 1
    identities = report('identities', 'record_identity')
    fields = [x['fact'] for x in identities['expressions'] if x['fact']['origin_location'] and
              x['fact']['origin_location']['line'] in tuple(i+1 for i,line in enumerate(SOURCE.read_text().splitlines()) if line.startswith(('struct Length {', 'struct Time {')))]
    assert {tuple(f['dimension']) for f in fields if f['dimension']} == {(1,0,0,0,0,0,0,0), (0,0,1,0,0,0,0,0)}
    branch = report('branches', 'branch_scope', defines=('BAD_BRANCH=1',), code=1)
    assert branch['denominator']['contradictory'] and len(branch['obligations']) > 3
    empty = report('empty', 'empty_scope')
    assert empty['denominator']['total'] == 0 and empty['scope']['enumeration_complete']
    raw = report('translation-raw', 'integrate', ROOT/'tests/fixtures/solver_physics/translation.c')
    typed = report('translation-typed', 'integrate', ROOT/'tests/fixtures/solver_physics/translation.c', ('TYPED_STATE=1',))
    assert raw['denominator']['unchecked'] > typed['denominator']['unchecked']
    assert any(x['fact']['reason'] == 'unannotated_storage' for x in raw['expressions'])
    assert any(x['loss_location'] for x in raw['obligations'] if x['state'] == 'unchecked')
    matrix = report('matrix', 'main', ROOT/'tests/fixtures/solver_physics/structure.c')
    assert matrix['denominator']['unchecked'] and not matrix['scope']['enumeration_complete']
    assert any(x['fact']['reason'] == 'unannotated_storage' for x in matrix['expressions'])
    included_header = out/'provenance.h'
    included_header.write_text('struct Included { double value FISICS_DIM(length); };\n')
    included_source = out/'provenance.c'
    included_source.write_text('#include <fisics/extensions.h>\n#include "provenance.h"\n#define READ(p) ((p)->value)\ndouble included(struct Included *p) FISICS_DIM(length) { return READ(p); }\n')
    included = report('included', 'included', included_source)
    origins = [x['fact']['origin_location'] for x in included['expressions'] if x['fact']['origin_location']]
    assert any(Path(x['file']).resolve() == included_header.resolve() and x['line'] == 1 for x in origins), origins
    # CLI refusal and preservation controls.
    base = [compiler, '--overlay=physics-units', '--units-coverage-function', 'travel']
    preserved = out/'preserved.json'; preserved.write_text('keep-me\n')
    for label, arguments in [
        ('missing-scope', ['--emit-units-coverage-json',preserved,SOURCE]),
        ('missing-value', ['--units-coverage-function']),
        ('no-output', ['--overlay=physics-units','--units-coverage-function','travel',SOURCE]),
        ('no-overlay', ['--units-coverage-function','travel','--emit-units-coverage-json',preserved,SOURCE]),
        ('duplicate-scope', base[1:]+['--units-coverage-function','travel','--emit-units-coverage-json',preserved,SOURCE]),
        ('multi-tu', base[1:]+['--emit-units-coverage-json',preserved,SOURCE,SOURCE]),
        ('unknown-scope', ['--overlay=physics-units','--units-coverage-function','absent','--emit-units-coverage-json',preserved,SOURCE]),
        ('output-conflict', base[1:]+['--emit-units-coverage-json',preserved,'-o',out/'unwanted',SOURCE]),
        ('same-output', base[1:]+['--emit-units-coverage-json',preserved,'--emit-units-coverage-text',preserved,SOURCE]),
        ('external-pp', base[1:]+['--preprocess=external','--emit-units-coverage-json',preserved,SOURCE]),
    ]:
        run(label,[compiler]+arguments,1)
        assert preserved.read_text() == 'keep-me\n'
    run('write-failure',base+['--emit-units-coverage-json',preserved,SOURCE],1,
        {'FISICS_TEST_FAIL_UNITS_COVERAGE_WRITE':'1'})
    assert preserved.read_text() == 'keep-me\n'
    copied = out/'input.c'; copied.write_bytes(SOURCE.read_bytes()); digest=hashlib.sha256(copied.read_bytes()).hexdigest()
    for name,path in [('direct',copied),('symlink',out/'alias.c'),('hardlink',out/'hard.c')]:
        if name=='symlink':path.symlink_to(copied)
        if name=='hardlink':os.link(copied,path)
        run('input-'+name,base+['--emit-units-coverage-json',path,copied],1)
        assert hashlib.sha256(copied.read_bytes()).hexdigest() == digest
    header = out/'keep.h'; header.write_text('#define KEPT_HEADER 1\n')
    header_digest = hashlib.sha256(header.read_bytes()).hexdigest()
    copied.write_text('#include "keep.h"\n' + SOURCE.read_text())
    run('included-input',base+['--emit-units-coverage-json',preserved,'--emit-units-coverage-text',header,copied],1)
    assert preserved.read_text() == 'keep-me\n'
    assert hashlib.sha256(header.read_bytes()).hexdigest() == header_digest
    malformed = out/'malformed.c'; malformed.write_text('void travel(void) { double = ; }\n')
    run('parse-failure',base+['--emit-units-coverage-json',preserved,malformed],1)
    assert preserved.read_text() == 'keep-me\n'
    assert not list(out.glob('*.tmp.*')) and not (out/'unwanted').exists()
    run('clang-control',['clang','-std=c99','-pedantic-errors','-I',ROOT/'include','-c',SOURCE,'-o',out/'control.o'])
    (out/'control.o').unlink()
    (out/'results.json').write_text(json.dumps(dict(passed=True,commands=rows,selected_valid_obligations=3,
        stable_denominator_after_metadata_removal=True,raw_gaps_retained=True),indent=2)+'\n')
    print(f'Coverage reporting contracts: PASS ({len(rows)} commands)')


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',type=Path,required=True);p.add_argument('--out',type=Path,required=True)
    a=p.parse_args();a.out.mkdir(parents=True,exist_ok=False);check(a.compiler.resolve(),a.out.resolve())


if __name__ == '__main__':
    main()
