#!/usr/bin/env python3
"""Strict scope acceptance, trust separation, ordinary-control and failure controls."""
import argparse
import json
import os
from pathlib import Path
import subprocess
ROOT=Path(__file__).resolve().parents[2]
REPORTING=ROOT/'tests/fixtures/solver_physics/coverage_reporting.c'
STRICT=ROOT/'tests/fixtures/solver_physics/coverage_strict.c'

def check(compiler,out):
    commands=[]
    def run(label,cmd,code=0,extra=None):
        p=subprocess.run([str(x) for x in cmd],cwd=ROOT,env={**os.environ,'FISICS_MAX_PROCS':'0',**(extra or {})},capture_output=True,text=True,timeout=60)
        (out/(label+'.log')).write_text(p.stdout+p.stderr)
        commands.append(dict(label=label,command=[str(x) for x in cmd],exit=p.returncode,expected_exit=code))
        assert p.returncode==code,(label,p.returncode,p.stdout,p.stderr)
    base=[compiler,'--overlay=physics-units','--units-coverage-strict']
    declared='--units-coverage-allow-declared-boundaries'
    literal='--units-coverage-allow-literal-assumptions'
    def report(label,function,source=REPORTING,flags=(),code=0):
        dest=out/(label+'.json'); text=out/(label+'.txt')
        cmd=base+['--units-coverage-function',function,'--emit-units-coverage-json',dest,'--emit-units-coverage-text',text]+list(flags)+[source]
        run(label,cmd,code)
        d=json.loads(dest.read_text());p=d['strict_policy']
        assert d['strict_enforcement'] is True and p['contract']=='selected_function_v1'
        assert p['accepted'] is (code==0)
        assert p['required']+p['ordinary_controls']==d['denominator']['total']
        assert p['required']==p['checked']+p['permitted_trust']+p['blocked']
        assert p['ordinary_controls']==sum(not x['strict_required'] for x in d['obligations'])
        assert ('accepted' if code==0 else 'rejected') in text.read_text()
        return d
    good=report('travel','travel'); assert good['strict_policy']['checked']==3
    stripped=report('stripped','travel',flags=['-DSTRIP_METADATA'],code=1)
    assert stripped['denominator']['total']==good['denominator']['total'] and stripped['strict_policy']['blocked']==3
    report('field','field_update')
    report('bad-field','field_update',flags=['-DBAD_UPDATE'],code=1)
    loop=report('loop','branch_scope');assert loop['strict_policy']['ordinary_controls']==3 and loop['strict_policy']['checked']==3
    report('unreachable-bad-branch','branch_scope',flags=['-DBAD_BRANCH'],code=1)
    report('empty','empty_scope',code=1)
    report('trust-denied','trust_path',code=1)
    trusted=report('trust-permitted','trust_path',flags=[declared]);assert trusted['strict_policy']['permitted_trust']>=2 and trusted['denominator']['trusted']>=2
    report('wrong-trust-permission','trust_path',flags=[literal],code=1)
    report('missing-with-all-trust','missing_path',flags=[literal,declared],code=1)
    report('unknown-scale','compound',flags=[literal,declared],code=1)
    report('math','norm')
    report('arguments','arguments',flags=[declared])
    report('bad-argument','arguments',flags=[declared,'-DBAD_ARGUMENT'],code=1)
    report('literal-denied','literal_force',STRICT,code=1)
    report('literal-permitted','literal_force',STRICT,flags=[literal])
    report('literal-wrong-permission','literal_force',STRICT,flags=[declared],code=1)
    report('trust-only-not-proof','external_only',STRICT,flags=[declared],code=1)
    report('controls-only-not-proof','counter_only',STRICT,code=1)
    for name in ['scalar_body','counter_enters_physics','annotated_counter','raw_bound','ordinary_scale','aggregate_unknown']:
        report(name,name,STRICT,flags=[declared,literal],code=1)
    report('wrong-dimension','wrong_dimension',STRICT,flags=[declared,literal,'-DBAD_DIM'],code=1)
    report('explicit-ratio','explicit_ratio',STRICT)
    report('physical-integer','physical_integer',STRICT)
    report('tu-error-outside-scope','literal_force',STRICT,flags=[literal,'-DBAD_DIM'],code=1)
    raw=report('raw-translation','integrate',ROOT/'tests/fixtures/solver_physics/translation.c',flags=[declared,literal],code=1)
    typed=report('typed-translation-input-gap','integrate',ROOT/'tests/fixtures/solver_physics/translation.c',flags=[declared,literal,'-DTYPED_STATE'],code=1)
    assert raw['strict_policy']['blocked']>typed['strict_policy']['blocked']>0
    matrix=report('raw-matrix','main',ROOT/'tests/fixtures/solver_physics/structure.c',flags=[declared,literal],code=1)
    assert not matrix['scope']['enumeration_complete'] and matrix['strict_policy']['blocked']>0

    repeated=report('repeat','branch_scope');assert repeated==loop
    assert (out/'repeat.txt').read_bytes()==(out/'loop.txt').read_bytes()
    # Trust permissions require strict mode; invalid invocations preserve destinations.
    sentinel=out/'sentinel.json';sentinel.write_text('keep-me\n')
    args=['--units-coverage-function','travel','--emit-units-coverage-json',sentinel,REPORTING]
    run('permission-without-strict',[compiler,'--overlay=physics-units',declared]+args,1)
    run('strict-without-scope',base+[REPORTING],1)
    run('duplicate-strict',base+['--units-coverage-strict']+args,1)
    run('duplicate-permission',base+[declared,declared]+args,1)
    run('strict-no-overlay',[compiler,'--units-coverage-strict']+args,1)
    run('publication-failure',base+args,1,{'FISICS_TEST_FAIL_UNITS_COVERAGE_WRITE':'1'})
    assert sentinel.read_text()=='keep-me\n' and not list(out.glob('*.tmp.*'))
    run('clang-source',['clang','-std=c99','-pedantic-errors','-I',ROOT/'include','-c',STRICT,'-o',out/'control.o'])
    (out/'control.o').unlink()
    (out/'results.json').write_text(json.dumps(dict(passed=True,commands=commands),indent=2)+'\n')
    print(f'Strict coverage contracts: PASS ({len(commands)} commands)')

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',type=Path,required=True);p.add_argument('--out',type=Path,required=True)
    a=p.parse_args();a.out.mkdir(parents=True,exist_ok=False);check(a.compiler.resolve(),a.out.resolve())
if __name__=='__main__':main()
