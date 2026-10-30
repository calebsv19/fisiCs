#!/usr/bin/env python3
"""Opt-in actual dynamic-energy helper checking; no whole-solver strict claim."""
from pathlib import Path
import argparse,hashlib,json,os,subprocess,shutil
ROOT=Path(__file__).resolve().parents[2]
def main():
 p=argparse.ArgumentParser(description=__doc__)
 for n in ['compiler','core-root','out']:p.add_argument('--'+n,type=Path,required=True)
 a=p.parse_args();a.compiler=a.compiler.resolve();a.core_root=a.core_root.resolve();a.out=a.out.resolve();a.out.mkdir(parents=True,exist_ok=False);c=a.core_root;o=a.out
 env={**os.environ,'PYTHONDONTWRITEBYTECODE':'1','FISICS_MAX_PROCS':'0'};env.pop('FISICS_OVERLAY',None)
 sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();source=c/'src/core_structural_frame_motion_integrate.c';header=c/'src/core_structural_dynamic_energy.h';policy=c/'physics/dynamic_energy_policy.json';fixture=ROOT/'tests/fixtures/solver_physics/dynamic_energy/runtime.c';inc=['-I'+str(ROOT/'include'),'-I'+str(c/'src'),'-I'+str(c/'include'),'-I'+str(c.parent/'core_base/include')];physical=['--overlay=physics-semantics','--physical-semantics-policy',policy,'-DCORE_STRUCTURAL_PHYSICAL_SEMANTICS=1'];result=dict(passed=False,commands=[],selections=[],scope='selected actual scalar bodies; matrix/DOF/basis/constitutive and energy-form restoration unproved')
 result['inputs']={str(q):sha(q) for q in [a.compiler,Path(__file__),source,header,fixture,*sorted((c/'physics').glob('*.json')),*sorted((c/'src').glob('*.h'))]}
 def save():(o/'results.json').write_text(json.dumps(result,indent=2,sort_keys=True)+'\n')
 def run(label,args,expected=0):
  cmd=list(map(str,args));q=subprocess.run(cmd,cwd=ROOT,env=env,capture_output=True,text=True,timeout=120);(o/(label+'.log')).write_text(q.stdout+q.stderr);result['commands'].append(dict(label=label,argv=cmd,exit=q.returncode,expected=expected));save();assert q.returncode==expected,(label,(q.stdout+q.stderr)[-1500:]);return q.stdout
 try:
  for fn in ['cs_de_kinetic_tt','cs_de_kinetic_rr','cs_de_damping_tt','cs_de_damping_rr','cs_de_translation_work','cs_de_rotation_work','cs_de_balance','cs_de_kinetic_tr','cs_de_kinetic_rt','cs_de_damping_tr','cs_de_damping_rt','cs_de_kinetic','cs_de_damping','cs_de_work','integrate']:
   mode='units' if fn.endswith(('_tr','_rt')) else 'physical';ok=fn not in ['cs_de_kinetic','cs_de_damping','cs_de_work','integrate'];prefix='physical-semantics' if mode=='physical' else 'units-coverage';report=o/(fn+'.json');flags=physical if mode=='physical' else ['--overlay=physics-units'];run(fn,[a.compiler,*flags,*inc,'--'+prefix+'-strict','--'+prefix+'-allow-literal-assumptions','--'+prefix+'-function',fn,'--emit-'+prefix+'-json',report,source],0 if ok else 1);j=json.loads(report.read_text());selection=j['selection' if mode=='physical' else 'strict_policy'];assert selection['accepted']==ok;result['selections'].append(dict(function=fn,mode=mode,policy=selection))
  bad=json.loads((c/'physics/dynamic_energy_catalog.json').read_text());next(x for x in bad['quantity_kinds'] if x['id']=='quantity.rotational_momentum')['dimension_vector'][0]=1
  badcat=o/'wrong-dimension-catalog.json';badcat.write_text(json.dumps(bad));badpolicy=json.loads(policy.read_text());badpolicy['catalog_paths']=[str(c/'physics/frame_catalog.json'),str(badcat)];bp=o/'wrong-dimension-policy.json';bp.write_text(json.dumps(badpolicy));plain=o/'plain.c';plain.write_text('int check(void){return 0;}\n');run('valid-plain-model',[a.compiler,*physical,'-c',plain,'-o',o/'valid-plain.o']);run('wrong-model-dimension',[a.compiler,'--overlay=physics-semantics','--physical-semantics-policy',bp,'-c',plain,'-o',o/'forbidden-model.o'],1);assert not (o/'forbidden-model.o').exists()
  missing=json.loads(policy.read_text());missing['catalog_paths']=[str(c/'physics/frame_catalog.json'),str(c/'physics/dynamic_energy_catalog.json')];missing['operation_rules']=[x for x in missing['operation_rules'] if x['id']!='rule.dynamic.rotational_momentum.angular_rate'];mp=o/'missing-operation-policy.json';mp.write_text(json.dumps(missing));run('missing-rotation-rule',[a.compiler,'--overlay=physics-semantics','--physical-semantics-policy',mp,'-DCORE_STRUCTURAL_PHYSICAL_SEMANTICS=1',*inc,'--physical-semantics-strict','--physical-semantics-allow-literal-assumptions','--physical-semantics-function','cs_de_kinetic_rr','--emit-physical-semantics-json',o/'missing-rotation-rule.json',source],1)
  controls={
   'valid-rotation':('double w DE_W,double m DE_I','cs_de_kinetic_rr(w,m,w)',True),
   'rate-is-not-decay':('double w DE_A,double m DE_I','cs_de_kinetic_rr(w,m,w)',False),
   'angle-is-not-rate':('double w CS_PHYSICAL_DIM(1) DE_KIND("quantity.angle"),double m DE_I','cs_de_kinetic_rr(w,m,w)',False),
   'mass-is-not-inertia':('double w DE_W,double m DE_M','cs_de_kinetic_rr(w,m,w)',False),
   'inertia-is-not-mass':('double v DE_V,double m DE_I','cs_de_kinetic_tt(v,m,v)',False),
   'raw-restore':('double v,double m','cs_de_kinetic_tt(v,m,v)',False),
   'wrong-load-kind':('double f CS_PHYSICAL_DIM(energy) DE_KIND("quantity.torque"),double u CS_PHYSICAL_DIM(length) DE_KIND("quantity.displacement")','cs_de_translation_work(f,f,u,u)',False)}
  for label,(params,body,ok) in controls.items():
   src=o/(label+'.c');src.write_text('#include "core_structural_dynamic_energy.h"\ndouble DE_E check('+params+'){return '+body+';}\n');run(label,[a.compiler,*physical,*inc,'--physical-semantics-strict','--physical-semantics-allow-literal-assumptions','--physical-semantics-allow-declared-boundaries','--physical-semantics-function','check','--emit-physical-semantics-json',o/(label+'.json'),src],0 if ok else 1);run(label+'-portable',['clang','-std=c11','-Werror','-Wall','-Wextra',*inc,'-c',src,'-o',o/(label+'.o')])
  for mode,tool,flags in [('clang','clang',['-std=c11']),('base',a.compiler,[]),('units',a.compiler,['--overlay=physics-units']),('physical',a.compiler,physical)]:
   exe=o/('runtime-'+mode);runtime_source=fixture
   if mode=='base':
    runtime_source=o/'runtime-base.c';runtime_source.write_text(run('base-preprocess',['clang','-std=c11','-fno-blocks','-E','-P',*inc,fixture]))
   run(mode+'-runtime-compile',[tool,*flags,*inc,runtime_source,'-lm','-o',exe]);run(mode+'-runtime',[exe])
  text=header.read_text();mutants=[('missing-half','half DE_R=.5','half DE_R=1'),('double-dt','return dt*alpha*quarter','return (dt+dt)*alpha*quarter'),('wrong-reference','return current-initial+kinetic+damping-external','return current+initial+kinetic+damping-external'),('omit-damping','return current-initial+kinetic+damping-external','return current-initial+kinetic-external'),('duplicate-work','return current-initial+kinetic+damping-external','return current-initial+kinetic+damping-external-external'),('work-sign','half*(old+now),right-left','half*(old+now),left-right')]
  for label,old,new in mutants:
   assert old in text;d=o/('mutant-'+label);d.mkdir();(d/header.name).write_text(text.replace(old,new));exe=d/'probe';run(label+'-compile',['clang','-std=c11','-I'+str(d),*inc,fixture,'-lm','-o',exe]);q=subprocess.run([str(exe)],capture_output=True,text=True,timeout=10);assert q.returncode in [1,2,3],(label,'numeric mutant escaped');result['commands'].append(dict(label=label+'-detected',exit=q.returncode,expected='fixture rejects 1..3'))
  assert all(sha(Path(n))==h for n,h in result['inputs'].items()),'source drift';result['passed']=True
 finally:save()
 print('Dynamic energy source contracts PASS')
if __name__=='__main__':main()
