#!/usr/bin/env python3
"""Finite Cosmic scalar prerequisites; prototype evidence, not solver adoption."""
from pathlib import Path
import argparse,hashlib,json,os,subprocess,resource
ROOT=Path(__file__).resolve().parents[2]
def main():
 p=argparse.ArgumentParser(description=__doc__)
 for k in ['compiler','probe','out']:p.add_argument('--'+k,type=Path,required=True)
 p.add_argument('--old-compiler',type=Path);p.add_argument('--fixture-root',type=Path);p.add_argument('--include-root',type=Path);a=p.parse_args()
 a.compiler=a.compiler.resolve();a.probe=a.probe.resolve();a.out=a.out.resolve();a.out.mkdir(parents=True,exist_ok=False);f=(a.fixture_root or ROOT/'tests/fixtures/physical_semantics/cosmic').resolve();inc=(a.include_root or ROOT/'include').resolve();a.out.joinpath('tmp').mkdir()
 env=dict(os.environ,FISICS_MAX_PROCS='0',PYTHONDONTWRITEBYTECODE='1',TMPDIR=str(a.out/'tmp'));env.pop('FISICS_OVERLAY',None);sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest();pins={str(p):sha(p) for p in f.iterdir() if p.is_file()};pins[str(a.compiler)]=sha(a.compiler);result=dict(passed=False,solver_adoption=False,commands=[],inputs=pins)
 def no_core():resource.setrlimit(resource.RLIMIT_CORE,(0,0))
 def run(name,cmd,expected=0):
  args=list(map(str,cmd));q=subprocess.run(args,cwd=f,env=env,capture_output=True,text=True,timeout=120,preexec_fn=no_core);(a.out/(name+'.log')).write_text(q.stdout+q.stderr);result['commands'].append(dict(id=name,command=args,exit=q.returncode,expected=expected));assert q.returncode==expected,(name,q.returncode,(q.stdout+q.stderr)[-2200:]);return q.stdout
 policy=f/'policy.json';catalog=json.loads((f/'catalog.json').read_text());kinds={x['id']:x['dimension_text'] for x in catalog['quantity_kinds']};pol=json.loads(policy.read_text());source=a.out/'probe.c';source.write_text('#include "kernels.h"\n');flags=['-DCOSMIC_PHYSICAL=1','--overlay=physics-semantics','--physical-semantics-policy',policy,'-I'+str(inc),'-I'+str(f)]
 def strict(label,src,fn='check',expected=0,chosen=policy):
  dest=a.out/(label+'.json');args=[a.compiler,*flags];args[args.index(policy)]=chosen;run(label,[*args,'--physical-semantics-function',fn,'--physical-semantics-strict','--physical-semantics-allow-literal-assumptions','--emit-physical-semantics-json',dest,src],expected)
  if dest.exists():
   d=json.loads(dest.read_text());s=d['selection'];assert s['accepted']==(expected==0);assert s['required']==s['checked']+s['permitted_trust']+s['blocked'];return s
  assert expected
 try:
  selections={}
  for i,rule in enumerate(pol['operation_rules']):
   aa,bb=rule['operand_kinds'];kk=rule['result_kind'];op='*' if rule['operation']=='multiply' else '/';src=a.out/('row-'+str(i)+'.c');src.write_text('#include "kernels.h"\ndouble CP('+kinds[kk]+',"'+kk+'") check(double x CP('+kinds[aa]+',"'+aa+'"),double y CP('+kinds[bb]+',"'+bb+'")){return x'+op+'y;}\n');strict('row-'+str(i),src)
  for fn in ['cp_kinetic_2d','cp_mu','cp_force','cp_acceleration','cp_pair_potential']:selections[fn]=strict(fn,source,fn)
  frame=a.out/'frame.c';frame.write_text('#include "kernels.h"\ndouble check(double a CP(force,"quantity.force") FISICS_FRAME("frame.world") FISICS_COMPONENT_AXIS("axis.x")) CP(force,"quantity.force") FISICS_FRAME("frame.world") FISICS_COMPONENT_AXIS("axis.x"){return a;}\n')
  strict('same-frame',frame)
  frame.write_text(frame.read_text().replace('FISICS_FRAME("frame.world")','FISICS_FRAME("frame.local")',1));strict('mixed-frame',frame,expected=1)
  bodies={
   'torque-energy':'double CP(energy,"quantity.torque") check(double m CP(mass,"quantity.mass"),double v CP(speed,"quantity.velocity")){return cp_kinetic_2d(m,v,v);}',
   'specific-speed':'double CP(speed^2,"quantity.specific_energy") check(double v CP(speed,"quantity.velocity")){return v*v;}',
   'raw-mass':'double CP(energy,"quantity.energy") check(double m,double v CP(speed,"quantity.velocity")){return cp_kinetic_2d(m,v,v);}',
   'cast-launder':'double CP(energy,"quantity.energy") check(double q CP(energy,"quantity.torque")){return (double)q;}',
   'form-relabel':'double CP(energy,"quantity.energy") FISICS_ENERGY_FORM("energy.gravitational") check(double e CP(energy,"quantity.energy") FISICS_ENERGY_FORM("energy.kinetic")){return e;}',
   'unknown-frame':'double CP(speed,"quantity.velocity") FISICS_FRAME("frame.missing") check(double v CP(speed,"quantity.velocity")){return v;}',
   'unsupported-sqrt':'double CP(speed,"quantity.velocity") check(double v CP(speed^2,"quantity.speed_squared")){return sqrt(v);}',
  }
  for name,body in bodies.items():
   src=a.out/(name+'.c');src.write_text('#include <math.h>\n#include "kernels.h"\n'+body+'\n');strict(name,src,expected=1)
  # Affine points cannot enter scalar product rules as displacements.
  src=a.out/'absolute-product.c';src.write_text('#include "kernels.h"\ndouble CP(length^2,"quantity.area") check(double p CP(length,"quantity.position") FISICS_PHYSICAL_ORIGIN("origin.world")){return p*p;}\n');strict('absolute-product',src,expected=1)
  reordered=json.loads(json.dumps(pol));reordered['catalog_paths']=[str(f/'catalog.json')];reordered['operation_rules'].reverse();rp=a.out/'reordered-policy.json';rp.write_text(json.dumps(reordered));strict('reordered',source,'cp_kinetic_2d',chosen=rp)
  bad=[]
  for label,mutate in [('duplicate',lambda x:x['operation_rules'].append(dict(x['operation_rules'][0],id='rule.duplicate'))),('unknown-kind',lambda x:x['operation_rules'][0]['operand_kinds'].__setitem__(0,'quantity.missing')),('wrong-result',lambda x:x['operation_rules'][0].__setitem__('result_kind','quantity.energy')),('old-v6',lambda x:x.__setitem__('schema_version',6))]:
   q=json.loads(json.dumps(pol));q['catalog_paths']=[str(f/'catalog.json')];mutate(q);dest=a.out/(label+'-policy.json');dest.write_text(json.dumps(q));run(label+'-policy',[a.compiler,*['--overlay=physics-semantics','--physical-semantics-policy',dest],'-I'+str(inc),'-I'+str(f),'-c',source,'-o',a.out/(label+'.o')],1);bad.append(str(dest))
  c=json.loads(json.dumps(catalog));g=next(x for x in c['quantity_kinds'] if x['id']=='quantity.gravitational_constant');g['dimension_vector']=[2,-1,-2,0,0,0,0,0];g['dimension_text']='length^2/mass/time^2';cat=a.out/'wrong-catalog.json';cat.write_text(json.dumps(c));q=json.loads(json.dumps(pol));q['catalog_paths']=[str(cat)];dest=a.out/'wrong-dimension-policy.json';dest.write_text(json.dumps(q));run('canonical-dimension',[a.compiler,'--overlay=physics-semantics','--physical-semantics-policy',dest,'-I'+str(inc),'-I'+str(f),'-c',source,'-o',a.out/'bad-dim.o'],1)
  v1={k:v for k,v in pol.items() if k in ['schema','schema_version','policy_id','catalog_paths','linear_kinds','affine_pairs','origins']};v1['schema_version']=1;v1['catalog_paths']=[str(f/'catalog.json')];old=a.out/'v1.json';old.write_text(json.dumps(v1));result['allocation']=json.loads(run('allocation',[a.probe,old,policy,bad[0]]))
  if a.old_compiler:run('old-compiler-refuses-new-policy',[a.old_compiler.resolve(),*flags,'-c',source,'-o',a.out/'old.o'],1)
  numeric=a.out/'numeric.c';numeric.write_text('#include "kernels.h"\n#include <assert.h>\n#include <stdio.h>\nint main(void){double e CP(energy,"quantity.energy")=25;double mu CP(length^3/time^2,"quantity.gravitational_parameter")=6;double pe CP(energy,"quantity.energy")=-8;double f CP(force,"quantity.force")=24.0/9;double a CP(acceleration,"quantity.acceleration")=2;assert(cp_kinetic_2d(2,3,4)==e);assert(cp_mu(2,3)==mu);assert(cp_pair_potential(6,4,3)==pe);assert(cp_force(6,4,9)==f);assert(cp_acceleration(8,4)==a);printf("%zu %zu %g %g\\n",sizeof(double),sizeof(void*),cp_kinetic_2d(2,3,4),cp_pair_potential(6,4,3));return 0;}\n')
  outputs=[]
  for name,tool,opts in [('clang','clang',[]),('base',a.compiler,[]),('units',a.compiler,['--overlay=physics-units','-DCOSMIC_UNITS=1']),('physical',a.compiler,['-DCOSMIC_PHYSICAL=1','--overlay=physics-semantics','--physical-semantics-policy',policy])]:
   exe=a.out/name;run(name+'-build',[tool,*opts,'-I'+str(inc),'-I'+str(f),'-ffp-contract=off',numeric,'-o',exe]);outputs.append(run(name+'-run',[exe]))
  assert len(set(outputs))==1;assert all(sha(k)==v for k,v in pins.items()),'source drift';result.update(passed=True,ordered_rules=len(pol['operation_rules']),strict_prototypes=selections,numerical_layout_output=outputs[0]);print('Cosmic finite scalar contracts PASS',len(result['commands']),'commands')
 finally:(a.out/'results.json').write_text(json.dumps(result,indent=2)+'\n')
if __name__=='__main__':main()
