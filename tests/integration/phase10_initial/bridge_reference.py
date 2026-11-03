"""Reconstruct expected topology/time/coefficients from authored inputs and raw traces.
No ss9 mapping, join or generated-case module is used here.
"""
import copy,hashlib,json,math,sys
from fractions import Fraction
import sys
def published_scalar(value):
 # Existing pinned cJSON.c print_number: 15 significant digits when the
 # parsed value differs by at most max(abs(a),abs(b))*DBL_EPSILON, else17.
 # Exact canonical sidecar inputs are checked separately before this display check.
 short=float(format(value,'.15g'))
 return short if abs(short-value)<=max(abs(short),abs(value))*sys.float_info.epsilon else float(format(value,'.17g'))
from references import Checks

def encode(v):return (json.dumps(v,sort_keys=True,separators=(',',':'),allow_nan=False)+'\n').encode()
def identity(kind,source,member,ordinal):return 'ts4_'+kind+'_'+hashlib.sha256(encode([source,member])).hexdigest()[:24]+'_'+str(ordinal)
def expected(authored,spec,traces,time):
 model=copy.deepcopy(authored);model['members']=[];rows=[];associations=[];nodes={n['id']:n['xyz_m'] for n in authored['nodes']};rules={r['member']:r for r in spec['members']};sources=[]
 for s in spec['sources']:
  trace=traces[s['id']];step=Fraction(trace['scenario']['step_num'],trace['scenario']['step_den']);sample=Fraction(time['time_num'],time['time_den'])/step;assert sample.denominator==1
  sources.append(dict(source_id=s['id'],sample=int(sample),**time))
 for member in authored['members']:
  rule=rules[member['id']];trace=traces[rule['source']];cells=trace['spatial']['cells'];span=cells[-1]['end_m'];layers={r['layer_id']:r for r in rule['layers']};a=nodes[member['node_a']];b=nodes[member['node_b']];indices=range(len(cells)) if rule['direction']=='forward' else range(len(cells)-1,-1,-1);previous=member['node_a'];sample=next(x['sample'] for x in sources if x['source_id']==rule['source'])
  for ordinal,i in enumerate(indices):
   cell=cells[i];layer=layers[cell['layer_id']];end=cell['end_m'] if rule['direction']=='forward' else span-cell['begin_m'];node=member['node_b'] if ordinal==len(cells)-1 else identity('n',rule['source'],member['id'],ordinal+1)
   if ordinal<len(cells)-1:model['nodes'].append(dict(id=node,xyz_m=[a[j]+(b[j]-a[j])*end/span for j in range(3)]))
   mid=identity('m',rule['source'],member['id'],ordinal);model['members'].append(dict(member,id=mid,node_a=previous,node_b=node,material=layer['material'],section=layer['section']))
   rows.append(dict(member=mid,temperature_k=float.fromhex(trace['samples'][sample][i]),stress_free_temperature_k=layer['stress_free_temperature_k'],expansion_per_k=layer['expansion_per_k']))
   associations.append(dict(member=mid,source_id=rule['source'],source_cell_id=cell['cell_id'],source_cell_index=i,source_sample=sample,cell_id=identity('c',rule['source'],member['id'],i)));previous=node
 return model,dict(schema='structural_sim_prescribed_member_temperature_v1',members=rows),sources,associations

def compare(result,authored,spec,traces,folder,case,structural):
 sys.path.insert(0,str(structural/'tests'));from thermal_frame_reference import solve
 c=Checks();p=case['parameters'];tols=case['thresholds']['by_quantity'];c.check(result['status']=='completed','native assembly completion');c.check(len(result['snapshots'])==len(spec['times']),'all canonical times');maxima={}
 def close(x,y,kind):
  bound=tols[kind]['absolute']+tols[kind]['relative']*max(abs(x),abs(y));c.close(x,y,bound,kind);maxima[kind]=max(maxima.get(kind,0),abs(x-y)/bound)
 mapping=json.loads((folder/'map.json').read_text());summaries=[]
 for index,time in enumerate(spec['times']):
  model,thermal,sources,associations=expected(authored,spec,traces,time);snapshot=result['snapshots'][index];entry=result['receipt']['cases'][index]
  c.check(json.loads((folder/'model.json').read_text())==model,'independent generated topology/materials')
  actual_thermal=json.loads((folder/entry['thermal_path']).read_text());c.check(actual_thermal==thermal,'independent alpha/Tref/raw-hex sample reconstruction')
  for obj in [snapshot,entry]:c.check(obj['case_index']==index and obj['time_num']==time['time_num'] and obj['time_den']==time['time_den'] and obj['sources']==sources,'independent exact clock/source associations')
  for expected_row in associations:
   row=next(x for x in mapping if x['member']==expected_row['member'])
   c.check(all(row[k]==expected_row[k] for k in ['source_id','source_cell_id','source_cell_index','cell_id']),'source cell identity/direction')
  for s in spec['sources']:
   source_path=folder/('source-'+s['id']+'.json');c.check(hashlib.sha256(source_path.read_bytes()).hexdigest()==s['result_sha256'],'source revision/raw observation bytes')
  ref=solve(model,thermal,spec['load_factor']);r=snapshot['result']
  for row in r['nodes']:
   e=ref['nodes'][row['id']]
   for i,j,sign,kind in [(0,0,1,'displacement'),(2,1,1,'displacement'),(4,2,-1,'rotation')]:close(row['displacement'][i],sign*e['u'][j],kind)
   for i,j,sign,kind in [(0,0,1,'force'),(2,1,1,'force'),(4,2,-1,'moment')]:close(row['reaction'][i],sign*e['reaction'][j],kind)
   for i in [1,3,5]:close(row['displacement'][i],0,'rotation' if i>=3 else 'displacement');close(row['reaction'][i],0,'moment' if i>=3 else 'force')
  sections={x['id']:x for x in model['sections']};materials={x['id']:x for x in model['materials']};nodes={x['id']:x['xyz_m'] for x in model['nodes']};temps={x['member']:x for x in thermal['members']};expansion=compliance=0
  for member in model['members']:
   L=math.dist(nodes[member['node_a']],nodes[member['node_b']]);t=temps[member['id']];sec=sections[member['section']];E=materials[member['material']]['youngs_modulus_pa'];expansion+=L*t['expansion_per_k']*(t['temperature_k']-t['stress_free_temperature_k']);compliance+=L/(E*sec['area_m2'])
  fixed=case['id']=='TS10-B';force=-expansion/compliance if fixed else 0
  for row in r['members']:
   q=ref['forces'][row['id']]
   for i,j,sign,kind in [(0,0,1,'force'),(2,1,1,'force'),(4,2,-1,'moment'),(6,3,1,'force'),(8,4,1,'force'),(10,5,-1,'moment')]:close(row['end_forces'][i],sign*q[j],kind)
   close(row['end_forces'][6],force,'force');t=temps[row['id']];c.check(row['temperature_k']==published_scalar(t['temperature_k']) and row['stress_free_temperature_k']==published_scalar(t['stress_free_temperature_k']) and row['expansion_per_k']==published_scalar(t['expansion_per_k']),'physical coefficient readback');close(row['thermal_strain'],t['expansion_per_k']*(t['temperature_k']-t['stress_free_temperature_k']),'displacement')
   member=next(x for x in model['members'] if x['id']==row['id']);sec=sections[member['section']];axial=q[3]/sec['area_m2'];bend=max(abs(q[2]),abs(q[5]))*sec['extreme_z_m']/sec['iy_m4'];close(row['maximum_tension_pa'],max(0,axial+bend),'stress');close(row['maximum_compression_pa'],max(0,-axial+bend),'stress')
  tip=next(x for x in r['nodes'] if x['id']=='c');close(tip['displacement'][0],0 if fixed else expansion,'displacement');close(r['strain_energy_j'],ref['energy'],'energy')
  for v in r['force_balance_n']:close(v,0,'force')
  for v in r['moment_balance_nm']:close(v,0,'moment')
  summaries.append(dict(time=time,sources=sources,free_expansion_m=expansion,restrained_force_n=force,tip_m=tip['displacement'][0],reference_energy_j=ref['energy']))
 return dict(passed=True,checks=c.n,maximum_errors=c.errors,maximum_tolerance_fraction=maxima,states=summaries,independent_input_reconstruction=True)
