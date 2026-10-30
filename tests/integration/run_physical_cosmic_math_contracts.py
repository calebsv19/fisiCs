#!/usr/bin/env python3
"""Finite unframed scalar norms/atan2, policy compatibility and refusal controls."""
from pathlib import Path
import argparse,json,hashlib,os,subprocess,resource
ROOT=Path(__file__).resolve().parents[2]
def main():
 p=argparse.ArgumentParser(description=__doc__)
 for k in ['compiler','probe','out']:p.add_argument('--'+k,type=Path,required=True)
 p.add_argument('--old-compiler',type=Path);p.add_argument('--fixture',choices=['cosmic_planar','cosmic_encounter','cosmic_spatial','cosmic_population','cosmic_interaction'],default='cosmic_planar');a=p.parse_args()
 for k,v in vars(a).items():
  if isinstance(v,Path):setattr(a,k,v.resolve())
 a.out.mkdir(parents=True,exist_ok=False);(a.out/'tmp').mkdir();f=ROOT/'tests/fixtures/physical_semantics'/a.fixture;policy=f/'policy.json';catalog=f/'catalog.json';pol=json.loads(policy.read_text());cat=json.loads(catalog.read_text());dims={x['id']:x['dimension_text'] for x in cat['quantity_kinds']};pins={str(x):hashlib.sha256(x.read_bytes()).hexdigest() for x in [policy,catalog,a.compiler]};result=dict(passed=False,commands=[],inputs=pins)
 env=dict(os.environ,FISICS_MAX_PROCS='0',PYTHONDONTWRITEBYTECODE='1',TMPDIR=str(a.out/'tmp'));env.pop('FISICS_OVERLAY',None)
 def no_core():resource.setrlimit(resource.RLIMIT_CORE,(0,0))
 def run(label,cmd,expected=0):
  cmd=list(map(str,cmd));q=subprocess.run(cmd,cwd=ROOT,env=env,capture_output=True,text=True,timeout=120,preexec_fn=no_core);(a.out/(label+'.log')).write_text(q.stdout+q.stderr);result['commands'].append(dict(id=label,command=cmd,exit=q.returncode,expected=expected));assert q.returncode==expected,(label,q.returncode,(q.stdout+q.stderr)[-1800:]);return q.stdout
 pre='#include <math.h>\n#define Q(d,k) [[fisics::dim(d)]] [[fisics::quantity_kind(k)]]\n'
 flags=['--overlay=physics-semantics','--physical-semantics-policy',policy,'-I'+str(ROOT/'include')]
 def strict(label,body,expected=0,model=policy):
  src=a.out/(label+'.c');src.write_text(pre+body+'\n');dest=a.out/(label+'.json');opts=list(flags);opts[opts.index(policy)]=model
  run(label,[a.compiler,*opts,'--physical-semantics-function','check','--physical-semantics-strict','--emit-physical-semantics-json',dest,src],expected)
  if expected==0:
   d=json.loads(dest.read_text());s=d['selection'];assert s['accepted'] and s['enumeration_complete'] and s['blocked']==0 and s['permitted_trust']==0
  return src
 try:
  for i,row in enumerate(pol['operation_rules']):
   op=row['operation'];aa=row['operand_kinds'];kk=row['result_kind'];params=','.join('double '+chr(97+j)+' Q('+dims[k]+',"'+k+'")' for j,k in enumerate(aa));expr=op+'('+','.join(chr(97+j) for j in range(len(aa)))+')' if op in ['sqrt','hypot','atan2','fmax','fmin','fabs'] else 'a'+('*' if op=='multiply' else '/')+'b';body='double check('+params+') Q('+dims[kk]+',"'+kk+'") {return '+expr+';}';strict('row-'+str(i),body)
  negatives={
   'raw-angle':'double check(double a,double b) Q(1,"quantity.angle"){return atan2(a,b);}',
   'raw-sqrt':'double check(double a) Q(length,"quantity.geometric_length"){return sqrt(a);}',
   'specific-not-speed':'double check(double a Q(speed^2,"quantity.specific_energy")) Q(speed,"quantity.velocity"){return sqrt(a);}',
   'angle-not-ratio':'double check(double a Q(length,"quantity.geometric_length"),double b Q(length,"quantity.geometric_length")) Q(1,"quantity.ratio"){return atan2(a,b);}',
   'point-not-displacement':'double check(double a Q(length,"quantity.position") [[fisics::physical_origin("origin.world")]]) Q(length,"quantity.geometric_length"){return hypot(a,a);}',
   'framed-norm-erasure':'double check(double a Q(speed,"quantity.velocity") [[fisics::frame("frame.world")]] [[fisics::component_axis("axis.x")]]) Q(speed,"quantity.velocity"){return hypot(a,a);}',
   'mixed-frame-norm':'double check(double a Q(speed,"quantity.velocity") [[fisics::frame("frame.world")]] [[fisics::component_axis("axis.x")]],double b Q(speed,"quantity.velocity") [[fisics::frame("frame.local")]] [[fisics::component_axis("axis.x")]]) Q(speed,"quantity.velocity"){return hypot(a,b);}',
   'function-pointer':'double check(double a Q(length^2,"quantity.area")) Q(length,"quantity.geometric_length"){double (*fn)(double)=sqrt;return fn(a);}',
   'user-definition':'double sqrt(double a){return a;} double check(double a Q(length^2,"quantity.area")) Q(length,"quantity.geometric_length"){return sqrt(a);}',
   'unsupported-float-name':'double custom_norm(double a);double check(double a Q(length^2,"quantity.area")) Q(length,"quantity.geometric_length"){return custom_norm(a);}',
  }
  if a.fixture=='cosmic_population':
   negatives.update({
    'external-not-speed':'double check(double a Q(speed^2,"quantity.specific_external_potential")) Q(speed,"quantity.velocity"){return sqrt(a);}',
    'external-not-specific-orbit':'double check(double a Q(speed^2,"quantity.specific_external_potential")) Q(speed^2,"quantity.specific_energy"){return a;}',
    'external-energy-not-phi':'double check(double a Q(energy,"quantity.external_potential_energy")) Q(speed^2,"quantity.specific_external_potential"){return a;}',
    'external-not-closed-energy':'double check(double a Q(energy,"quantity.external_potential_energy")) Q(energy,"quantity.energy"){return a;}',
    'external-energy-unweighted':'double check(double a Q(speed^2,"quantity.specific_external_potential")) Q(energy,"quantity.external_potential_energy"){return a;}',
    'halo-center-origin':'double check(double a Q(length,"quantity.position") [[fisics::physical_origin("origin.world")]],double b Q(length,"quantity.position") [[fisics::physical_origin("origin.local")]]) Q(length,"quantity.displacement"){return a-b;}',
    'arbitrary-force-hook':'double check(double (*hook)(double),double a Q(force,"quantity.force")) Q(force,"quantity.force"){return hook(a);}',
   })
  if a.fixture=='cosmic_interaction':
   def energy(form):return 'Q(energy,"quantity.energy") [[fisics::energy_form("energy.'+form+'")]]'
   for source,target in [('elastic','damage_loss'),('kinetic','preparation_loss'),('contact_loss','thermal'),('damage_loss','thermal'),('preparation_loss','elastic')]:
    negatives['no-relabel-'+source+'-'+target]='double check(double a '+energy(source)+') '+energy(target)+'{return a;}'
   negatives['no-restore-storage']='double check(double a Q(energy,"quantity.energy")) '+energy('elastic')+'{return a;}'
   negatives['raw-loss-hook']='double check(double (*fn)(double),double a '+energy('elastic')+') '+energy('damage_loss')+'{return fn(a);}'
  for label,body in negatives.items():strict(label,body,1)
  bad=[]
  def model(label,change):
   v=json.loads(json.dumps(pol));v['catalog_paths']=[str(catalog)];change(v);dest=a.out/(label+'.json');dest.write_text(json.dumps(v));bad.append(dest);return dest
  math_index=next(i for i,x in enumerate(pol['operation_rules']) if x['operation']=='sqrt')
  for label,change in [('duplicate',lambda v:v['operation_rules'].append(dict(v['operation_rules'][0],id='rule.duplicate'))),('wrong-sqrt-kind',lambda v:v['operation_rules'][math_index]['operand_kinds'].__setitem__(0,'quantity.specific_energy')),('old-v6',lambda v:v.__setitem__('schema_version',6)),('wrong-origin',lambda v:v['operation_rules'][math_index].__setitem__('origin_policy','preserve'))]:
   dest=model(label,change);strict(label+'-refusal','double check(void) Q(length,"quantity.geometric_length"){return 0;}',1,dest)
  v=json.loads(json.dumps(pol));v['catalog_paths']=[str(catalog)];v['operation_rules'].reverse();rev=a.out/'reversed-policy.json';rev.write_text(json.dumps(v));body='double check(double a Q(length^2,"quantity.area")) Q(length,"quantity.geometric_length"){return sqrt(a);}';strict('reversed',body,model=rev)
  # Round-trip the full operation model through supported scalar C signatures.
  # System math headers export unrelated pointer/callback declarations; their
  # whole-TU signature join is deliberately outside this scalar contract.
  receipts=[]
  for n in ['first','second']:
   src=a.out/('join-'+n+'.c');src.write_text('#define Q(d,k) [[fisics::dim(d)]] [[fisics::quantity_kind(k)]]\nextern double shared_mass(double a Q(mass,"quantity.mass")) Q(mass,"quantity.mass");\ndouble '+n+'(double a Q(mass,"quantity.mass")) Q(mass,"quantity.mass"){return a;}\n');dest=a.out/('join-'+n+'.json')
   run('join-'+n,[a.compiler,*flags,'--physical-semantics-function',n,'--physical-semantics-strict','--emit-physical-semantics-json',dest,src]);receipts.append(dest)
  run('receipt-roundtrip',[a.compiler,'--physical-semantics-check',*receipts])
  tampered=json.loads(receipts[0].read_text());tampered['operation_model']['rules'][0]['result_kind']='quantity.energy';bad_receipt=a.out/'tampered-receipt.json';bad_receipt.write_text(json.dumps(tampered));run('receipt-tamper-refusal',[a.compiler,'--physical-semantics-check',bad_receipt,receipts[1]],1)
  result['allocation']=json.loads(run('allocation',[a.probe,ROOT/'tests/fixtures/physical_semantics/policy.json',policy,bad[0]]))
  if a.old_compiler:run('old-compiler-refusal',[a.old_compiler,*flags,'-c',a.out/'reversed.c','-o',a.out/'old.o'],1)
  # atan2 units require like dimensions/concrete representations, and strict result is dimensionless.
  src=a.out/'atan-units.c';src.write_text('#include <math.h>\ndouble check(double a [[fisics::dim(length)]],double b [[fisics::dim(time)]]) [[fisics::dim(1)]]{return atan2(a,b);}\n');run('atan2-wrong-units',[a.compiler,'--overlay=physics-units','-c',src,'-o',a.out/'bad.o'],1)
  source=a.out/'atan-units-positive.c';source.write_text('#include <math.h>\ndouble check(double a [[fisics::dim(length)]] [[fisics::unit(meter)]],double b [[fisics::dim(length)]] [[fisics::unit(meter)]]) [[fisics::dim(1)]]{return atan2(a,b); }\n');report=a.out/'atan-units-positive.json';run('atan2-units-strict',[a.compiler,'--overlay=physics-units','--units-coverage-function','check','--units-coverage-strict','--emit-units-coverage-json',report,source]);assert json.loads(report.read_text())['strict_policy']['accepted']
  source=a.out/'atan-concrete.c';source.write_text((a.out/'atan-units-positive.c').read_text().replace('b [[fisics::dim(length)]] [[fisics::unit(meter)]]','b [[fisics::dim(length)]] [[fisics::unit(centimeter)]]'));run('atan2-concrete-refusal',[a.compiler,'--overlay=physics-units','-c',source,'-o',a.out/'concrete.o'],1)
  assert all(hashlib.sha256(Path(x).read_bytes()).hexdigest()==h for x,h in pins.items());result.update(passed=True,ordered_rules=len(pol['operation_rules']),negative_calls=len(negatives));print('Finite Cosmic math contracts PASS',len(result['commands']),'commands')
 finally:(a.out/'results.json').write_text(json.dumps(result,indent=2)+'\n')
if __name__=='__main__':main()
