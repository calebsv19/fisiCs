"""P11.5 costs: existing runner/budgets, only new scalar compile/report/check paths."""
from pathlib import Path
import argparse,json,shutil,subprocess,re,statistics,gzip,hashlib,difflib
from support import SerialBuild,dump,sha

def main():
 p=argparse.ArgumentParser(description=__doc__)
 for name in ['iteration','binding','out']:p.add_argument('--'+name,type=Path,required=True)
 a=p.parse_args();assert json.loads((a.iteration/'result.json').read_text())['passed'];inputs=json.loads((a.iteration/'cost-inputs.json').read_text());contract=json.loads((a.binding/'bindings.json').read_text());a.out.mkdir();workspace=Path(contract['workflows'][0]['source']['path']).parents[4];b=SerialBuild(workspace/'docs/private_program_docs/fisiCs/audits/2026-11-01_p10_1_binding',a.out/'commands');b.env.pop('DISABLE_CODEGEN',None)
 budget=contract['budgets']['overlay_policy'];assert sha(budget['path'])==budget['sha256'];limits=budget['values'];assert limits['samples']==9 and limits['parallelism']==1
 # Exact same two test-only adaptations already qualified by P10.6.
 maintained=Path(__file__).resolve().parents[2]/'performance/measure_overlay_cost.py';raw=maintained.read_text();adapted=raw.replace("'FISICS_MAX_PROCS':'0'","'FISICS_MAX_PROCS':'1'").replace("pp_command=['clang',","pp_command=[spec['preprocessor'],").replace("*args,item['source']]","*spec['preprocessor_args'],*args,item['source']]");runner=a.out/'measure_overlay_cost_bound.py';runner.write_text(adapted)
 prior=workspace/'data/fisics/p10-6-acceptance-20261102/prepared/runner-adaptation.json';old=json.loads(prior.read_text());assert sha(maintained)==old['original_sha256'] and sha(runner)==old['adapted_sha256'];dump(a.out/'runner-adaptation.json',dict(original=str(maintained),original_sha256=sha(maintained),adapted_sha256=sha(runner),exact_P10_6_runner=True,diff=''.join(difflib.unified_diff(raw.splitlines(True),adapted.splitlines(True)))))
 dump(a.out/'contract.json',dict(samples=9,warmups=1,compile_inputs=6,compile_modes=3,strict_reports=6,receipt_checks=3,expected_executions=270,expected_measured=243,expected_warmups=27,budget=budget,parallelism=1,failed_windows_retained=True,native_costs='unchanged sealed P10.6 evidence reused',owner_TU_costs='unchanged sealed P10.6 evidence reused',preprocessing_excluded_from_compile_times=True))
 compile_results=[];checks=[];check_summary=[]
 for family in inputs['families']:
  name=family['family'];root=Path(family['root']);sources=list(map(Path,family['sources']));args=['-I'+str(root/'include')];tu=[dict(id=name+'-'+s.stem,source=str(s),args=args) for s in sources];strict=[]
  for source,report in zip(sources,map(Path,family['reports'])):
   d=json.loads(report.read_text());strict.append(dict(id=name+'-'+source.stem,source=str(source),function=d['selection']['function'],args=args,physical=True,permissions=[],accepted=True,expected_checked=d['selection']['checked'],expected_permitted_trust=d['selection']['permitted_trust']))
  manifest=a.out/(name+'-manifest.json');dump(manifest,dict(physical_policy=family['policy'],preprocessor=b.clang,preprocessor_args=['-isysroot',b.tool['sdk_path']],dependencies=sorted(family['dependencies']),translation_units=tu,strict_functions=strict));dest=a.out/('compile-'+name);cmd=['/opt/homebrew/bin/python3','-B',str(runner),'--compiler',str(b.fisics),'--manifest',str(manifest),'--budget',budget['path'],'--out',str(dest)];q=subprocess.run(cmd,cwd=root,env=b.env,capture_output=True,timeout=1200)
  (a.out/(name+'.stdout')).write_bytes(q.stdout);(a.out/(name+'.stderr')).write_bytes(q.stderr);assert q.returncode==0,(name,q.stderr.decode()[-1500:]);v=json.loads((dest/'results.json').read_text());assert v['passed'] and len(v['samples'])==80;compile_results.append(dict(family=name,path=str(dest/'results.json'),sha256=sha(dest/'results.json'),executions=80,argv=cmd,passed=True));print(name,'SCALAR COMPILE/STRICT COST PASS',flush=True)
  folder=a.out/('check-'+name);folder.mkdir();values=[];before={p:sha(p) for p in family['reports']}
  for trial in range(-1,9):
   label=str(trial+1);timing=folder/(label+'.time');cmd=family['check_argv'];q=subprocess.run(['/usr/bin/time','-l','-o',str(timing),*cmd],cwd=root,env=b.env,capture_output=True,timeout=60)
   for stream,data in [('stdout',q.stdout),('stderr',q.stderr)]:(folder/(label+'.'+stream+'.gz')).write_bytes(gzip.compress(data,mtime=0))
   assert q.returncode==0,(name,trial,q.stderr.decode());raw=timing.read_text();wall=float(re.search(r'([\d.]+)\s+real',raw)[1]);rss=int(re.search(r'(\d+)\s+maximum resident set size',raw)[1]);row=dict(family=name,trial=trial,warmup=trial<0,argv=cmd,seconds=wall,peak_rss_bytes=rss,exit=0,stdout_sha256=hashlib.sha256(q.stdout).hexdigest(),stderr_sha256=hashlib.sha256(q.stderr).hexdigest(),timing=str(timing));checks.append(row)
   if trial>=0:values.append(row)
   dump(a.out/'check-progress.json',dict(samples=checks))
  assert before=={p:sha(p) for p in family['reports']};summary=dict(family=name,samples=9,median_seconds=statistics.median(x['seconds'] for x in values),max_seconds=max(x['seconds'] for x in values),median_rss_bytes=statistics.median(x['peak_rss_bytes'] for x in values),max_rss_bytes=max(x['peak_rss_bytes'] for x in values));summary['passed']=summary['max_seconds']<=limits['strict_seconds'] and summary['max_rss_bytes']<=limits['strict_rss_bytes'];check_summary.append(summary);dump(a.out/'checks.json',dict(passed=all(x['passed'] for x in check_summary),samples=checks,summary=check_summary));assert summary['passed'],summary
  for p,h in family['dependencies'].items():assert sha(p)==h,p
 b.freshness();dump(a.out/'result.json',dict(passed=True,checkpoint='P11.5-cost',executions=270,measured=243,warmups=27,compile_report_executions=240,receipt_check_executions=30,compile_results=compile_results,check_summary=check_summary,budget_sha256=budget['sha256'],native_costs_reused=True,owner_TU_costs_reused=True,parallelism=1,failed_windows_retained=True,adoption=False,installation=False,release=False));print('P11.5 COST PASS:270 executions, unchanged budgets',flush=True)
if __name__=='__main__':main()
