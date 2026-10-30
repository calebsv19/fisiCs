#!/usr/bin/env python3
"""Finite Cosmic 3D double norm/products, exact unary receipts and frames."""
from pathlib import Path
import argparse,copy,hashlib,json,os,resource,subprocess
ROOT=Path(__file__).resolve().parents[2]
def main():
 p=argparse.ArgumentParser(description=__doc__)
 for n in ['compiler','probe','out']:p.add_argument('--'+n,type=Path,required=True)
 p.add_argument('--old-compiler',type=Path);a=p.parse_args()
 for k,v in vars(a).items():
  if isinstance(v,Path):setattr(a,k,v.resolve())
 a.out.mkdir(parents=True,exist_ok=False);(a.out/'tmp').mkdir();fix=ROOT/'tests/fixtures/physical_semantics/cosmic_spatial';policy=fix/'policy.json';catalog=fix/'catalog.json';model=json.loads(policy.read_text());cat=json.loads(catalog.read_text());dims={r['id']:r['dimension_text'] for r in cat['quantity_kinds']};axial={'quantity.angular_rate','quantity.specific_angular_momentum','quantity.orbital_angular_momentum'}
 pins={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in [a.compiler,a.probe,policy,catalog]};result=dict(passed=False,commands=[],inputs=pins,selections={});env=dict(os.environ,FISICS_MAX_PROCS='0',PYTHONDONTWRITEBYTECODE='1',TMPDIR=str(a.out/'tmp'));env.pop('FISICS_OVERLAY',None)
 def no_core():resource.setrlimit(resource.RLIMIT_CORE,(0,0))
 def run(n,cmd,code=0):
  cmd=list(map(str,cmd));q=subprocess.run(cmd,cwd=ROOT,env=env,text=True,capture_output=True,timeout=180,preexec_fn=no_core);(a.out/(n+'.log')).write_text(q.stdout+q.stderr);result['commands'].append(dict(id=n,command=cmd,exit=q.returncode,expected=code));assert q.returncode==code,(n,q.returncode,(q.stdout+q.stderr)[-2400:]);return q.stdout
 pre='typedef struct V {double x,y,z;} V;\n'
 def attr(k,rep=True,frame='world'):
  s='[[fisics::dim('+dims[k]+')]] [[fisics::quantity_kind("'+k+'")]]'
  if rep:s+=' [[fisics::representation("rep.cosmic.'+('axial' if k in axial else 'polar')+'")]] [[fisics::frame("frame.'+frame+'")]]'
  if k=='quantity.position':s+=' [[fisics::physical_origin("origin.'+frame+'")]]'
  return s
 flags=['--overlay=physics-semantics','--physical-semantics-policy',policy]
 def strict(n,body,code=0,pol=policy,trust=True):
  src=a.out/(n+'.c');src.write_text(pre+body+'\n');dest=a.out/(n+'.json');f=list(flags);f[f.index(policy)]=pol;cmd=[a.compiler,*f,'--physical-semantics-function','check','--physical-semantics-strict']
  if trust:cmd+=['--physical-semantics-allow-literal-assumptions','--physical-semantics-allow-declared-boundaries']
  run(n,cmd+['--emit-physical-semantics-json',dest,src],code)
  if dest.exists():
   d=json.loads(dest.read_text());s=d['selection'];assert s['accepted']==(code==0)
   if code==0:assert s['checked']>0 and s['blocked']==0 and s['enumeration_complete'];result['selections'][n]=s
  return src,dest
 try:
  examples={}
  for h in model['shape_helpers']:
   ks=h['operand_kinds'];k=h['result_kind'];rr=h['result_representation'];ret='V' if rr else 'double';params=','.join('V '+chr(97+i)+' '+attr(q) for i,q in enumerate(ks));out=attr(k,bool(rr));decl=ret+' helper('+params+') '+out+' [[fisics::physical_operation("'+h['id']+'")]];\n';body=decl+ret+' check('+params+') '+out+' {V copy '+attr(ks[0])+'=a;return helper(copy'+(',b' if len(ks)==2 else '')+');}'
   label=h['id'].replace('.','-');strict(label,body);examples[h['id']]=body
  norm=examples['helper.cosmic.norm_velocity'];cross=examples['helper.cosmic.cross_specific_angular']
  negatives={
   'wrong-frame':norm.replace('check(V a '+attr('quantity.velocity'),'check(V a '+attr('quantity.velocity',frame='planar')),
   'missing-frame':norm.replace('check(V a '+attr('quantity.velocity'),'check(V a '+attr('quantity.velocity').replace(' [[fisics::frame("frame.world")]]','')),
   'wrong-axis':norm.replace('return helper(copy);','copy.x=copy.y;return helper(copy);'),
   'scalar-laundering':norm.replace('return helper(copy);','double erased [[fisics::dim(length/time)]] [[fisics::quantity_kind("quantity.velocity")]]=copy.x;return helper(copy);'),
   'function-pointer':norm.replace('return helper(copy);','double (*fn)(V)=helper;return fn(copy);'),
   'wrong-angular-parity':examples['helper.cosmic.norm_specific_angular_momentum'].replace('rep.cosmic.axial','rep.cosmic.polar'),
   'wrong-product-order':cross.replace('return helper(copy,b);','return helper(b,copy);'),
   'wrong-source-kind':norm.replace('check(V a '+attr('quantity.velocity'),'check(V a '+attr('quantity.acceleration')),
  }
  for n,b in negatives.items():
   assert b!=norm if n!='wrong-product-order' else b!=cross,('mutation anchor',n)
   strict(n,b,1)
  strict('implementation-trust-required',norm,1,trust=False)
  transforms={}
  for t in model['frame_transforms']:
   k=t['input_kind'];decl='V move(V a '+attr(k,frame='planar')+') '+attr(k)+' [[fisics::frame_transform("'+t['id']+'")]];\n';body=decl+'V check(V a '+attr(k,frame='planar')+') '+attr(k)+' {V copy '+attr(k,frame='planar')+'=a;return move(copy);}';strict(t['id'].replace('.','-'),body);transforms[k]=body
  body=transforms['quantity.velocity'];strict('transform-wrong-direction',body.replace('check(V a '+attr('quantity.velocity',frame='planar'),'check(V a '+attr('quantity.velocity')),1)
  strict('transform-id-removed',body.replace(' [[fisics::frame_transform("transform.cosmic.planar_to_world.velocity")]]',''),1)
  # Full supported unary shape signatures survive receipt write/read and tamper refusal.
  receipts=[]
  for n in ['one','two']:
   src,dest=strict('join-'+n,norm.replace('check(','check('))
   receipts.append(dest)
  run('unary-receipt-roundtrip',[a.compiler,'--physical-semantics-check',*receipts])
  d=json.loads(receipts[0].read_text());entry=next(x for x in d['declarations'] if x.get('shape_helper'));entry['shape_helper']='helper.cosmic.norm_acceleration';bad=a.out/'receipt-tamper.json';bad.write_text(json.dumps(d));run('unary-receipt-tamper',[a.compiler,'--physical-semantics-check',bad,receipts[1]],1)
  # Invalid model controls reach the finite table, not a dimension-only result inference.
  badpol=[]
  def badmodel(n,edit):
   m=copy.deepcopy(model);m['catalog_paths']=[str(catalog)];edit(m);path=a.out/(n+'-policy.json');path.write_text(json.dumps(m));strict(n+'-refuse',norm,1,path);badpol.append(path)
  badmodel('norm-extra-argument',lambda m:m['shape_helpers'][0]['operand_kinds'].append('quantity.displacement'))
  badmodel('norm-angle-result',lambda m:next(h for h in m['shape_helpers'] if h['id']=='helper.cosmic.norm_ratio').__setitem__('result_kind','quantity.angle'))
  badmodel('norm-float-storage',lambda m:m['representations'][0].__setitem__('scalar','float'))
  badmodel('norm-field-order',lambda m:m['representations'][0].__setitem__('fields',['y','x','z']))
  badmodel('cross-wrong-parity',lambda m:next(h for h in m['shape_helpers'] if h['operation']=='cross').__setitem__('result_representation','rep.cosmic.polar'))
  badmodel('unknown-kind',lambda m:next(h for h in m['shape_helpers'] if h['operation']=='norm').__setitem__('operand_kinds',['quantity.energy']))
  badmodel('duplicate',lambda m:m['shape_helpers'].append(dict(m['shape_helpers'][0])))
  result['allocation']=json.loads(run('allocation',[a.probe,ROOT/'tests/fixtures/physical_semantics/policy.json',policy,badpol[-1]]))
  m=copy.deepcopy(model);m['catalog_paths']=[str(catalog)];m['shape_helpers'].reverse();m['frame_transforms'].reverse();reverse=a.out/'reverse-policy.json';reverse.write_text(json.dumps(m));strict('reordered-model',norm,pol=reverse)
  if a.old_compiler:run('old-compiler-refuses-new-model',[a.old_compiler,*flags,'-c',a.out/'helper-cosmic-norm_velocity.c','-o',a.out/'old.o'],1)
  cmd=['python3','-B',ROOT/'tests/integration/run_physical_cosmic_math_contracts.py','--fixture','cosmic_spatial','--compiler',a.compiler,'--probe',a.probe,'--out',a.out/'scalar-model']
  run('all-scalar-rules',cmd)
  assert all(hashlib.sha256(Path(p).read_bytes()).hexdigest()==h for p,h in pins.items())
  result.update(passed=True,shape_helpers=len(model['shape_helpers']),transforms=len(model['frame_transforms']));print('Cosmic spatial model/contracts PASS',len(result['commands']),'commands plus scalar model gate')
 finally:(a.out/'results.json').write_text(json.dumps(result,indent=2)+'\n')
if __name__=='__main__':main()
