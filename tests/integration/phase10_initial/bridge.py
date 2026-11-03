"""Bound fixture setup and existing connected-assembly owner invocation only."""
from pathlib import Path
import copy,json,sys,hashlib
from build import dump

def configure(structural):
 sys.path.insert(0,str(structural/'tools'))

def prepare(build,thermal_native):
 configure(build.structural)
 from ss9.thermal_source import ThermalValidator
 root=build.out.parent/'bridge-sources';root.mkdir();case=next(c for c in build.contract['cases'] if c['id']=='TS10-A');scenario=case['parameters']['owner_source_scenario']
 for name,scale in [('hot',1),('cool',2)]:
  value=copy.deepcopy(scenario);value['step_num']//=scale;value['steps']*=scale
  for row in value['power_schedule']:row['end_step']*=scale
  owner=ThermalValidator(thermal_native)
  try:
   owner.call('inspect',{});owner.call('replace',dict(scenario=value))
   for _ in range(scale):owner.call('step',dict(steps=60))
   owner.call('result_export',dict(path=str(root/(name+'.json'))))
  finally:owner.close()
  trace=json.loads((root/(name+'.json')).read_text())['payload']['trace'];assert trace['scenario']==value
 dump(root/'preparation.json',dict(producer=str(thermal_native),producer_sha256=hashlib.sha256(Path(thermal_native).read_bytes()).hexdigest(),hot_accepted_steps=60,cool_accepted_steps=120,comparison_runs=0,role='compatible immutable prescribed input preparation, not a separate comparison cell'))
 return root

def execute(structural,native,thermal_native,sources,case,alpha,load_factor,root):
 configure(structural)
 from ss4.contracts import encode,digest
 from ss4.dispatch import Workbench
 from ss9.assembly_contract import SCHEMA
 from bridge_reference import compare
 root.mkdir();p=case['parameters'];model=copy.deepcopy(p['spatial_template']);beam=model['members'][0];model['nodes']=[dict(id=k,xyz_m=v) for k,v in p['nodes'].items()];model['members']=[dict(beam,**x) for x in p['authored_members']];model['supports']=[dict(id='s'+str(i),**x) for i,x in enumerate(p['supports'])];model['loads']=p['mechanical_loads'];model['gravity_m_s2']=p['gravity_m_s2'];(root/'model.json').write_bytes(encode(model));traces={};source_rows=[]
 for name in p['sources']:
  raw=(sources/(name+'.json')).read_bytes();(root/(name+'.json')).write_bytes(raw);trace=json.loads(raw)['payload']['trace'];traces[name]=trace;source_rows.append(dict(id=name,result_path=name+'.json',result_sha256=digest(raw),engine=trace['engine'],scenario_digest_fnv1a64=trace['scenario_digest_fnv1a64'],assembly_id=trace['spatial']['assembly_id'],axis='along_member',cells=trace['spatial']['cells']))
 times=[dict(time_num=400,time_den=1)] if case['id']=='TS10-A' else [dict(time_num=t,time_den=1) for t in [0,200,400,600]]
 rules=[]
 for i,member in enumerate(p['authored_members']):rules.append(dict(member=member['id'],mode='mapped',source=p['member_sources'][i],direction=p['directions'][i],layers=[dict(layer_id='layer-1',material='mat',section='sec',stress_free_temperature_k=p['stress_free_temperature_k'],expansion_per_k=alpha)]))
 spec=dict(schema=SCHEMA,request_id='p102',model_path='model.json',model_sha256=digest(encode(model)),sources=source_rows,members=rules,times=times,load_factor=load_factor);(root/'request.json').write_bytes(encode(spec));dump(root/'authored.json',dict(model=model,spec=spec))
 app=Workbench(root,native,thermal_native);result=app.execute('run_thermal_history',dict(path='request.json'));dump(root/'result.json',result);folder=root/result['model_path'];answer=compare(result,model,spec,traces,folder.parent,case,structural)
 # Read-only continuation uses existing sealed identity; it does not solve again.
 from ss9.assembly_reader import read
 assert read(app.ws,'p102')==result
 return answer
