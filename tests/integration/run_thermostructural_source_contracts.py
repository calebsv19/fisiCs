#!/usr/bin/env python3
"""Opt-in finite prescribed-temperature source contracts; no runtime/whole-owner claim."""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__)
 for n in ['compiler','core-root','out']:p.add_argument('--'+n,type=Path,required=True)
 a=p.parse_args();a.compiler=a.compiler.resolve();a.core_root=a.core_root.resolve();a.out=a.out.resolve();a.out.mkdir(parents=True,exist_ok=False)
 c=a.core_root;fixture=ROOT/'tests/fixtures/solver_physics/thermostructural';source=c/'src/core_structural_thermal_strain.c';policy=c/'physics/frame_policy.json';commands=[];selections=[];result=dict(passed=False,scope='selected physical/units arithmetic and portable fixtures; SI/constitutive/basis/Python trust remains declared',commands=commands,selections=selections)
 deps=[a.compiler,Path(__file__),policy,c/'physics/frame_catalog.json',*list((c/'src').glob('*.h')),source,ROOT/'include/fisics/extensions.h',*list(fixture.glob('*.c'))];result['inputs']={str(p):sha(p) for p in deps}
 callsites={'sf_th_eigenstrain':3,'sf_th_force':1,'sf_th_total':1,'sf_th_elastic':1,'sf_th_axial':1,'sf_th_translation_energy':1,'sf_th_rotation_energy':1,'sf_th_bending_energy':2,'sf_th_elastic_sum':2}
 actual=source.read_text()
 assert all(actual.count(fn+'(')==n for fn,n in callsites.items()),'actual source/helper callsite drift'
 result['actual_source_callsites']=callsites
 env={**os.environ,'PYTHONDONTWRITEBYTECODE':'1','FISICS_MAX_PROCS':'0'};env.pop('FISICS_OVERLAY',None)
 def run(label,args,expected=0):
  cmd=list(map(str,args));q=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,env=env,timeout=120);(a.out/(label+'.log')).write_text(q.stdout+q.stderr);commands.append(dict(label=label,argv=cmd,exit=q.returncode,expected=expected));assert q.returncode==expected,(label,q.returncode,(q.stdout+q.stderr)[-2000:]);return q.stdout
 inc=['-I'+str(ROOT/'include'),'-I'+str(c/'include'),'-I'+str(c/'src'),'-I'+str(c.parent/'core_base/include')];physical=['--overlay=physics-semantics','--physical-semantics-policy',policy];functions=['sf_th_eigenstrain','sf_th_force','sf_th_total','sf_th_elastic','sf_th_axial','sf_th_translation_energy','sf_th_rotation_energy','sf_th_elastic_sum']
 try:
  # These malformed models must fail at policy admission even for plain C;
  # a later missing-origin source error would not prove role validation.
  bad=json.loads(policy.read_bytes());bad['catalog_paths']=[str(c/'physics/frame_catalog.json')];bad['affine_pairs']=[];bad['origins']=[];bad['linear_kinds']+=['quantity.temperature.absolute','quantity.temperature.difference']
  wrong=a.out/'wrong-difference-role.json';wrong.write_text(json.dumps(bad));plain=a.out/'plain.c';plain.write_text('int check(void){return 0;}\n')
  run('wrong-difference-role',[a.compiler,'--overlay=physics-semantics','--physical-semantics-policy',wrong,'-c',plain,'-o',a.out/'forbidden-role.o'],1)
  assert not (a.out/'forbidden-role.o').exists()
  for name in ['positive','negative-alpha','wrong-origin','missing-origin','absolute-multiply','raw-alpha','raw-scalar','cast-relabel','angle-strain','absolute-as-difference','thermal-as-elastic','unknown-elastic','scalar-elastic']:
   report=a.out/(name+'.json');code=0 if name in ['positive','negative-alpha'] else 1
   run(name,[a.compiler,*physical,'-DPILOT_UNITS=1','-DPILOT_PHYSICAL=1','-DCORE_STRUCTURAL_PHYSICAL_SEMANTICS=1',*inc,'--physical-semantics-strict','--physical-semantics-function','check','--emit-physical-semantics-json',report,fixture/(name+'.c')],code)
   run(name+'-clang',['clang','-std=c11','-pedantic-errors','-Wall','-Wextra','-Werror','-Wno-unused-parameter',*inc,'-c',fixture/(name+'.c'),'-o',a.out/(name+'.o')])
  for fn,mode,code in [(n,'physical',0) for n in functions]+[('sf_th_bending_energy','units',0)]+[(n,'physical',1) for n in ['sf_th_bending_energy','sf_thermal_validate','sf_thermal_element','sf_thermal_recover','cs_frame_elastic_restore_assumed_linear']]:
   prefix='physical-semantics' if mode=='physical' else 'units-coverage';report=a.out/(fn+'-'+mode+'.json');flags=[*physical,'-DCORE_STRUCTURAL_PHYSICAL_SEMANTICS=1'] if mode=='physical' else ['--overlay=physics-units']
   run(fn+'-'+mode,[a.compiler,*flags,*inc,'--'+prefix+'-strict','--'+prefix+'-allow-literal-assumptions','--'+prefix+'-function',fn,'--emit-'+prefix+'-json',report,source],code)
   if report.is_file():
    data=json.loads(report.read_bytes());selection=data['selection' if mode=='physical' else 'strict_policy'];assert selection['accepted']==(code==0)
    if not code:assert selection['checked']>0 and selection['blocked']==0
    selections.append(dict(function=fn,mode=mode,policy=selection))
  for mode,args in [('clang',['clang','-std=c11','-pedantic-errors']),('base',[a.compiler]),('units',[a.compiler,'--overlay=physics-units','-DPILOT_UNITS=1']),('physical',[a.compiler,*physical,'-DPILOT_UNITS=1','-DPILOT_PHYSICAL=1'])]:
   exe=a.out/('runtime-'+mode);run('runtime-'+mode+'-compile',[*args,*inc,fixture/'runtime.c','-o',exe]);run('runtime-'+mode,[exe])
  assert all(sha(Path(p))==h for p,h in result['inputs'].items()),'source drift'
  result['passed']=True
 finally:
  result['artifacts']={str(p.relative_to(a.out)):sha(p) for p in a.out.rglob('*') if p.is_file() and p.suffix not in ['.o'] and not os.access(p,os.X_OK)}
  (a.out/'results.json').write_text(json.dumps(result,indent=2,sort_keys=True)+'\n')
 print('Thermostructural source contracts: PASS')
if __name__=='__main__':main()
