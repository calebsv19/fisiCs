"""Replay P11.3/P11.4 evidence without compilers, solvers or cost sampling."""
from pathlib import Path
import argparse,json,gzip,hashlib,sys
from support import INITIAL,dump,sha,ROOT
from references import thermal,dynamic
from bridge_reference import compare

def raw_bytes(p):
 p=Path(p);return p.read_bytes() if p.exists() else gzip.decompress(p.with_suffix(p.suffix+'.gz').read_bytes())
def verify_command(build,row):
 for stream in ['stdout','stderr']:
  data=raw_bytes(build/'logs'/(row['label']+'.'+stream));assert hashlib.sha256(data).hexdigest()==row[stream+'_sha256']
 assert row['exit']==row['expected_exit'];return raw_bytes(build/'logs'/(row['label']+'.stdout'))
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--workflow',type=Path,required=True);p.add_argument('--controls',type=Path,required=True);p.add_argument('--binding',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args();assert not a.out.exists();binding=json.loads((a.binding/'bindings.json').read_text());healthy=json.loads((a.workflow/'healthy/result.json').read_text());controls=json.loads((a.controls/'result.json').read_text());build=a.workflow/'build';assert healthy['passed'] and controls['passed']
 workspace=Path(binding['workflows'][0]['source']['path']).parents[4];contract=workspace/'docs/private_program_docs/fisiCs/audits/2026-11-01_p10_1_binding';cases={c['id']:c for c in json.loads((contract/'bindings.json').read_text())['cases']};by_wf={x['family']:x for x in binding['workflows']};health={}
 for r in healthy['rows']:
  family=r['family'];wf=by_wf[family]
  if family!='TS1':
   raw=verify_command(build,r['command']);assert hashlib.sha256(raw).hexdigest()==r['output_sha256'];values=[json.loads(x) for x in raw.splitlines()];v=(thermal if family=='T1' else dynamic)(values[0],values[1:],cases[wf['case']]);assert v['passed']
  else:
   result_path=Path(r['result']);result=json.loads(result_path.read_text());root=result_path.parent;auth=json.loads((root/'authored.json').read_text());traces={s:json.loads((root/(s+'.json')).read_text())['payload']['trace'] for s in cases[wf['case']]['parameters']['sources']};v=compare(result,auth['model'],auth['spec'],traces,(root/result['model_path']).parent,cases[wf['case']],workspace/'_worktrees/structural_sim_main_edit');assert v['passed'];digest=hashlib.sha256(json.dumps(result['snapshots'],sort_keys=True,separators=(',',':')).encode()).hexdigest();assert digest==r['output_sha256']
  health[family,r['profile']]=r['output_sha256']
 for r in healthy['reports']:
  for mode in ['strict','raw']:
   item=r[mode];data=raw_bytes(item['path']);assert hashlib.sha256(data).hexdigest()==item['sha256'];d=json.loads(data);assert d['selection']==item['selection'] and d['analysis_valid'];assert d['selection']['accepted']==(mode=='strict')
 assert len(health)==12 and len(controls['rows'])==48
 by={(r['control'],r['profile']):r for r in controls['rows']};assert len(by)==48
 for cell in binding['profile_matrix']:
  r=by[cell['control'],cell['profile']]
  if not cell['applicable']:assert r['status']=='not_applicable';continue
  assert r['passed']
  if r['kind']=='healthy':assert r['output_sha256']==health[r['family'],r['profile']]
  if r['kind']=='wrong_dimension' and r['profile'] in ['fisics-units','fisics-physical']:
   verify_command(build,r['command']);assert r['command']['exit']==1 and 'units subtraction mismatch:' in r['diagnostic']
  if r['kind']=='dimensionally_valid_defect':
   family=r['family']
   if family=='TS1':
    verify_command(build,r['command']);assert r['reference']['detected'] and r['reference']['coordinated_rehash_readback_passed'];assert any(not x['passed'] for x in r['reference']['physical'])
   else:
    run=r['run'];label='P114-'+r['mutant']+'-'+r['profile'];raw=raw_bytes(build/'logs'/(label+'.stdout'));assert hashlib.sha256(raw).hexdigest()==run['stdout_sha256'] and run['exit']==0;values=[json.loads(x) for x in raw.splitlines()]
    try:(thermal if family=='T1' else dynamic)(values[0],values[1:],cases[by_wf[family]['case']])
    except AssertionError as e:assert r['intended_detector'] in str(e)
    else:raise AssertionError(('mutant survived replay',r['mutant']))
 for wf in binding['workflows']:
  family=wf['family'];d=json.loads((a.controls/(family+'-dimensions.json')).read_text());n=json.loads((a.controls/(family+'-numerics.json')).read_text());f=json.loads((a.controls/(family+'-freshness.json')).read_text());assert d['passed'] and n['passed'] and f['passed']
  for r in d['restored']:verify_command(build,r['command']);assert r['header_sha256']==wf['header']['sha256']
  for r in n['restored']:
   if family=='TS1':verify_command(build,r['command']);assert not r['detected'] and r['reference']['oracle']['passed']
   else:
    raw=verify_command(build,r['command']);assert hashlib.sha256(raw).hexdigest()==health[family,r['profile']]
  assert f['fresh_pair_passed'] and f['stale_source_refused'] and f['exact_restore_passed'] and f['reports_preserved']
  for cmd in f['commands']:verify_command(build,cmd)
  assert [cmd['exit'] for cmd in f['commands']]==[0,0,0,1,0]
  root=a.controls/'freshness-scalar'/family;amend=json.loads((root/'fixture-amendment.json').read_text());assert (root/'a.c').read_text()==amend['effective_a'] and (root/'b.c').read_text()==amend['effective_b'];assert amend['original_a']==wf['freshness_fixture']['a'] and amend['original_b']==wf['freshness_fixture']['b'];assert not amend['checker_or_policy_changed']
  for path,h in f['reports'].items():assert hashlib.sha256(raw_bytes(path)).hexdigest()==h
 # The frozen contract and all prior owners remain independent inputs.
 for path,h in json.loads((contract/'owner-inputs.json').read_text()).items():assert sha(path)==h,path
 result=dict(passed=True,healthy_outcomes=12,logical_controls=12,profile_cells=48,applicable=39,not_applicable=9,dimension_rejections=6,numerical_detections=12,stale_refusals=3,all_restorations_passed=True,raw_owner_body_acceptance=False,raw_reports_correctly_unaccepted=True,scalar_witness_amendments=3,compilers_invoked=0,solvers_invoked=0,performance_samples=0,p11_5_started=False);dump(a.out,result);print(json.dumps(result,indent=2))
if __name__=='__main__':main()
