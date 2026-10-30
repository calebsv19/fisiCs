#!/usr/bin/env python3
"""Experimental, opt-in native Structural mass-path qualification.

Requires actual source plus a matching private frontend adapter. It does not
alter ordinary compiler strict coverage or emit runtime instrumentation.
"""
import argparse,hashlib,json,os,subprocess,sys,time
from pathlib import Path
from collections import Counter
from bridge import verify_path
from flow_engine import Engine,Refusal

from identity import canonical
from provenance import adapter_receipt,native_inputs,path_list,object_value,require

def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def merge_export(functions,doc,core):
 object_value(doc,'adapter export')
 if doc.get('schema')!='fisiCs.internal.resolved_ast.v1':raise Refusal('adapter schema mismatch')
 require(isinstance(doc.get('functions'),list),'adapter functions must be an array')
 for fn in doc['functions']:
  object_value(fn,'exported function');require(all(isinstance(fn.get(k),str) and fn[k] for k in ('file','name')) and isinstance(fn.get('body'),dict),'invalid exported function')
  if not Path(fn['file']).is_relative_to(core):continue
  old=functions.get(fn['name'])
  if old and canonical(old)!=canonical(fn):raise Refusal('ambiguous cross-TU function identity '+fn['name'])
  functions[fn['name']]=fn

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--adapter-receipt',type=Path,required=True);p.add_argument('--adapter',type=Path,required=True);p.add_argument('--compiler-root',type=Path,required=True)
 p.add_argument('--core-root',type=Path,required=True);p.add_argument('--out',type=Path,required=True)
 p.add_argument('--readset',type=Path,required=True)
 p.add_argument('--admit-planar-si-exclusive-owner',action='store_true',required=True)
 a=p.parse_args();f=a.compiler_root.resolve();c=a.core_root.resolve();out=a.out.resolve();start=time.monotonic()
 try:out.mkdir(parents=True,exist_ok=False)
 except OSError as e:print('output: '+str(e),file=sys.stderr);return 2
 before={};commands=[];stage='input'
 try:
  stage='provenance';native=adapter_receipt(a.adapter_receipt,f,a.adapter)
  stage='input'
  sdk=Path(subprocess.check_output(['xcrun','--show-sdk-path'],text=True).strip())/'usr/include';res=Path(subprocess.check_output(['clang','-print-resource-dir'],text=True).strip())/'include'
  inc=[f/'include',c/'src',c/'include',c.parent/'core_base/include',sdk,res]
  # Before/after identity guards bind owner inputs, adapter and authored contracts.
  roots=[c/'src',c/'include',c.parent/'core_base/include',f/'include',Path(__file__).parent]
  paths={x.resolve() for root in roots for x in root.rglob('*') if x.is_file() and x.suffix in ('.c','.h','.py','.json') and '__pycache__' not in str(x)}
  readset=path_list(json.loads(a.readset.read_text()),'readset')
  paths.update(Path(x).resolve() for x in readset)
  paths.add(a.adapter.resolve());paths.add(a.adapter_receipt.resolve());paths.add(a.readset.resolve());before={str(x):sha(x) for x in paths};deps=set();functions={};commands=[]
  names=['core_structural_frame_inertia','core_structural_frame_motion_integrate','core_structural_plastic_frame_joints','core_structural_fiber_beam_solve','core_structural_fiber_beam']
  stage='export'
  for n in names:
   target=out/(n+'.json');cmd=list(map(str,[a.adapter,c/'src'/(n+'.c'),target,*inc]));q=subprocess.run(cmd,capture_output=True,text=True,timeout=10,env={**os.environ,'FISICS_AST_SOURCE_ROOT':str(c)});(out/(n+'.log')).write_text(q.stdout+q.stderr);commands.append(dict(argv=cmd,exit=q.returncode,environment={'FISICS_AST_SOURCE_ROOT':str(c)}))
   if q.returncode:raise Refusal('frontend export failed: '+n)
   d=json.loads(target.read_text())
   merge_export(functions,d,c)
   for e in json.loads(Path(str(target)+'.deps.json').read_text())['edges']:deps.update((e['from'],e['to']))
  stage='qualifier';engine=Engine(functions);proof=verify_path(engine)
  proof.pop('mass');proof.pop('owner')
  after={path:sha(Path(path)) for path in before}
  if native['source_inputs']!=native_inputs(f):raise Refusal('native source changed during qualification')
  if before!=after:raise Refusal('source/tool identity changed during qualification')
  if any(path not in before for path in deps):raise Refusal('new unpinned include dependency; prepare a fresh reviewed readset')
  dependency_hashes={path:sha(Path(path)) for path in sorted(deps)}
  report=dict(schema='fisiCs.experimental.structural_mass_path.v1',accepted=True,native_build_provenance=dict(adapter_receipt_sha256=sha(a.adapter_receipt),source_root=native['source_root'],source_inputs=native['source_inputs'],frontend_sha256=native['frontend_sha256']),admitted_model=dict(nodes=8,joints=0,members=7,original_members=8,failed_member=3,fibers=16,capacity_dofs=27,fixed_dofs=[0,1,2],direct_frame=False,corotational=False,endpoint_load='NULL',cancellation_callback='NULL'),scope='selected native SI unforced non-corotational reduced-owner path, 8 nodes, 0 joints, 7 of 8 members, 16 fibers, 24 active DOFs',proof=proof,storage_effect_checks=dict(expanded_checks=len(engine.rows),explicit_layout_zero_admissions=len(engine.zeros),steps=engine.steps,scalar_summary_reuses=engine.scalar_reuses,scalar_summaries=len(engine.scalar_cache),matrix_source_summary_reuses=engine.matrix_reuses,matrix_source_summaries=len(engine.matrix_cache)),sites=[dict(file=k[0],line=k[1],column=k[2],operation=k[3],executions=v) for k,v in sorted(engine.rows.sites.items())],consumer_lineage_semantics='possible origins of non-annihilated expressions; not algebraic sensitivity or per-cell nonzero proof',authored_admissions=['fixed reviewed AST control/capability envelope outside selected interpreted physical regions; unreviewed envelope edits refuse','exclusive live owner and valid disjoint C subobjects; no intervening external owner operation','finite valid planar SI geometry, endpoint indices, orthonormal basis and materials','explicit local/output/owner TT/TR/RT/RR mass layouts, planar basis zero layout and native IEEE754 binary64 zero representation','existing kinetic/damping/work generalized quantity-kind and DOF helper interfaces; source bodies checked, not trusted getter results','successful solver/work ports for displacement/internal-force roots; solver numerical law not inferred','unforced endpoint_load is NULL; null cancellation callback; source-checked dispatch to actual evaluate receiver','native C math contracts hypot/sqrt/fabs/fmax, compiler infinity constants and fresh/disjoint core allocation/free contracts; those implementations are not checked'],ordinary_whole_function_strict_coverage='unchanged; unsupported original full functions remain refused',source_tool_hashes=before,dependency_hashes=dependency_hashes,commands=commands,seconds=time.monotonic()-start)
  (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print('Bounded actual assembly path accepted:',len(engine.rows),'expanded storage-effect checks; explicit roots/zero admissions remain separate.');return 0
 except (Refusal,KeyError,TypeError,ValueError,OSError,subprocess.SubprocessError) as e:
  site=getattr(locals().get('engine'),'last_site',None) or {}
  first_loss={k:site.get(k) for k in ('file','line','column','tag')}
  (out/'report.json').write_text(json.dumps(dict(accepted=False,stage=stage,diagnostic=type(e).__name__+': '+str(e),first_loss=first_loss,commands=commands,source_tool_hashes=before),indent=2)+'\n');print(str(e),file=sys.stderr);return 1
if __name__=='__main__':sys.exit(main())
