#!/usr/bin/env python3
"""Explicit source-mutant regression gate for the experimental qualifier.

Writes only a fresh output directory. Uses the actual current core source and
fresh resolved AST exports. Does not modify core source or select a compiler.
"""
import argparse,json,os,subprocess,time
from pathlib import Path
from bridge import verify_path
from provenance import adapter_receipt
from flow_engine import Engine,Refusal
from identity import canonical
from copy import deepcopy
from bridge import walk,params

def main():
 p=argparse.ArgumentParser(description=__doc__)
 for n in ('adapter','compiler-root','core-root','out'):p.add_argument('--'+n,type=Path,required=True)
 p.add_argument('--adapter-receipt',type=Path,required=True)
 a=p.parse_args();adapter_receipt(a.adapter_receipt,a.compiler_root,a.adapter);c=a.core_root.resolve();f=a.compiler_root.resolve();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False)
 sdk=Path(subprocess.check_output(['xcrun','--show-sdk-path'],text=True).strip())/'usr/include';res=Path(subprocess.check_output(['clang','-print-resource-dir'],text=True).strip())/'include'
 inc=[f/'include',c/'src',c/'include',c.parent/'core_base/include',sdk,res]
 names=['core_structural_frame_inertia','core_structural_frame_motion_integrate','core_structural_plastic_frame_joints','core_structural_fiber_beam_solve','core_structural_fiber_beam']
 def export(src,target):
  cmd=list(map(str,[a.adapter,src,target,*inc]));q=subprocess.run(cmd,capture_output=True,text=True,timeout=15,env={**os.environ,'FISICS_AST_SOURCE_ROOT':str(c)})
  target.with_suffix('.log').write_text(q.stdout+q.stderr)
  return q,cmd
 base={}
 for name in names:
  target=out/(name+'.json');q,cmd=export(c/'src'/(name+'.c'),target)
  if q.returncode:raise RuntimeError('baseline frontend failed: '+name)
  base[name]=json.loads(target.read_text())
 binding=deepcopy(next(fn for fn in base['core_structural_frame_inertia']['functions'] if fn['name']=='cd_consistent_mass'))
 renumbered=deepcopy(binding)
 def renumber(n):
  if isinstance(n,dict):
   for k,v in n.items():
    if k in ('id','origin','key'):n[k]='fresh:'+v
    else:renumber(v)
  elif isinstance(n,list):
   for v in n:renumber(v)
 renumber(renumbered)
 changed=deepcopy(binding);pp=params(changed)
 use=next(n for n in walk(changed['body']) if n.get('tag')=='id' and n.get('origin')==pp[1]['key']);use['origin']=pp[2]['key']
 identity_checks=dict(transient_renumbering_equal=canonical(binding)==canonical(renumbered),different_binding_rejected=canonical(binding)!=canonical(changed))
 if not all(identity_checks.values()):raise RuntimeError('resolved binding normalization failed')
 results=[]
 cases=json.loads(Path(__file__).with_name('soundness_cases.json').read_text())+json.loads(Path(__file__).with_name('predecessor_cases.json').read_text())
 if len(cases)!=106:raise RuntimeError('source corpus count changed')
 for case in cases:
  directory=out/case['id'];directory.mkdir();docs=dict(base);front=0;commands=[]
  if case['tu']:
   source=(c/'src'/(case['tu']+'.c')).read_text()
   if case['old'] not in source:raise RuntimeError('source mutation anchor absent: '+case['id'])
   source=case.get('prefix','')+source.replace(case['old'],case['new'],1)
   for old,new in case.get('extra_replacements',[]):
    if old not in source:raise RuntimeError('extra source mutation anchor absent: '+case['id'])
    source=source.replace(old,new)
   src=directory/(case['tu']+'.c');src.write_text(source);target=directory/'ast.json';q,cmd=export(src,target);front=q.returncode;commands.append(cmd)
   if not front:docs[case['tu']]=json.loads(target.read_text())
  accepted=False;diagnostic=None;stage='frontend' if front else 'qualifier';start=time.monotonic();engine=None
  if not front:
   functions={}
   # A mutant replaces exactly its owning TU. Duplicate declarations must still
   # have identical resolved binding structure, as in the real CLI.
   try:
    for doc in docs.values():
     for fn in doc['functions']:
      if not Path(fn['file']).is_relative_to(c) and fn['file']!=str(directory/(str(case['tu'])+'.c')):continue
      old=functions.get(fn['name'])
      if old and canonical(old)!=canonical(fn):raise Refusal('ambiguous cross-TU function identity '+fn['name'])
      functions[fn['name']]=fn
    engine=Engine(functions);verify_path(engine);accepted=True
   except (Refusal,KeyError,TypeError,ValueError) as e:diagnostic=type(e).__name__+': '+str(e)
  # Every negative must be valid for the frontend, not an unrelated C error.
  row=dict(id=case['id'],expected=case['expected'],accepted=accepted,frontend_exit=front,stage=stage,diagnostic=diagnostic,passed=front==0 and accepted==case['expected'],seconds=time.monotonic()-start,first_loss={k:(getattr(engine,'last_site',None) or {}).get(k) for k in ('file','line','column','tag')},commands=commands)
  results.append(row);print(row['id'],row['passed'],diagnostic or '',flush=True)
 result=dict(passed=all(x['passed'] for x in results),identity_checks=identity_checks,cases=results)
 (out/'results.json').write_text(json.dumps(result,indent=2)+'\n');return not result['passed']
if __name__=='__main__':raise SystemExit(main())
