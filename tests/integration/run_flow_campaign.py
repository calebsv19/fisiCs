#!/usr/bin/env python3
"""Execute the finite authored physical-flow matrix; do not infer expectations from outcomes."""
from pathlib import Path
import argparse,json,hashlib,os,subprocess
ROOT=Path(__file__).resolve().parents[2]
def main():
 p=argparse.ArgumentParser(description=__doc__)
 for n in ['compiler','matrix','out','frame-probe','energy-probe']:p.add_argument('--'+n,type=Path,required=True)
 a=p.parse_args();a.out.mkdir(parents=True,exist_ok=False);out=a.out.resolve();compiler=a.compiler.resolve();matrix=json.loads(a.matrix.read_text());source_root=a.matrix.resolve().parent;env={**os.environ,'FISICS_MAX_PROCS':'0','PYTHONDONTWRITEBYTECODE':'1'};env.pop('FISICS_OVERLAY',None);env.pop('DISABLE_CODEGEN',None)
 result=dict(passed=False,matrix_sha256=hashlib.sha256(a.matrix.read_bytes()).hexdigest(),compiler_sha256=hashlib.sha256(compiler.read_bytes()).hexdigest(),scenarios=[],commands=[],failures=[])
 def save():(out/'results.json').write_text(json.dumps(result,indent=2)+'\n')
 def run(n,args,code):
  q=subprocess.run(list(map(str,args)),cwd=ROOT,env=env,capture_output=True,text=True,timeout=180);(out/(n+'.log')).write_text(q.stdout+q.stderr);result['commands'].append(dict(id=n,argv=list(map(str,args)),exit=q.returncode,expected=code));save();assert q.returncode==code,(n,q.returncode,code,(q.stdout+q.stderr)[-1800:]);return q.stdout
 suites={}
 for batch in ['B0','B1','B2','B3','B4','B5']:
  if batch in ['B4','B5']:
   name='frames' if batch=='B4' else 'coverage';script='run_physical_frame_flow_contracts.py' if name=='frames' else 'run_physical_coverage_contracts.py';dest=out/name;args=['python3','-B',ROOT/'tests/integration'/script,'--compiler',compiler,'--out',dest]
   if name=='frames':args+=['--probe',a.frame_probe.resolve()]
   run(name,args,0);suites[name]=json.loads((dest/('result.json' if name=='frames' else 'results.json')).read_text())
   if batch=='B4':
    dest=out/'energy';run('energy',['python3','-B',ROOT/'tests/integration/run_physical_energy_contracts.py','--compiler',compiler,'--probe',a.energy_probe.resolve(),'--flow-probe',a.frame_probe.resolve(),'--out',dest],0);suites['energy']=json.loads((dest/'result.json').read_text())
  for x in [x for x in matrix['scenarios'] if x['batch']==batch]:
   id=x['id'];row=dict(id=id,batch=batch,classification=x['classification'],passed=False)
   try:
    if x.get('suite'):
     d=suites[x['suite']]
     if x['suite'] in ['frames','energy']:
      c=next(c for c in d['cases'] if c['id']==x['case']);assert c['error']==x['diagnostic'];assert c['accepted']==(x['classification']=='positive');row['observed']=c
     else:
      cmds={c['id']:c for c in d['commands']};assert all(n in cmds for n in x['commands']);row['commands']=[cmds[n] for n in x['commands']]
      # Owning runner checks both orders, diagnostic/receipt contents and required trust.
      for n in x['commands']:
       receipt=out/'coverage'/(n+'.json')
       if receipt.exists():
        data=json.loads(receipt.read_text())
        if data.get('schema')=='fisiCs.physical_semantics':row.setdefault('selections',[]).append(data['selection'])
     row['assertion_authority']='owning stable contract runner; matrix selection frozen before run'
    else:
     source=source_root/x['source'];assert hashlib.sha256(source.read_bytes()).hexdigest()==x['source_sha256']
     flags=['-I'+str(ROOT/'include')];physical=x['mode']=='physical'
     if x['mode']=='units':flags+=['--overlay=physics-units']
     if physical:flags+=['--overlay=physics-semantics','--physical-semantics-policy',ROOT/'tests/fixtures/physical_semantics/policy.json']
     if x['classification']=='ordinary':
      for mode,opts in [('off',[]),('units',['--overlay=physics-units']),('physical',['--overlay=physics-semantics','--physical-semantics-policy',ROOT/'tests/fixtures/physical_semantics/policy.json'])]:
       exe=out/(id+'-'+mode);run(id+'-'+mode+'-compile',[compiler,*opts,'-I'+str(ROOT/'include'),source,'-o',exe],0);run(id+'-'+mode+'-run',[exe],0)
      exe=out/(id+'-clang');run(id+'-clang-compile',['clang','-std=c99','-Wall','-Wextra','-Werror','-pedantic','-I'+str(ROOT/'include'),source,'-o',exe],0);run(id+'-clang-run',[exe],0)
     else:
      diag=out/(id+'-diagnostics.json');code=1 if x['classification']=='negative' else 0
      run(id+'-diagnostics',[compiler,*flags,'--dump-sema','--emit-diags-json',diag,source],code)
      diagnostics=json.loads(diag.read_text())['diagnostics']
      if x['diagnostic']:assert any(d['code_id']==x['diagnostic'] and d['severity_name']=='error' for d in diagnostics),(id,'missing intended diagnostic',x['diagnostic'],[d['code_id'] for d in diagnostics])
      prefix='physical-semantics' if physical else 'units-coverage';receipt=out/(id+'.json');opts=['--'+prefix+'-function','check','--'+prefix+'-strict','--emit-'+prefix+'-json',receipt]
      expected=0 if x['classification'] in ['positive','trust'] else 1
      if x['classification']=='trust':
       run(id+'-trust-default-denied',[compiler,*flags,*opts,source],1);opts+=['--'+prefix+'-allow-'+x['permission']]
      if x['classification']=='unknown':opts+=['--'+prefix+'-allow-literal-assumptions','--'+prefix+'-allow-declared-boundaries']
      run(id+'-strict',[compiler,*flags,*opts,source],expected);data=json.loads(receipt.read_text());sel=data['selection' if physical else 'strict_policy'];assert sel['accepted']==(expected==0);assert sel['required']==sel['checked']+sel['permitted_trust']+sel['blocked'];row['selection']=sel
      if expected==0:assert sel['checked']>0 and sel['blocked']==0
      if x['classification']=='trust':assert sel['permitted_trust']>0
      if x['classification']=='positive':assert sel['permitted_trust']==0
    row['passed']=True
   except Exception as e:row['failure']=str(e);result['failures'].append(dict(id=id,classification=x['classification'],detail=str(e)))
   result['scenarios'].append(row);save()
  print(batch,'done; failures',len(result['failures']),flush=True)
 result['passed']=len(result['scenarios'])==168 and not result['failures'];save();assert result['passed'],result['failures'];print('Flow campaign PASS168')
if __name__=='__main__':main()
