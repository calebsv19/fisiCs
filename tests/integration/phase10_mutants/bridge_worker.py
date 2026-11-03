"""One isolated Python adapter generation; independent oracle stays canonical."""
from pathlib import Path
import argparse,copy,hashlib,json,math,shutil,sys
from fractions import Fraction

def main():
 p=argparse.ArgumentParser()
 for name in ['structural','canonical-structural','initial','native','thermal-native','sources','binding','out']:p.add_argument('--'+name,type=Path,required=True)
 p.add_argument('--case',required=True);p.add_argument('--alpha',type=float,required=True);p.add_argument('--load-factor',type=float,required=True);p.add_argument('--mutant',required=True);a=p.parse_args()
 sys.path.insert(0,str(a.initial));sys.path.insert(0,str(a.structural/'tools'))
 from build import dump,sha
 from ss4.contracts import encode,digest
 from ss4.dispatch import Workbench
 from ss9.assembly_reader import read
 import bridge_reference
 from bridge import execute
 case=next(c for c in json.loads((a.binding/'bindings.json').read_text())['cases'] if c['id']==a.case);original_compare=bridge_reference.compare;observed=[]
 def observe(*args):
  try:r=original_compare(*args);v=dict(passed=True,reference=r)
  except AssertionError as e:v=dict(passed=False,failure=str(e))
  observed.append(v);return v
 bridge_reference.compare=observe
 oracle=execute(a.structural,a.native,a.thermal_native,a.sources,case,a.alpha,a.load_factor,a.out)
 assert len(observed)==1
 result=json.loads((a.out/'result.json').read_text());authored=json.loads((a.out/'authored.json').read_text());spec=authored['spec'];model=authored['model'];traces={name:json.loads((a.sources/(name+'.json')).read_text())['payload']['trace'] for name in case['parameters']['sources']};folder=(a.out/result['model_path']).parent
 # Independent closed-form mechanics consume original authored spec and raw
 # traces, never mutated generated thermal rows or generated output clocks.
 physical=[];clocks=[]
 for i,time in enumerate(spec['times']):
  refmodel,thermal,sources,associations=bridge_reference.expected(model,spec,traces,time);temps={r['member']:r for r in thermal['members']};nodes={r['id']:r['xyz_m'] for r in refmodel['nodes']};sections={r['id']:r for r in model['sections']};mats={r['id']:r for r in model['materials']};expansion=compliance=0.
  for m in refmodel['members']:
   L=math.dist(nodes[m['node_a']],nodes[m['node_b']]);t=temps[m['id']];expansion+=L*t['expansion_per_k']*(t['temperature_k']-t['stress_free_temperature_k']);compliance+=L/(mats[m['material']]['youngs_modulus_pa']*sections[m['section']]['area_m2'])
  snap=result['snapshots'][i];r=snap['result'];fixed=a.case=='TS10-B';expected_tip=0 if fixed else expansion;expected_force=-expansion/compliance if fixed else 0;actual_tip=next(n for n in r['nodes'] if n['id']=='c')['displacement'][0];actual_force=r['members'][0]['end_forces'][6]
  t=case['thresholds']['by_quantity'];bt=t['displacement']['absolute']+t['displacement']['relative']*max(abs(actual_tip),abs(expected_tip));bf=t['force']['absolute']+t['force']['relative']*max(abs(actual_force),abs(expected_force))
  physical.append(dict(canonical_time=time,actual_tip_m=actual_tip,reference_tip_m=expected_tip,tip_error_m=abs(actual_tip-expected_tip),tip_bound_m=bt,actual_axial_force_n=actual_force,reference_axial_force_n=expected_force,force_error_n=abs(actual_force-expected_force),force_bound_n=bf,passed=abs(actual_tip-expected_tip)<=bt and abs(actual_force-expected_force)<=bf))
  published=Fraction(snap['time_num'],snap['time_den']);expected_time=Fraction(time['time_num'],time['time_den']);joins=[]
  for s in sources:
   trace=traces[s['source_id']];step=Fraction(trace['scenario']['step_num'],trace['scenario']['step_den']);joins.append(dict(source=s['source_id'],sample=s['sample'],published_sample_num=(published/step).numerator,published_sample_den=(published/step).denominator,passed=published/step==s['sample']))
  clocks.append(dict(expected_num=expected_time.numerator,expected_den=expected_time.denominator,published_num=published.numerator,published_den=published.denominator,joins=joins,passed=published==expected_time and all(j['passed'] for j in joins)))
 # A coordinated reserialization changes bytes, then recomputes every artifact
 # hash, receipt and READY. The same mutated adapter must still read it normally.
 pub=a.out/'rehashed-publication';shutil.copytree(folder,pub/'thermal_runs/p102');base=pub/'thermal_runs/p102';receipt=json.loads((base/'receipt.json').read_text());changed=0
 for name in receipt['files']:
  f=base/name
  if f.suffix=='.json':
   raw=(json.dumps(json.loads(f.read_text()),indent=1,sort_keys=True,allow_nan=False)+'\n').encode()
   changed+=raw!=f.read_bytes();f.write_bytes(raw)
  receipt['files'][name]=sha(f)
 (base/'receipt.json').write_bytes(encode(receipt));(base/'READY').write_text(digest(encode(receipt))+'\n')
 # Canonical regeneration requires canonical byte encoding for derived files;
 # reserialize only snapshot result records, retaining canonical derived inputs.
 for name in receipt['files']:
  if name.startswith('case-') and not name.endswith('-thermal.json'):continue
  f=base/name;f.write_bytes((folder/name).read_bytes());receipt['files'][name]=sha(f)
 (base/'receipt.json').write_bytes(encode(receipt));(base/'READY').write_text(digest(encode(receipt))+'\n')
 changed=sum((base/name).read_bytes()!=(folder/name).read_bytes() for name in receipt['files']);assert changed==len(spec['times'])
 app=Workbench(pub,a.native,a.thermal_native);rehash=read(app.ws,'p102');expected=copy.deepcopy(result);expected['receipt']=receipt;assert rehash==expected
 sensitive=a.mutant not in ('control',) and not (a.mutant=='M08' and a.load_factor==1)
 if sensitive:
  assert not oracle['passed'],('SURVIVOR',a.mutant)
  if a.mutant in ('M08','M09'):
   assert 'independent alpha/Tref/raw-hex sample reconstruction' in oracle['failure'];assert any(not x['passed'] for x in physical)
  else:
   assert 'independent exact clock/source associations' in oracle['failure'];assert all(x['passed'] for x in physical) and all(not x['passed'] for x in clocks)
 else:assert oracle['passed'] and all(x['passed'] for x in physical+clocks)
 answer=dict(passed=True,detected=sensitive,sensitivity_control=a.mutant=='M08' and a.load_factor==1,oracle=oracle,physical=physical,clocks=clocks,mutated_owner_readback_passed=True,coordinated_rehash_readback_passed=True,changed_snapshot_bytes=changed,canonical_authored_inputs_unchanged=True,publication=str(pub),publication_files={str(f.relative_to(pub)):sha(f) for f in pub.rglob('*') if f.is_file()},benchmark_runs=0,p10_6_started=False)
 dump(a.out/'detector.json',answer);print(json.dumps(answer,allow_nan=False))
if __name__=='__main__':main()
