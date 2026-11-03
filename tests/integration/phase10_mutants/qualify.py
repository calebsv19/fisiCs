"""P10.5 only: exact ten mutants in disposable copies; no cost/adoption switch."""
from pathlib import Path
import argparse,copy,gzip,hashlib,json,math,pickle,shutil,subprocess,sys,traceback
from decimal import Decimal as D,localcontext
ROOT=Path(__file__).resolve().parent
INITIAL=ROOT.parent/'phase10_initial'
sys.path.insert(0,str(INITIAL))
from build import Build,PROFILES,dump,sha
from references import thermal,dynamic

CHANGES={
 'M01':('return dt * g * (left - right);','return -dt * g * (left - right);'),
 'M02':('return cv * a * dx;','return 2 * cv * a * dx;'),
 'M03':('return e - v * h + dt *','return dt *'),
 'M04':('cs_physical_translation_work(half*(old+now),right-left)','cs_physical_translation_work(half*(old+now),left-right)'),
 'M05':('return current-initial+kinetic+damping-external;','return current+kinetic+damping-external;'),
 'M06':('return dt*alpha*quarter*(oi+vi)*mass*(oj+vj);','return 2*dt*alpha*quarter*(oi+vi)*mass*(oj+vj);'),
 'M07':('o->accepted.time_s=o->accepted.tick*dt','o->accepted.time_s=(o->accepted.tick+1)*dt'),
 'M08':("expansion_per_k=row['expansion_per_k']","expansion_per_k=row['expansion_per_k']*spec['load_factor']"),
 'M09':("stress_free_temperature_k=row['stress_free_temperature_k']","stress_free_temperature_k=0.0"),
 'M10':('output.append(dict(**time, thermal=',"output.append(dict(**dict(time, time_num=time['time_num']+1), thermal=")}

def replace(path,old,new):
 raw=path.read_text();assert raw.count(old)==1,(path,old,raw.count(old));path.write_text(raw.replace(old,new))

def native_copy(b,id):
 family='T1' if id in ('M01','M02','M03','control-observed') else 'D1'
 target=b.work/'copies'/id;target.mkdir(parents=True)
 if family=='T1':
  origin=b.core/'core_thermal';shutil.copytree(origin/'include',target/'include');shutil.copytree(origin/'src',target/'src');extra=[target/'include']
 else:
  origin=b.core/'core_structural';(target/'src').mkdir()
  for p in (origin/'src').glob('*.h'):shutil.copyfile(p,target/'src'/p.name)
  p=Path(b.contract['selected_translation_units']['D1'][0]);shutil.copyfile(p,target/'src'/p.name);extra=[target/'src']
 if id!='control-observed':
  mutation=next(m for m in b.contract['mutants'] if m['id']==id);source=Path(mutation['site']['path']);assert sha(source)==mutation['site']['sha256'];dest=target/source.relative_to(origin)
  if id=='M06':
   lines=[line for line in source.read_text().splitlines() if line.startswith('static inline double DE_E cs_de_damping_tt(')];assert len(lines)==1;before=lines[0];assert before.count(CHANGES[id][0])==1;replace(dest,before,before.replace(*CHANGES[id]))
  else:replace(dest,*CHANGES[id])
  dump(b.out/(id+'-mutation.json'),dict(binding=mutation,copy=str(dest),before=CHANGES[id][0],after=CHANGES[id][1],mutated_sha256=sha(dest)))
 if id in ('M01','control-observed'):
  # Observe the complete candidate before the unchanged native balance guard.
  p=target/'src/core_thermal_rod.c';text=p.read_text();text='#include <stdio.h>\n'+text
  anchor='    double scale=1;';assert text.count(anchor)==1
  observer='    if(m->count==2) fprintf(stderr,"P105_CANDIDATE {\\"e0\\":%.17g,\\"e1\\":%.17g,\\"old0\\":%.17g,\\"old1\\":%.17g,\\"t0\\":%.17g,\\"t1\\":%.17g,\\"q\\":%.17g,\\"dt\\":%.17g}\\n", result.state.energy_j[0],result.state.energy_j[1],s->energy_j[0],s->energy_j[1],result.temperature_k[0],result.temperature_k[1],result.face_transfer_j[1],dt);\n'
  p.write_text(text.replace(anchor,observer+anchor));dump(b.out/(id+'-observation.json'),dict(path=str(p),anchor=anchor,read_only_observer=observer,sha256=sha(p),guards_unchanged=True))
 oldinc=b.inc;b.inc=['-I'+str(p) for p in extra]+oldinc;products={}
 try:
  for profile in PROFILES:
   parts=[];d=b.work/'mutants'/id/profile;d.mkdir(parents=True)
   for original in b.contract['selected_translation_units'][family]:
    source=target/'src'/Path(original).name;obj=d/(source.stem+'.o')
    if profile=='matched-clang':b.compile_native(source,obj)
    else:
     prepared=b.prepared(source,family,profile,d/(source.stem+'.c'));flags=[]
     if profile=='fisics-units':flags=['--overlay=physics-units']
     if profile=='fisics-physical':flags=['--overlay=physics-semantics','--physical-semantics-policy',b.contract['policies'][family]]
     b.run('mut-cc-'+id+'-'+profile+'-'+source.stem,[b.fisics,*flags,'-c',prepared,'-o',obj])
    assert b'Mach-O' in b.run('mut-arch-'+str(len(b.commands)),['/usr/bin/file',obj]);parts.append(obj)
   if family=='T1':objects=[b.work/'thermal-driver.o',*parts]
   else:
    common=[];selected=set(b.contract['selected_translation_units']['D1'])
    for obj in sorted((b.work/'native-objects').glob('*.o')):
     source=b.objects[str(obj)]['source']
     if source not in selected and source!=str(b.structural/'src/main.c'):common.append(obj)
    objects=[b.work/'dynamic-driver.o',*common,*parts]
   products[profile]=b.link('mutants/'+id+'/'+profile+'/probe',objects)
 finally:b.inc=oldinc
 return products

def execute(b,label,exe,kind,variant,family):
 cmd=[str(exe),str(kind),str(variant)];timeout=300 if family=='thermal' else 180
 try:q=subprocess.run(cmd,env=b.env,capture_output=True,timeout=timeout)
 except subprocess.TimeoutExpired:raise AssertionError(('timeout is not detection',label))
 assert q.returncode in (0,2),('crash/not detector',label,q.returncode)
 assert len(q.stdout)<=(16 if family=='thermal' else 32)*1024**2
 for name,raw in [('stdout',q.stdout),('stderr',q.stderr)]:
  (b.logs/(label+'.'+name+'.gz')).write_bytes(gzip.compress(raw,mtime=0))
 values=[json.loads(line) for line in q.stdout.splitlines()];assert values
 return q.returncode,values[0],values[1:],q.stderr.decode(),dict(argv=cmd,exit=q.returncode,watchdog_s=timeout,stdout_sha256=hashlib.sha256(q.stdout).hexdigest(),stdout_bytes=len(q.stdout))

def reference(family,header,rows,case):
 try:return dict(passed=True,result=(thermal if family=='thermal' else dynamic)(header,rows,case))
 except AssertionError as e:return dict(passed=False,failure=str(e))

def faint_prefix(header,rows,case):
 # Refusals are never the oracle. Check only actual published accepted states,
 # retaining the original declared horizon and the unchanged 1e-9 cell bound.
 assert rows and len(rows)<=header['ticks']
 from references import dec
 with localcontext() as ctx:
  ctx.prec=80;v=header['variant'];h=dec(header['dt']);C=[D(50),D(100 if v==7 else 50)];g=D(8)/3 if v==7 else D(2);amp=dec(case['parameters']['amplitudes'][v]);e=[amp if v==6 else D(0),D(0)];first=None;maximum=D(0)
  for tick,row in enumerate(rows,1):
   assert row['tick']==tick and row['time']==tick*header['dt']
   powers=[D(0) if v in (4,6) else amp,amp if v==4 else D(0)]
   if v==5:powers[0]=amp if tick-1<round(1.28/header['dt']) else D(0) if tick-1<round(2.56/header['dt']) else -amp/4
   a=h*g/C[0];b=h*g/C[1];x=e[0]+h*powers[0];y=e[1]+h*powers[1];e=[((1+b)*x+b*y)/(1+a+b),(a*x+(1+a)*y)/(1+a+b)]
   for i in range(2):
    assert e[i]>0
    error=abs(dec(row['e'][i])/e[i]-1);maximum=max(maximum,error)
    if error>D('1e-9') and first is None:first=dict(tick=tick,cell=i,actual_j=row['e'][i],reference_j=str(e[i]),relative_error=str(error),relative_bound='1e-9',absolute_floor_j=0)
  return dict(passed=first is None,failure='cell_relative' if first else None,first_witness=first,maximum_relative_error=str(maximum),accepted_ticks_checked=len(rows),declared_ticks=header['ticks'])

def native(b,products,out):
 out.mkdir(exist_ok=True);cases={c['id']:c for c in b.contract['cases']};control={};allrows=json.loads((out/'results.json').read_text())['rows'] if (out/'results.json').exists() else [];labels={r['label']:r for r in allrows}
 for r in allrows:
  raw=gzip.decompress((b.logs/(r['label']+'.stdout.gz')).read_bytes());assert hashlib.sha256(raw).hexdigest()==r['run']['stdout_sha256']
  if r['label'].startswith('control-') and not r['label'].startswith('control-observed-'):control[r['profile'],r['case'],r['variant']]=r['run']['stdout_sha256']
 for profile in PROFILES:
  for family,prefix in [('thermal','T'),('dynamic','D')]:
   for kind in range(3):
    id=prefix+'10-'+chr(65+kind)
    for variant in range(8 if id=='T10-C' else 2):
     label='control-'+profile+'-'+id+'-'+str(variant)
     if label in labels:continue
     rc,h,rows,err,run=execute(b,label,products[profile][family],kind,variant,family);assert rc==0,(label,err)
     check=reference(family,h,rows,cases[id]);assert check['passed'],(label,check);control[profile,id,variant]=run['stdout_sha256'];allrows.append(dict(label=label,mutant=None,case=id,profile=profile,variant=variant,control_passed=True,reference=check,run=run))
 print('VERIFIED UNMUTATED CONTROLS',len(control),'PASS; retained native rows',len(allrows),flush=True)
 for id in ['control-observed',*[f'M{i:02}' for i in range(1,8)]]:
  expected=4*(1 if id in ('control-observed','M01') else 8 if id=='M03' else 2)
  if sum(r['label'].startswith(id+'-') for r in allrows)==expected:continue
  exes=native_copy(b,id);family='thermal' if id in ('control-observed','M01','M02','M03') else 'dynamic';caseid={'control-observed':'T10-B','M01':'T10-B','M02':'T10-A','M03':'T10-C','M04':'D10-C','M05':'D10-A','M06':'D10-B','M07':'D10-A'}[id];kind=ord(caseid[-1])-65
  for profile in PROFILES:
   for variant in range(1 if id in ('control-observed','M01') else 8 if id=='M03' else 2):
    label=id+'-'+profile+'-'+str(variant);rc,h,rows,err,run=execute(b,label,exes[profile],kind,variant,family);check=reference(family,h,rows,cases[caseid]);details={}
    if id in ('M01','control-observed'):
     candidates=[json.loads(s.split(' ',1)[1]) for s in err.splitlines() if s.startswith('P105_CANDIDATE ')];assert candidates
     c=candidates[0];expected=c['dt']*10*(c['t0']-c['t1']);face_error=abs(c['q']-expected);storage_error=abs((c['e0']-c['old0'])+c['q']);details=dict(candidate=c,independent_face_j=expected,face_error_j=face_error,left_storage_join_error_j=storage_error,bound_j=1e-10)
     if id=='M01':assert rc==2 and face_error>1e-10 and storage_error>1e-10 and 'core_thermal_rod_step' in err;reason='independent face sign + per-cell candidate storage; unchanged native guard refused publication'
     else:assert rc==0 and check['passed'] and run['stdout_sha256']==control[profile,caseid,variant] and face_error<=1e-10 and storage_error<=1e-10;reason='observer control identical to uninstrumented control'
    else:
     if id=='M03':
      check=faint_prefix(h,rows,cases[caseid]);details=dict(accepted_prefix=check,native_later_refusal=rc==2,refusal_credited_as_detection=False)
      if rc==2:assert 'core_thermal_material_rod_step' in err
     else:assert rc==0,('native failure is not intended detector',id,err)
     assert not check['passed'],('SURVIVOR',id,profile,variant)
     wanted={'M02':'independent initial energy','M03':'cell_relative','M04':'W_independent_j','M05':'ledger_reconstruction_j','M06':'D_independent_j','M07':'native tick/time'}[id];assert wanted in check['failure'],(id,check)
     reason=wanted
     if id=='M02':
      expected=10+float(cases[caseid]['parameters']['power_w_profiles'][variant])/24;error=abs(rows[-1]['t'][0]-expected);assert error>1e-8;details=dict(independent_capacity_j_per_k=24,expected_final_temperature_k=expected,actual_final_temperature_k=rows[-1]['t'][0],error_k=error,bound_k=1e-8)
     if id=='M07':
      p=cases[caseid]['parameters'];k=p['youngs_modulus_pa']*sum(p['fiber_area_m2'])/p['length_m'];m=p['density_kg_m3']*sum(p['fiber_area_m2'])*p['length_m']/2+p['added_mass_kg_node1'];omega=math.sqrt(k/m);theta=2*math.atan(omega*h['dt']/2);initial=p['released_force_n_profiles'][variant]/k;witness=[]
      for r in rows:
       announced=round(r['time']/h['dt']);u=initial*math.cos(theta*announced);v=-initial*omega*math.sin(theta*announced)
       if r['time']!=r['tick']*h['dt'] and (abs(r['u'][3]-u)>1e-12 or abs(r['v'][3]-v)>1e-8):witness.append(dict(tick=r['tick'],time=r['time'],reference_u_at_announced_time=u,actual_u=r['u'][3],reference_v_at_announced_time=v,actual_v=r['v'][3]))
      assert witness;details=dict(clock_phase_witness=witness[0],wrong_phase_states=len(witness))
    allrows.append(dict(label=label,mutant=None if id=='control-observed' else id,case=caseid,profile=profile,variant=variant,intended_detector=reason,detected=id!='control-observed',control_passed=True,reference=check,details=details,run=run))
  dump(out/'results.json',dict(passed=True,rows=allrows,p10_6_started=False));print(id,'PASS',flush=True)
 return allrows

def bridges(b,products,thermal_native,sources,out):
 out.mkdir();rows=[]
 for id in ['control','M08','M09','M10']:
  clone=b.work/'adapter-copies-qualified'/id;shutil.copytree(b.structural/'tools',clone/'tools',ignore=shutil.ignore_patterns('__pycache__','*.pyc'));shutil.copyfile(b.structural/'VERSION',clone/'VERSION');(clone/'tests').mkdir(parents=True);shutil.copyfile(b.structural/'tests/thermal_frame_reference.py',clone/'tests/thermal_frame_reference.py')
  if id!='control':
   p=clone/'tools/ss9/assembly_cases.py';m=next(x for x in b.contract['mutants'] if x['id']==id);assert sha(m['site']['path'])==m['site']['sha256'];replace(p,*CHANGES[id]);dump(b.out/(id+'-mutation.json'),dict(binding=m,copy=str(p),before=CHANGES[id][0],after=CHANGES[id][1],mutated_sha256=sha(p)))
  for profile in PROFILES:
   selectors=[('TS10-B',1.2e-5,1)] if id=='M10' else [('TS10-A',alpha,load) for alpha in [1.2e-5,-1.2e-5] for load in [0,1]]+([('TS10-B',1.2e-5,1)] if id=='control' else [])
   for index,(case,alpha,load) in enumerate(selectors):
    dest=out/(id+'-'+profile+'-'+str(index));raw=b.run('bridge-'+id+'-'+profile+'-'+str(index),['/opt/homebrew/bin/python3','-B',ROOT/'bridge_worker.py','--structural',clone,'--canonical-structural',b.structural,'--initial',INITIAL,'--native',products[profile]['structural'],'--thermal-native',thermal_native,'--sources',sources,'--binding',b.binding,'--case',case,'--alpha',alpha,'--load-factor',load,'--mutant',id,'--out',dest],timeout=1200)
    row=json.loads(raw);row.update(profile=profile,case=case,alpha=alpha,load_factor=load,mutant=None if id=='control' else id);rows.append(row);dump(out/'results.json',dict(passed=True,rows=rows,p10_6_started=False))
  print(id,'BRIDGE PASS',flush=True)
 return rows

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--binding',type=Path,required=True);p.add_argument('--out',type=Path,required=True);p.add_argument('--resume',action='store_true');a=p.parse_args();a.out.mkdir(exist_ok=a.resume)
 if a.resume:
  with (a.out/'build-state.pickle').open('rb') as f:b,products,thermal_native,sources=pickle.load(f)
  assert b.binding==a.binding and b.out==a.out/'build';b.commands=json.loads((b.out/'commands.json').read_text());b.freshness()
 else:
  b=Build(a.binding,a.out/'build');products,thermal_native=b.build()
  from bridge import prepare
  sources=prepare(b,thermal_native)
 dump(a.out/'build-complete.json',dict(passed=True,products=products,thermal_native=str(thermal_native),sources=str(sources)))
 # Trusted task-local resume state only, retired with native scratch at closure.
 with (a.out/'build-state.pickle').open('wb') as f:pickle.dump((b,products,thermal_native,sources),f)
 n=native(b,products,a.out/'native');g=bridges(b,products,thermal_native,sources,a.out/'bridge-qualified');b.freshness();dump(b.out/'dependencies.json',b.dependencies);dump(b.out/'objects.json',b.objects)
 ids=sorted({x['mutant'] for x in n+g if x.get('detected')});assert ids==[f'M{i:02}' for i in range(1,11)]
 dump(a.out/'result.json',dict(status='complete',passed=True,checkpoint='P10.5',mutants_detected=ids,profiles=list(PROFILES),native_rows=len(n),bridge_rows=len(g),native=n,bridge=g,benchmark_runs=0,p10_6_started=False,adoption=False,installation=False,release=False));print('P10.5 TEN DETECTED; STOP BEFORE P10.6',flush=True)
if __name__=='__main__':main()
