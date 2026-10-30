#!/usr/bin/env python3
"""Finite immutable local storage provenance; no external solver dependency."""
from pathlib import Path
import argparse,hashlib,json,os,subprocess
ROOT=Path(__file__).resolve().parents[2]
def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--compiler',type=Path,required=True);p.add_argument('--out',type=Path,required=True)
 a=p.parse_args();compiler=a.compiler.resolve();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False)
 fixture=ROOT/'tests/fixtures/physical_semantics/storage_snapshots';matrix=json.loads((fixture/'matrix.json').read_text());rows=[];commands=[]
 env={**os.environ,'FISICS_MAX_PROCS':'0','PYTHONDONTWRITEBYTECODE':'1'};env.pop('FISICS_OVERLAY',None)
 def run(name,args,code):
  cmd=list(map(str,args));q=subprocess.run(cmd,cwd=ROOT,env=env,capture_output=True,text=True,timeout=60)
  (out/(name+'.log')).write_text(q.stdout+q.stderr);commands.append(dict(id=name,argv=cmd,exit=q.returncode,expected=code))
  assert q.returncode==code,(name,q.returncode,(q.stdout+q.stderr)[-1800:])
 for x in matrix['cases']:
  source=fixture/x['file'];assert hashlib.sha256(source.read_bytes()).hexdigest()==x['sha256']
  report=out/(x['id']+'.json');diag=out/(x['id']+'-diagnostics.json')
  args=[compiler,'-I'+str(ROOT/'include'),'--overlay=physics-semantics','--physical-semantics-policy',fixture/'policy.json','--physical-semantics-strict','--physical-semantics-function','check','--emit-physical-semantics-json',report,source]
  if x['classification']=='trust':run(x['id']+'-permission-denied',args,1)
  code=0 if x['classification'] in ['positive','trust'] else 1
  run(x['id'],[*args,'--physical-semantics-allow-declared-boundaries','--physical-semantics-allow-literal-assumptions'],code)
  d=json.loads(report.read_text());sel=d['selection'];assert sel['accepted']==(code==0)
  assert sel['required']==sel['checked']+sel['permitted_trust']+sel['blocked']
  assert code or (sel['checked']>0 and sel['blocked']==0)
  if x['classification']=='positive':assert sel['permitted_trust']==0
  if x['classification']=='trust':assert sel['permitted_trust']>0
  if x['classification']=='negative':
   run(x['id']+'-diagnostics',[compiler,'-I'+str(ROOT/'include'),'--overlay=physics-semantics','--physical-semantics-policy',fixture/'policy.json','--dump-sema','--emit-diags-json',diag,source],1)
   assert any(z['code_id'] in [4316,4317,4318] for z in json.loads(diag.read_text())['diagnostics'])
  rows.append(dict(id=x['id'],classification=x['classification'],selection=sel))
 # A portable runtime storage example must preserve normal C and ABI behavior.
 src=out/'ordinary.c';src.write_text('double pick(double x,double y){const double m[2][2]={{x,y},{y,x}};return m[1][1];} int main(void){return pick(3,4)!=3;}\n')
 for mode,flags in [('off',[]),('units',['--overlay=physics-units']),('physical',['--overlay=physics-semantics','--physical-semantics-policy',fixture/'policy.json'])]:
  exe=out/('ordinary-'+mode);run(mode+'-compile',[compiler,*flags,src,'-o',exe],0);run(mode+'-run',[exe],0)
 exe=out/'ordinary-clang';run('clang-compile',['clang','-std=c99','-Wall','-Wextra','-Werror','-pedantic',src,'-o',exe],0);run('clang-run',[exe],0)
 result=dict(passed=True,cases=rows,commands=commands,compiler_sha256=hashlib.sha256(compiler.read_bytes()).hexdigest(),matrix_sha256=hashlib.sha256((fixture/'matrix.json').read_bytes()).hexdigest())
 (out/'results.json').write_text(json.dumps(result,indent=2)+'\n');print('Storage snapshots PASS',len(rows),'cases')
if __name__=='__main__':main()
