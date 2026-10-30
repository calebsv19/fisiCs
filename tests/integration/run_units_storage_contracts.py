#!/usr/bin/env python3
"""Physical literal writes, C predicates/status and reviewed carrier-interface contracts."""
import argparse,json,os,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
SOURCE=ROOT/'tests/fixtures/solver_physics/storage_boundaries.c'
def check(compiler,out):
    commands=[]
    def run(label,cmd,code=0):
        p=subprocess.run([str(x) for x in cmd],cwd=ROOT,env={**os.environ,'FISICS_MAX_PROCS':'0'},capture_output=True,text=True,timeout=60)
        (out/(label+'.log')).write_text(p.stdout+p.stderr);commands.append(dict(label=label,command=[str(x) for x in cmd],exit=p.returncode,expected_exit=code))
        assert p.returncode==code,(label,p.returncode,p.stdout,p.stderr)
    def report(label,function,code=0,flags=()):
        path=out/(label+'.json')
        run(label,[compiler,'--overlay=physics-units','--units-coverage-strict','--units-coverage-function',function,'--emit-units-coverage-json',path,*flags,SOURCE],code)
        d=json.loads(path.read_text());p=d['strict_policy'];assert p['accepted'] is (code==0)
        assert p['required']+p['ordinary_controls']==d['denominator']['total']
        assert p['required']==p['checked']+p['permitted_trust']+p['blocked']
        return d
    literal='--units-coverage-allow-literal-assumptions';declared='--units-coverage-allow-declared-boundaries'
    report('literal-denied','literals',1)
    d=report('literal-permitted','literals',flags=[literal]);assert d['strict_policy']['permitted_trust']==4
    report('literal-wrong-permission','literals',1,[declared])
    report('interface-denied','caller',1)
    d=report('interface-permitted','caller',flags=[declared]);assert d['strict_policy']['checked']==1 and d['strict_policy']['permitted_trust']==2
    assert sum(x.get('strict_basis')=='declared_c_interface' for x in d['obligations'])==2
    report('callee-independent','commit')
    report('external-not-eligible','external_caller',1,[declared,literal])
    report('scalar-pointer-not-eligible','scalar_caller',1,[declared,literal])
    d=report('predicate-status','guards');assert d['strict_policy']['ordinary_controls']>=4
    report('physical-truth-not-waived','physical_guard',1,[declared,literal])
    report('physical-status-not-waived','physical_status',1,[declared,literal])
    for name in ['BAD_VARIABLE','BAD_COMPOUND','BAD_TIME','BAD_CALL']:
        d=report(name.lower(),'incompatible',1,[declared,literal,'-D'+name]);assert d['strict_policy']['blocked']>0
    d=report('wrong-unit','unit_boundary',1,[declared,literal,'-DBAD_UNIT']);assert d['denominator']['contradictory']>0
    report('invalid-callee-tu','caller',1,[declared,literal,'-DBAD_HELPER'])
    # Ordinary compilation proves literal support and preserves direct rejection controls.
    for label,macro,code in [('plain-literals',None,0),('plain-variable','BAD_VARIABLE',1),('plain-compound','BAD_COMPOUND',1),('plain-time','BAD_TIME',1),('plain-unit','BAD_UNIT',1)]:
        obj=out/(label+'.o');cmd=[compiler,'--overlay=physics-units','-c',SOURCE,'-o',obj]
        if macro:cmd+=['-D'+macro]
        run(label,cmd,code)
        if obj.exists():obj.unlink()
    run('clang-source',['clang','-std=c99','-pedantic-errors','-I',ROOT/'include','-fsyntax-only',SOURCE])
    (out/'results.json').write_text(json.dumps(dict(passed=True,commands=commands),indent=2)+'\n')
    print(f'Storage/C-boundary contracts: PASS ({len(commands)} commands)')
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args();a.out.mkdir(parents=True,exist_ok=False);check(a.compiler.resolve(),a.out.resolve())
if __name__=='__main__':main()
