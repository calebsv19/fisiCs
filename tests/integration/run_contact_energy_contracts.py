#!/usr/bin/env python3
"""Qualify contained axial contact scalar candidates; never edits the donor/consumer."""
from pathlib import Path
import argparse,sys,subprocess,re,json,hashlib,os
ROOT=Path(__file__).resolve().parents[2]
def main():
 p=argparse.ArgumentParser(description=__doc__)
 for n in ['compiler','particle-root','donor-root','backend-build','out']:p.add_argument('--'+n,type=Path,required=True)
 a=p.parse_args();a.out=a.out.resolve();a.out.mkdir(parents=True,exist_ok=False)
 fixture=ROOT/'tests/fixtures/solver_physics/contact_energy';sys.path.insert(0,str(fixture));from prepare import prepare,SELECTED,DONOR
 from source_gate import gate,FUN
 # Importer validates exact frozen profile; live Ball source is never a build input.
 sys.path.insert(0,str(a.particle_root/'tools'));from contact_donor import validate_export
 validate_export(a.donor_root);pins=prepare(a.particle_root,a.donor_root,a.out/'candidate')
 inc=['-I'+str(ROOT/'include'),'-I'+str(a.particle_root/'include'),'-I'+str(a.particle_root/'src/contact'),'-I'+str(a.donor_root/'src')]
 identity=json.loads((a.backend_build/'build_identity.json').read_text());assert identity['source_root']==str(a.particle_root.resolve())
 for n,h in identity['engine_inputs'].items():assert hashlib.sha256((a.particle_root/n).read_bytes()).hexdigest()==h
 inc += [v for v in identity['flags'] if v.startswith('-I')]
 actual={};mapping={'ce_kinetic':'coulomb_step.c','ce_momentum_delta':'three_response.c','ce_impulse':Path(DONOR).name,'ce_loss':'three_response.c','ce_balance':'three_step.c'}
 prepared={}
 for name in sorted(set(mapping.values())):
  q=subprocess.run(['clang','-E','-P','-fno-blocks','-D__FISICS__=1','-DCONTACT_PHYSICAL=1',*inc,str(a.out/'candidate'/name)],capture_output=True,text=True,check=True)
  dest=a.out/('physical-'+name);dest.write_text(q.stdout);prepared[name]=dest
 for fn in FUN:actual[fn]=prepared[mapping[fn]]
 result=gate(a.compiler.resolve(),ROOT/'include',fixture,a.out/'candidate',a.out/'contracts',actual)
 raw=[]
 for name,fn in [('coulomb_step.c','ps_coulomb_energy'),('three_response.c','ps_three_impulses'),('three_step.c','ps_contact_three_step'),(Path(DONOR).name,'iterate_with_impulse_cutoff')]:
  report=a.out/('raw-'+fn+'.json');q=subprocess.run([str(a.compiler),'--overlay=physics-semantics','--physical-semantics-policy',str(fixture/'policy.json'),'--physical-semantics-strict','--physical-semantics-function',fn,'--emit-physical-semantics-json',str(report),str(prepared[name])],capture_output=True,text=True);(a.out/('raw-'+fn+'.log')).write_text(q.stdout+q.stderr);assert q.returncode!=0 and report.exists();s=json.loads(report.read_text())['selection'];assert not s['accepted'] and s['blocked']>0;raw.append(s)
 result['raw_refusals']=raw;result['transforms']=pins;result['prepared_sha256']={n:hashlib.sha256(v.read_bytes()).hexdigest() for n,v in prepared.items()};result['compiler_sha256']=hashlib.sha256(a.compiler.read_bytes()).hexdigest()
 for n,v in pins.items():assert hashlib.sha256(Path(n).read_bytes()).hexdigest()==v['sha256']
 validate_export(a.donor_root);(a.out/'results.json').write_text(json.dumps(result,indent=2)+'\n');print('Contact energy source contracts PASS')
if __name__=='__main__':main()
