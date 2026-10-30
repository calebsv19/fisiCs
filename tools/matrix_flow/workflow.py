#!/usr/bin/env python3
"""Explicit contained build, reviewed input preparation, report and optional tests."""
import argparse,json,subprocess,sys
from pathlib import Path
from preflight import analysis_environment

def main():
 p=argparse.ArgumentParser(description=__doc__)
 for n in ('compiler-root','core-root','out'):p.add_argument('--'+n,type=Path,required=True)
 p.add_argument('--check',action='store_true');a=p.parse_args();out=a.out.resolve();here=Path(__file__).parent.resolve()
 try:out.mkdir(parents=True,exist_ok=False)
 except OSError as e:print('output: '+str(e));return 2
 try:env,configuration=analysis_environment()
 except (OSError,ValueError,subprocess.SubprocessError) as error:
  result=dict(passed=False,status='preflight_failed',diagnostic=str(error));(out/'result.json').write_text(json.dumps(result,indent=2)+'\n');print(str(error),file=sys.stderr);return 1
 (out/'analysis-configuration.json').write_text(json.dumps(configuration,indent=2)+'\n')
 print('Analysis configuration: '+configuration['clang']+'; resource '+configuration['resource'],flush=True)
 rows=[]
 def run(name,args):
  cmd=[sys.executable,'-B',str(here/name),*map(str,args)]
  with (out/(name+'.log')).open('w') as log:q=subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,env=env)
  rows.append(dict(argv=cmd,exit=q.returncode));(out/'result.json').write_text(json.dumps(dict(passed=False,status='running' if q.returncode==0 else 'failed',commands=rows,analysis_configuration=configuration),indent=2)+'\n')
  return q.returncode==0
 build=out/'frontend';adapter=out/'adapter';inputs=out/'inputs'
 if not run('prepare_inputs.py',['--compiler-root',a.compiler_root,'--core-root',a.core_root,'--out',inputs]):return 1
 if not run('build_frontend.py',['--compiler-root',a.compiler_root,'--out',build,*(['--with-compiler'] if a.check else [])]):return 1
 if not run('build_adapter.py',['--compiler-root',a.compiler_root,'--frontend-archive',build/'libfrontend.a','--frontend-receipt',build/'result.json','--out',adapter]):return 1
 args=['--compiler-root',a.compiler_root,'--core-root',inputs/'core/core_structural','--adapter',adapter/'ast-export','--adapter-receipt',adapter/'result.json']
 if not run('cli.py',[*args,'--readset',inputs/'readset.json','--admit-planar-si-exclusive-owner','--out',out/'report']):return 1
 if a.check:
  if not run('check_arithmetic.py',[]):return 1
  if not run('check_soundness.py',[*args,'--out',out/'soundness']):return 1
  if not run('check_operations.py',[*args,'--frontend-build',build,'--readset',inputs/'readset.json','--compiler',build/'fisics','--out',out/'operations']):return 1
 (out/'result.json').write_text(json.dumps(dict(passed=True,status='complete',commands=rows,analysis_configuration=configuration),indent=2)+'\n')
 return 0
if __name__=='__main__':raise SystemExit(main())
