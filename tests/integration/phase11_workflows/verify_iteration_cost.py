"""Read-only replay of P11.5 iteration and all raw cost samples."""
from pathlib import Path
import argparse,json,hashlib,statistics,re
from support import dump,sha
from verify import raw_bytes

def timing(path):
 text=Path(path).read_text();return float(re.search(r'([\d.]+)\s+real',text)[1]),int(re.search(r'(\d+)\s+maximum resident set size',text)[1])
def main():
 p=argparse.ArgumentParser(description=__doc__)
 for name in ['iteration','cost','binding','out']:p.add_argument('--'+name,type=Path,required=True)
 a=p.parse_args();assert not a.out.exists();iteration=json.loads((a.iteration/'result.json').read_text());cost=json.loads((a.cost/'result.json').read_text());assert iteration['passed'] and cost['passed'] and len(iteration['rows'])==84;binding=json.loads((a.binding/'bindings.json').read_text());limits=binding['budgets']['overlay_policy']['values'];assert sha(binding['budgets']['overlay_policy']['path'])==cost['budget_sha256']==binding['budgets']['overlay_policy']['sha256']
 negative={'source-stale','include-stale','policy-stale','catalog-stale','old-generation-stale','regenerated-now-stale','corrupt-report','missing-report','failed-generation'}
 for row in iteration['rows']:
  assert row['passed']
  if row['step']=='occupied-output-refused':continue
  cmd=row['command'];assert cmd['exit']==cmd['expected_exit']==(1 if row['step'] in negative else 0)
  for stream in ['stdout','stderr']:assert hashlib.sha256(raw_bytes(a.iteration/'commands/logs'/(cmd['label']+'.'+stream))).hexdigest()==cmd[stream+'_sha256']
  if row['step'] in negative-{'failed-generation'}:assert b'physical cross-TU receipt/signature consistency rejected' in raw_bytes(a.iteration/'commands/logs'/(cmd['label']+'.stderr'))
 inputs=json.loads((a.iteration/'cost-inputs.json').read_text())
 for family in inputs['families']:
  for path,h in family['dependencies'].items():assert sha(path)==h,path
  assert sha(family['original_policy'])==family['original_policy_sha256']==sha(family['policy'])
  for path in family['reports']:
   d=json.loads(raw_bytes(path));assert d['analysis_valid'] and d['selection']['accepted'] and d['selection']['checked']==1 and d['selection']['permitted_trust']==0
 comparisons=0;reports=0;measurements=0
 for item in cost['compile_results']:
  path=Path(item['path']);v=json.loads(path.read_text());assert sha(path)==item['sha256'] and v['passed'] and v['budget']==limits and len(v['samples'])==80
  assert sha(v['samples'][0]['command'][0])==v['compiler_sha256']
  for source,h in v['source_sha256'].items():assert sha(source)==h,source
  for prepared in v['prepared_inputs']:
   for mode,h in prepared['sha256'].items():assert sha(path.parent/(prepared['id']+'-'+mode+'.c'))==h
  for row in v['samples']:
   assert timing(path.parent/(row['label']+'.time'))==(row['seconds'],row['peak_rss_bytes']);assert b'Error:' not in raw_bytes(path.parent/(row['label']+'.log'));measurements+=1
  for summary in v['summary']:
   assert summary['accepted']
   if summary['kind']=='compile':
    values={}
    for role,mode in [('base',summary['baseline']),('overlay',summary['overlay'])]:
     rows=[r for r in v['samples'] if r['label'].startswith(summary['id']+'-') and r['label'].endswith('-'+mode) and '-c' in r['command']];assert len(rows)==10 and sum(x['warmup'] for x in rows)==1;rows=[r for r in rows if not r['warmup']];wall=statistics.median(r['seconds'] for r in rows);rss=statistics.median(r['peak_rss_bytes'] for r in rows);assert wall==summary[role+'_median_seconds'] and rss==summary[role+'_median_rss_bytes'];values[role]=(wall,rss)
    assert values['overlay'][0]<=1.5*values['base'][0]+.05 and values['overlay'][1]<=1.5*values['base'][1]+8388608;comparisons+=1
   else:
    rows=[r for r in v['samples'] if r['label'].startswith(summary['id']+'-strict-')];assert len(rows)==10 and sum(x['warmup'] for x in rows)==1
    for r in rows:
     d=json.loads(raw_bytes(path.parent/(r['label']+'.json')));s=d['selection'];assert d['analysis_valid'] and s['accepted'] and s['enumeration_complete'] and s['checked']==1 and not s['blocked'] and not s['permitted_trust'];assert r['seconds']<=limits['strict_seconds'] and r['peak_rss_bytes']<=limits['strict_rss_bytes']
    measured=[r for r in rows if not r['warmup']];assert max(r['seconds'] for r in measured)==summary['max_seconds'] and max(r['peak_rss_bytes'] for r in measured)==summary['max_rss_bytes'];assert statistics.median(r['seconds'] for r in measured)==summary['median_seconds'] and statistics.median(r['peak_rss_bytes'] for r in measured)==summary['median_rss_bytes'];reports+=1
 checks=json.loads((a.cost/'checks.json').read_text());assert checks['passed'] and len(checks['samples'])==30
 for row in checks['samples']:
  assert row['exit']==0 and timing(row['timing'])==(row['seconds'],row['peak_rss_bytes']);folder=Path(row['timing']).parent;label=str(row['trial']+1)
  for stream in ['stdout','stderr']:assert hashlib.sha256(raw_bytes(folder/(label+'.'+stream))).hexdigest()==row[stream+'_sha256']
  assert row['seconds']<=limits['strict_seconds'] and row['peak_rss_bytes']<=limits['strict_rss_bytes']
 for summary in checks['summary']:
  rows=[r for r in checks['samples'] if r['family']==summary['family']];assert len(rows)==10 and sum(x['warmup'] for x in rows)==1;rows=[r for r in rows if not r['warmup']];assert statistics.median(r['seconds'] for r in rows)==summary['median_seconds'] and max(r['seconds'] for r in rows)==summary['max_seconds'];assert statistics.median(r['peak_rss_bytes'] for r in rows)==summary['median_rss_bytes'] and max(r['peak_rss_bytes'] for r in rows)==summary['max_rss_bytes'];assert summary['passed']
 assert comparisons==18 and reports==6 and measurements==240
 dump(a.out,dict(passed=True,iteration_checks=84,new_cost_executions=270,measured=243,warmups=27,compile_comparisons=18,strict_reports=6,receipt_check_workloads=3,all_raw_timings_logs_and_medians_replayed=True,budgets_unchanged=True,compiler_invocations=0,solver_invocations=0,adoption=False));print('P11.5 REPLAY PASS:84 checks,270 raw costs,18 matched comparisons')
if __name__=='__main__':main()
