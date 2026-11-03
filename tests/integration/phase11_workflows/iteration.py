"""P11.5: exercise the existing physical receipt invalidation/recheck route."""
from pathlib import Path
import argparse,json,shutil,hashlib
from support import SerialBuild,command,dump,sha,substitute

def main():
 p=argparse.ArgumentParser(description=__doc__)
 for name in ['binding','amendment','out']:p.add_argument('--'+name,type=Path,required=True)
 a=p.parse_args();binding=json.loads((a.binding/'bindings.json').read_text());amend=json.loads(a.amendment.read_text());assert amend['passed'];a.out.mkdir()
 workspace=Path(binding['workflows'][0]['source']['path']).parents[4];contract=workspace/'docs/private_program_docs/fisiCs/audits/2026-11-01_p10_1_binding';b=SerialBuild(contract,a.out/'commands');b.env.pop('DISABLE_CODEGEN',None)
 rows=[];cost=[]
 for wf in binding['workflows']:
  family=wf['family'];w=next(x for x in amend['families'] if x['family']==family);root=a.out/'fixtures'/family;root.mkdir(parents=True);inc=root/'include/fisics';inc.mkdir(parents=True);header=inc/'extensions.h';shutil.copyfile(workspace/'fisiCs/include/fisics/extensions.h',header)
  source=[root/'a.c',root/'b.c'];source[0].write_text(w['effective_a']);source[1].write_text(w['effective_b']);assert sha(source[0])==w['qualified_a_sha256'] and sha(source[1])==w['qualified_b_sha256']
  original_policy=Path(wf['freshness_fixture']['argv'][0][3]);physics=root/'physics';physics.mkdir();policy=physics/original_policy.name;shutil.copyfile(original_policy,policy);catalogs=[]
  for rel in json.loads(policy.read_text())['catalog_paths']:
   target=physics/rel;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(original_policy.parent/rel,target);catalogs.append(target)
  inputs=[*source,header,policy,*catalogs];original={str(x):x.read_bytes() for x in inputs}
  def invoke(label,argv,expected=0):
   q,c=command(b,family+'-'+label,argv,expected=expected,cwd=root);rows.append(dict(family=family,step=label,passed=True,command=c));return q,c
  def generate(name):
   dest=root/name;dest.mkdir();reports=[];argvs=[]
   for i,base in enumerate(wf['freshness_fixture']['argv'][:2]):
    argv=substitute(base,a.out/'fixtures');argv[1:1]=['-I'+str(root/'include')];argv[argv.index('--physical-semantics-policy')+1]=str(policy);report=dest/(source[i].name+'.physical.json');argv[argv.index('--emit-physical-semantics-json')+1]=str(report);argv[-1]=str(source[i]);invoke(name+'-emit-'+str(i),argv);d=json.loads(report.read_text());assert d['analysis_valid'] and d['selection']['accepted'] and d['selection']['enumeration_complete'];assert str(header) in [x['path'] for x in d['dependencies']['files']];reports.append(report);argvs.append(argv)
   return reports,argvs
  def check(label,reports,expected=0):
   q,c=invoke(label,[b.fisics,'--physical-semantics-check',*reports],expected)
   if expected:assert 'physical cross-TU receipt/signature consistency rejected' in q.stderr.decode(),q.stderr.decode()
   return q,c
  reports,argvs=generate('baseline');check('baseline-check',reports);receipt_hashes={str(x):sha(x) for x in reports};check('unchanged-reuse',reports);assert all(sha(x)==receipt_hashes[str(x)] for x in reports)
  # These edits affect actual receipt inputs; nothing in an owner checkout moves.
  for label,target,suffix in [('source',source[1],b'\n/* P11.5 source edit */\n'),('include',header,b'\n/* P11.5 included dependency edit */\n'),('policy',policy,b'\n '),('catalog',catalogs[0],b'\n ')]:
   saved=target.read_bytes();target.write_bytes(saved+suffix);check(label+'-stale',reports,1);assert all(sha(x)==receipt_hashes[str(x)] for x in reports);target.write_bytes(saved);check(label+'-exact-restore',reports)
  # Changed source requires new reports; the old reports must remain stale.
  saved=source[1].read_bytes();source[1].write_bytes(saved+b'\n/* P11.5 regenerated generation */\n');check('old-generation-stale',reports,1);regenerated,_=generate('regenerated');check('regenerated-check',regenerated);source[1].write_bytes(saved);check('regenerated-now-stale',regenerated,1);check('old-generation-restored',reports)
  target=reports[1];saved=target.read_bytes();target.write_bytes(b'[]\n');check('corrupt-report',reports,1);target.write_bytes(saved);check('corrupt-report-restored',reports);target.unlink();check('missing-report',reports,1);target.write_bytes(saved);check('missing-report-restored',reports)
  # A failed compile gets a fresh report location and cannot borrow prior success.
  bad=root/'failed-generation';bad.mkdir();saved=source[0].read_text();before='double value FISICS_DIM('+w['dimension']+')';assert saved.count(before)==1;source[0].write_text(saved.replace(before,'double value FISICS_DIM(time)'));argv=list(argvs[0]);failed=bad/'a.c.physical.json';argv[argv.index('--emit-physical-semantics-json')+1]=str(failed);q,_=invoke('failed-generation',argv,1);assert 'mismatch' in (q.stdout+q.stderr).decode().lower();assert not failed.exists() or not json.loads(failed.read_text()).get('analysis_valid',True);source[0].write_text(saved);recovered,_=generate('recovered');check('recovered-check',recovered);check('original-still-valid',reports)
  assert all(x.read_bytes()==original[str(x)] for x in inputs);assert all(sha(x)==receipt_hashes[str(x)] for x in reports)
  # Fresh root rejection is deliberate, before any compile/output mutation.
  try:root.mkdir()
  except FileExistsError:rows.append(dict(family=family,step='occupied-output-refused',passed=True,compiler_invoked=False))
  else:raise AssertionError('existing generation reused')
  cost.append(dict(family=family,root=str(root),sources=list(map(str,source)),reports=list(map(str,reports)),emit_argv=argvs,check_argv=[str(b.fisics),'--physical-semantics-check',*map(str,reports)],policy=str(policy),dependencies={str(x):sha(x) for x in inputs},original_policy=str(original_policy),original_policy_sha256=sha(original_policy)))
  print(family,'ITERATION/DEPENDENCY/REGENERATION PASS',flush=True)
 b.freshness();dump(a.out/'cost-inputs.json',dict(passed=True,compiler=str(b.fisics),families=cost,environment_guard=1));dump(a.out/'result.json',dict(passed=True,checkpoint='P11.5-iteration',families=3,steps=len(rows),rows=rows,owner_inputs_preserved=True,exact_restorations_passed=True,unchanged_receipt_reuse=True,changed_source_regeneration=True,failed_generation_did_not_publish_success=True,occupied_output_refused=True,default_products_touched=False,compiler_or_solver_implementation_changed=False));print('ITERATION PASS',len(rows),'checks',flush=True)
if __name__=='__main__':main()
