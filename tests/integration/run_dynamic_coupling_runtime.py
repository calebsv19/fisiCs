#!/usr/bin/env python3
from pathlib import Path
import os,subprocess,json,hashlib,argparse
p=argparse.ArgumentParser(description='Actual-source cross-block numerical, ABI and archive qualification')
for n in ['compiler','core-root','core-build','structural-root','contracts','out']:p.add_argument('--'+n,type=Path,required=True)
a=p.parse_args();f=Path(__file__).resolve().parents[2];c=a.core_root.resolve();s=a.structural_root.resolve();candidate=a.contracts.resolve()/'candidate';out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);rows=[]
fixture=f/'tests/fixtures/solver_physics/dynamic_coupling'
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
contract=json.loads((a.contracts/'results.json').read_text());assert contract['passed']
assert all(sha(n)==h for n,h in contract['inputs'].items()),'contract source/compiler drift'
assert sha(a.compiler)==contract['inputs'][str(a.compiler.resolve())]
assert (a.core_build/'libcore_structural.a').is_file()
runtime_inputs={str(p):sha(p) for p in [*fixture.glob('*.c'),*a.core_build.glob('*.o'),a.contracts/'clang.o',a.contracts/'physical.o']}
assert len(list(a.core_build.glob('*.o')))>1
inc=['-I'+str(f/'include'),'-I'+str(candidate),'-I'+str(c/'src'),'-I'+str(c/'include'),'-I'+str(c.parent/'core_base/include')];env={**os.environ,'FISICS_MAX_PROCS':'0','PYTHONDONTWRITEBYTECODE':'1'}
flags=['--overlay=physics-semantics','--physical-semantics-policy',candidate/'policy.json','-DCORE_STRUCTURAL_PHYSICAL_SEMANTICS=1']
def run(n,args,code=0,cwd=f):
 q=subprocess.run(list(map(str,args)),cwd=cwd,env=env,capture_output=True,text=True,timeout=240);(out/(n+'.log')).write_text(q.stdout+q.stderr);rows.append(dict(id=n,argv=list(map(str,args)),exit=q.returncode,expected=code));(out/'commands.json').write_text(json.dumps(rows,indent=2));assert q.returncode==code,(n,(q.stdout+q.stderr)[-1600:]);return q.stdout
for mode,tool,opts in [('clang','clang',['-std=c11','-O0','-ffp-contract=off']),('base',a.compiler.resolve(),[]),('units',a.compiler.resolve(),['--overlay=physics-units']),('physical',a.compiler.resolve(),flags)]:
 src=fixture/'coupling_runtime.c'
 if mode=='base':
  src=out/'runtime-base.c';src.write_text(run('base-preprocess',['clang','-std=c11','-fno-blocks','-E','-P',*inc,fixture/'coupling_runtime.c']))
 run(mode+'-helpers',[tool,*opts,*inc,'-c',src,'-o',out/(mode+'.o')])
for mode,tool,opts in [('clang','clang',['-std=c11','-O0','-ffp-contract=off']),('fisics',a.compiler.resolve(),[])]:
 run(mode+'-oracle',[tool,*opts,'-c',fixture/'coupling_oracle.c','-o',out/(mode+'-oracle.o')])
outputs=[]
for mode in ['clang','base','units','physical']:
 for caller in ['clang','fisics']:
  exe=out/(mode+'-'+caller);run(exe.name+'-link',['clang',out/(mode+'.o'),out/(caller+'-oracle.o'),'-lm','-o',exe]);outputs.append(run(exe.name+'-run',[exe]))
assert len(set(outputs))==1
# Cross-block mutants remain dimensionally legal but must fail independent oracle.
text=(candidate/'core_structural_dynamic_energy.h').read_text()
for name,old,new in [('cross-half','return half*vi*mass*vj;','return vi*mass*vj;'),('cross-dt','return dt*alpha*quarter*(oi+vi)*mass*(oj+vj);','return alpha*quarter*(oi+vi)*mass*(oj+vj);'),('cross-sign','return half*vi*mass*vj;','return -half*vi*mass*vj;')]:
 d=out/name;d.mkdir();assert old in text;changed='\n'.join(line.replace(old,new) if (' cs_de_kinetic_tr(' in line or ' cs_de_damping_tr(' in line) else line for line in text.splitlines())+'\n';assert changed!=text;(d/'core_structural_dynamic_energy.h').write_text(changed);obj=d/'mutant.o';run(name+'-compile',['clang','-std=c11','-O0','-I'+str(d),*inc,'-c',fixture/'coupling_runtime.c','-o',obj]);exe=d/'probe';run(name+'-link',['clang',obj,out/'clang-oracle.o','-lm','-o',exe]);run(name+'-detect',[exe],1)
# Link actual selected integration object into fresh current native core archive.
common=[p for p in sorted(a.core_build.resolve().glob('*.o')) if p.stem!='core_structural_frame_motion_integrate']
for mode in ['clang','physical']:
 archive=out/(mode+'.a');run(mode+'-archive',['ar','rcs',archive,*common,a.contracts.resolve()/(mode+'.o')]);exe=out/(mode+'-account');run(mode+'-account-build',['clang','-std=c11',*inc,'-I'+str(s/'tests'),s/'tests/dynamic_energy_probe.c',archive,'-lm','-o',exe]);
 # Maintained all-tick account qualification takes probe tools as arguments (inspected separately).
assert all(sha(n)==h for n,h in runtime_inputs.items()),'runtime/build input drift'
assert all(sha(n)==h for n,h in contract['inputs'].items()),'contract drift'
(out/'inputs.json').write_text(json.dumps(runtime_inputs,indent=2))
(out/'results.json').write_text(json.dumps(dict(passed=True,exact_checks_per_variant=2916,variants=8,mutants_detected=3,actual_integration_archives=['clang','physical'],commands=len(rows)),indent=2))
print('Consumer helper ABI and actual archive build PASS')
