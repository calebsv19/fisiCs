"""P11.4 only: the exact twelve P11.1 controls and their applicable profiles."""
from pathlib import Path
import argparse,json,pickle,shutil,sys,importlib.util,gzip
from support import ROOT,INITIAL,PROFILES,dump,sha,command,compile_copy,profile_flags,selected_report,native_reference,substitute

def owner_copy(b,wf,name):
 family=wf['family'];origin=b.core/('core_thermal' if family=='T1' else 'core_structural');dest=b.work/'owner-copies'/name;dest.mkdir(parents=True)
 if family=='T1':shutil.copytree(origin/'include',dest/'include');shutil.copytree(origin/'src',dest/'src')
 else:
  (dest/'src').mkdir()
  for p in (origin/'src').glob('*.h'):shutil.copyfile(p,dest/'src'/p.name)
  p=Path(wf['source']['path']);shutil.copyfile(p,dest/'src'/p.name)
 return dest,origin

def dimensions(b,wf,out):
 family=wf['family'];control=next(c for c in b.p11_binding['controls'] if c['id']==family+'-A2');copy,origin=owner_copy(b,wf,'wrong-dimension-'+family);site=control['site'];header=copy/Path(site['path']).relative_to(origin);original=header.read_text();assert sha(site['path'])==site['sha256'] and original.count(site['before'])==1;header.write_text(original.replace(site['before'],site['after']));_,rows=compile_copy(b,family,copy,'wrong-dimension-'+family,only_source=Path(wf['source']['path']).name,reject=True)
 header.write_text(original);assert sha(header)==site['sha256'];restored=[];oldinc=b.inc;b.inc=['-I'+str(copy/'src'),'-I'+str(copy/'include')]+oldinc
 try:
  for profile in ['fisics-units','fisics-physical']:
   folder=b.work/'restored-dimension'/family/profile;folder.mkdir(parents=True);source=copy/'src'/Path(wf['source']['path']).name;prepared=b.prepared(source,family,profile,folder/'restored.c');q,cmd=command(b,'restore-dimension-'+family+'-'+profile,[b.fisics,*profile_flags(b,family,profile),'-c',prepared,'-o',folder/'restored.o']);item=dict(profile=profile,header_sha256=sha(header),passed=True,command=cmd)
   if profile=='fisics-physical':item['strict']=selected_report(b,wf,prepared,folder/'restored.physical.json',label='restore-dimension-strict-'+family)
   restored.append(item)
 finally:b.inc=oldinc
 answer=[dict(**r,control=control['id'],family=family,kind='wrong_dimension',intended_detector='legal C in Clang/base; dimension mismatch in units/physical',restored_exact_bytes=True) for r in rows];dump(out/(family+'-dimensions.json'),dict(passed=True,rows=answer,site=site,restored=restored));return answer

def numerics(b,wf,out,products,thermal_native,sources,p105):
 family=wf['family'];control=next(c for c in b.p11_binding['controls'] if c['id']==family+'-A3');mutant=control['mutant_binding'];id=mutant['id'];rows=[];restorations=[]
 if family in ['T1','D1']:
  exes=p105.native_copy(b,id)
  for profile in PROFILES:
   rc,h,states,err,run=p105.execute(b,'P114-'+id+'-'+profile,exes[profile],2 if family=='T1' else 0,0,'thermal' if family=='T1' else 'dynamic');assert rc==0,('unrelated native refusal is not detector',id,err);case=next(c for c in b.contract['cases'] if c['id']==wf['case']);check=p105.reference('thermal' if family=='T1' else 'dynamic',h,states,case);expected='cell_relative' if family=='T1' else 'ledger_reconstruction_j';assert not check['passed'] and expected in check['failure'];rows.append(dict(control=control['id'],family=family,profile=profile,kind='dimensionally_valid_defect',passed=True,detected=True,mutant=id,intended_detector=expected,reference=check,run=run))
  copy=b.work/'copies'/id;origin=b.core/('core_thermal' if family=='T1' else 'core_structural');target=copy/Path(mutant['site']['path']).relative_to(origin);saved=out/'mutated-inputs'/id/target.name;saved.parent.mkdir(parents=True);shutil.copyfile(target,saved);assert sha(saved)==json.loads((b.out/(id+'-mutation.json')).read_text())['mutated_sha256'];target.write_bytes(Path(mutant['site']['path']).read_bytes());assert sha(target)==mutant['site']['sha256'];exes,_=compile_copy(b,family,copy,id+'-restored')
  for profile in PROFILES:restorations.append(native_reference(b,wf,profile,exes[profile],'restored-'+id+'-'+profile))
 else:
  clone=b.work/'p11-adapter-copies'/id;shutil.copytree(b.structural/'tools',clone/'tools',ignore=shutil.ignore_patterns('__pycache__','*.pyc'));shutil.copyfile(b.structural/'VERSION',clone/'VERSION');(clone/'tests').mkdir();shutil.copyfile(b.structural/'tests/thermal_frame_reference.py',clone/'tests/thermal_frame_reference.py');target=clone/'tools/ss9/assembly_cases.py';original=target.read_text();old,new=p105.CHANGES[id];assert original.count(old)==1 and sha(target)==mutant['site']['sha256'];target.write_text(original.replace(old,new));saved=out/'mutated-inputs'/id/target.name;saved.parent.mkdir(parents=True);shutil.copyfile(target,saved);worker=ROOT.parent/'phase10_mutants/bridge_worker.py'
  for state in ['mutated','restored']:
   if state=='restored':target.write_text(original);assert sha(target)==mutant['site']['sha256']
   for profile in PROFILES:
    dest=out/('M09-'+state+'-'+profile);argv=['/opt/homebrew/bin/python3','-B',worker,'--structural',clone,'--canonical-structural',b.structural,'--initial',INITIAL,'--native',products[profile]['structural'],'--thermal-native',thermal_native,'--sources',sources,'--binding',b.binding,'--case','TS10-A','--alpha','1.2e-5','--load-factor','1','--mutant','M09' if state=='mutated' else 'control','--out',dest];q,cmd=command(b,'M09-'+state+'-'+profile,argv,timeout=1200);v=json.loads(q.stdout);assert v['passed'] and v['detected']==(state=='mutated')
    item=dict(control=control['id'],family=family,profile=profile,kind='dimensionally_valid_defect',passed=True,mutant='M09',detected=state=='mutated',reference=v,command=cmd,intended_detector='authored Tref and independent free expansion')
    (rows if state=='mutated' else restorations).append(item)
 dump(out/(family+'-numerics.json'),dict(passed=True,rows=rows,restored=restorations,bound_mutant=mutant));print(family,'NUMERICAL DEFECT/RESTORE PASS',flush=True);return rows

def freshness(b,wf,out):
 family=wf['family'];folder=out/'freshness-scalar';folder.mkdir(exist_ok=True);root=folder/family;root.mkdir();fixture=wf['freshness_fixture'];a=root/'a.c';c=root/'b.c'
 # The bound witness tests a shared scalar signature, not unsupported owner ABI
 # records imported incidentally by the private helper header.
 dim='1' if family=='TS1' else 'energy';kind='quantity.mechanical_strain' if family=='TS1' else 'quantity.energy';typ='FISICS_DIM('+dim+') FISICS_QUANTITY_KIND("'+kind+'")';anchor='p11_'+family.lower()+'_anchor';peer='p11_'+family.lower()+'_peer';prefix='#include <fisics/extensions.h>\n';aa=prefix+'double '+typ+' '+anchor+'(double value '+typ+'){return value;}\n';bb=prefix+'extern double '+typ+' '+anchor+'(double value '+typ+');\ndouble '+typ+' '+peer+'(double value '+typ+'){return value;}\n';a.write_text(aa);c.write_text(bb);dump(root/'fixture-amendment.json',dict(original_a=fixture['a'],original_b=fixture['b'],effective_a=aa,effective_b=bb,dimension=dim,quantity_kind=kind,owner_header_sha256=wf['header']['sha256'],reason='Private full header imports unsupported shared record/pointer interfaces. Keep checker refusal; isolate the already-bound scalar signature only.',checker_or_policy_changed=False))
 args=[substitute(argv,folder) for argv in fixture['argv']];commands=[]
 for i,argv in enumerate(args):q,cmd=command(b,family+'-fresh-'+str(i),argv);commands.append(cmd)
 reports=[root/'a.c.physical.json',root/'b.c.physical.json'];before={str(p):sha(p) for p in reports};saved=c.read_bytes();c.write_bytes(saved+fixture['mutation']['append'].encode());q,cmd=command(b,family+'-stale-source',args[-1],expected=1);assert fixture['stderr_contains'] in q.stderr.decode();assert {str(p):sha(p) for p in reports}==before;commands.append(cmd);c.write_bytes(saved);q,cmd=command(b,family+'-restore-source',args[-1]);assert {str(p):sha(p) for p in reports}==before;commands.append(cmd)
 result=dict(control=family+'-A4',family=family,profile='fisics-physical',kind='stale_artifact',passed=True,intended_detector='fresh physical readset before shared signature comparison',fresh_pair_passed=True,stale_source_refused=True,exact_restore_passed=True,reports_preserved=True,source_sha256=sha(c),reports=before,commands=commands);dump(out/(family+'-freshness.json'),result);print(family,'FRESH/STALE/RESTORE PASS',flush=True);return result

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--healthy',type=Path,required=True);p.add_argument('--out',type=Path,required=True);p.add_argument('--resume',action='store_true');a=p.parse_args();a.out.mkdir(exist_ok=a.resume);healthy=json.loads((a.healthy/'healthy/result.json').read_text());assert healthy['passed']
 with (a.healthy/'state.pickle').open('rb') as f:b,binding,products,thermal_native,sources=pickle.load(f)
 b.commands=json.loads((b.out/'commands.json').read_text());b.p11_binding=binding;b.freshness();spec=importlib.util.spec_from_file_location('p105_bound',ROOT.parent/'phase10_mutants/qualify.py');p105=importlib.util.module_from_spec(spec);spec.loader.exec_module(p105);rows=[dict(r,kind='healthy') for r in healthy['rows']]
 for wf in binding['workflows']:
  family=wf['family'];d=a.out/(family+'-dimensions.json');n=a.out/(family+'-numerics.json');f=a.out/(family+'-freshness.json')
  if d.exists():v=json.loads(d.read_text());assert a.resume and v['passed'];rows.extend(v['rows'])
  else:rows.extend(dimensions(b,wf,a.out))
  print(family,'DIMENSION/RESTORE PASS',flush=True)
  if n.exists():v=json.loads(n.read_text());assert a.resume and v['passed'];rows.extend(v['rows'])
  else:rows.extend(numerics(b,wf,a.out,products,thermal_native,sources,p105))
  if f.exists():v=json.loads(f.read_text());assert a.resume and v['passed'];rows.append(v)
  else:rows.append(freshness(b,wf,a.out))
  dump(a.out/'progress.json',dict(passed=True,rows=rows))
 for cell in binding['profile_matrix']:
  if not cell['applicable']:rows.append(dict(control=cell['control'],profile=cell['profile'],applicable=False,status='not_applicable',reason=cell['expected']))
 assert len(rows)==48 and sum(r.get('passed',False) for r in rows)==39
 by={(r['control'],r['profile']):r for r in rows};assert len(by)==48
 for expected in binding['profile_matrix']:
  actual=by[expected['control'],expected['profile']];assert actual.get('passed',False) if expected['applicable'] else actual['status']=='not_applicable'
 b.freshness();dump(b.out/'objects.json',b.objects);dump(b.out/'dependencies.json',b.dependencies);result=dict(passed=True,checkpoint='P11.4',logical_controls=12,profile_cells=48,applicable_passed=39,explicit_not_applicable=9,rows=rows,dimension_rejections=6,numerical_detections=12,freshness_refusals=3,all_restorations_passed=True,performance_samples=0,p11_5_started=False,adoption=False,installation=False,release=False);dump(a.out/'result.json',result);print('P11.4 PASS:12 logical controls;39 applicable/9 explicit N/A; STOP BEFORE P11.5',flush=True)
if __name__=='__main__':main()
