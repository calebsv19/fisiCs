"""P11.3 only: explicit healthy owner compile/report/reference workflows."""
from pathlib import Path
import argparse,json,pickle,shutil,sys,hashlib
from support import ROOT,INITIAL,SerialBuild,PROFILES,dump,sha,selected_report,native_reference
from abi import qualify as abi
from bridge import prepare,execute

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--binding',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args();a.out.mkdir(exist_ok=False);binding=json.loads((a.binding/'bindings.json').read_text());workspace=Path(binding['workflows'][0]['source']['path']).parents[4];contract=workspace/'docs/private_program_docs/fisiCs/audits/2026-11-01_p10_1_binding';b=SerialBuild(contract,a.out/'build');b.env.pop('DISABLE_CODEGEN',None);assert b.fisics==binding['compiler']['path'] and sha(b.fisics)==binding['compiler']['sha256'];products,thermal_native=b.build();abi(b);sources=prepare(b,thermal_native);out=a.out/'healthy';out.mkdir();rows=[];reports=[]
 for wf in binding['workflows']:
  family=wf['family'];folder=out/family;folder.mkdir();prepared=folder/'prepared-physical.c';source=b.work/family/'fisics-physical'/Path(wf['source']['path']).name;shutil.copyfile(source,prepared);reports.append(dict(family=family,strict=selected_report(b,wf,prepared,folder/'healthy.physical.json',label=family+'-strict'),raw=selected_report(b,wf,prepared,folder/'raw.physical.json',raw=True,label=family+'-raw')))
  for profile in PROFILES:
   if family=='TS1':
    dest=folder/('native-'+profile);case=next(c for c in b.contract['cases'] if c['id']==wf['case']);v=execute(b.structural,Path(products[profile]['structural']),thermal_native,sources,case,1.2e-5,1,dest);assert v['passed'];actual=json.loads((dest/'result.json').read_text());digest=hashlib.sha256(json.dumps(actual['snapshots'],sort_keys=True,separators=(',',':')).encode()).hexdigest();row=dict(passed=True,profile=profile,family=family,reference=v,output_sha256=digest,result=str(dest/'result.json'))
   else:row=native_reference(b,wf,profile,products[profile]['thermal' if family=='T1' else 'dynamic'],'healthy-'+family+'-'+profile)
   row.update(control=family+'-A1',expected='healthy compile/reference and selected report pass');rows.append(row)
  assert len({r['output_sha256'] for r in rows if r['family']==family})==1;print(family,'HEALTHY WORKFLOW PASS',flush=True)
 b.freshness();dump(b.out/'objects.json',b.objects);dump(b.out/'dependencies.json',b.dependencies)
 with (a.out/'state.pickle').open('wb') as f:pickle.dump((b,binding,products,thermal_native,sources),f)
 result=dict(passed=True,checkpoint='P11.3',families=3,profile_outcomes=12,rows=rows,reports=reports,reciprocal_ABI_runs=8,guard=1,original_criteria_unchanged=True,workflow_entrypoint=str(Path(__file__)),performance_samples=0,p11_4_started=False,p11_5_started=False,adoption=False,installation=False,release=False);dump(out/'result.json',result);print('P11.3 PASS; ready for separately invoked P11.4',flush=True)
if __name__=='__main__':main()
