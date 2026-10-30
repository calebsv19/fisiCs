#!/usr/bin/env python3
"""Opt-in connected prescribed-temperature qualification; compiler and solver owners stay separate.
Writes only a fresh --out tree. Uses an explicitly supplied core build and native
baseline, never default binaries, installed apps, packages or solver equations.
"""
import argparse, hashlib, json, os, re, subprocess, time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__)
 for n in ('compiler','core-root','core-build','structural-root','native','thermal-native','out'): p.add_argument('--'+n,type=Path,required=True)
 a=p.parse_args()
 for k,v in vars(a).items():setattr(a,k,v.resolve())
 a.out.mkdir(parents=True,exist_ok=False);o=a.out;c=a.core_root;s=a.structural_root
 env=os.environ.copy()
 for n in ('MAKEFLAGS','MFLAGS','MAKEOVERRIDES','MAKELEVEL','BIN','BUILD_DIR','LIB_FRONTEND','EXAMPLES_BUILD_DIR','CC','CFLAGS','CPPFLAGS','LDFLAGS','LDLIBS'):env.pop(n,None)
 env.update(PYTHONDONTWRITEBYTECODE='1',FISICS_MAX_PROCS='0',TMPDIR=str(o))
 result=dict(passed=False,commands=[],scope='selected thermal TU in Clang native connected-frame host; no whole-solver strict physical acceptance')
 inputs=[a.compiler,a.native,a.thermal_native,Path(__file__),c/'src/core_structural_thermal_strain.c',c/'physics/frame_policy.json',c/'physics/frame_catalog.json',*sorted((c/'src').glob('*.h')),*sorted(a.core_build.glob('*.o')),*sorted((s/'tools/ss9').glob('*.py')),*sorted((s/'tests').glob('*thermal_assembly*.py')),s/'tests/thermal_frame_reference.py']
 result['inputs']={str(q):sha(q) for q in inputs}
 def run(label,args,cwd=s,expected=0):
  cmd=list(map(str,args));start=time.monotonic();q=subprocess.run(cmd,cwd=cwd,env=env,capture_output=True,text=True,timeout=1200)
  (o/(label+'.log')).write_text(q.stdout+q.stderr);result['commands'].append(dict(label=label,argv=cmd,cwd=str(cwd),exit=q.returncode,expected=expected,seconds=time.monotonic()-start));save()
  assert q.returncode==expected,(label,(q.stdout+q.stderr)[-2500:]);print(label,'PASS',flush=True);return q.stdout
 def save(): (o/'results.json').write_text(json.dumps(result,indent=2,sort_keys=True)+'\n')
 def suite(label,binary):
  run(label+'-entry',['python3','-B','tests/qualify_thermal_assembly_entry.py','--binary',binary,'--thermal-native',a.thermal_native,'--scratch',o/(label+'-entry'),'--output',o/(label+'-entry.json')])
  fixtures=o/(label+'-entry/workspace')
  run(label+'-solve',['python3','-B','tests/qualify_thermal_assembly_solve.py','--binary',binary,'--thermal-native',a.thermal_native,'--fixtures',fixtures,'--scratch',o/(label+'-solve'),'--output',o/(label+'-solve.json')])
  return fixtures,o/(label+'-solve/workspace')
 try:
  run('source-contracts',['python3','-B',ROOT/'tests/integration/run_thermostructural_source_contracts.py','--compiler',a.compiler,'--core-root',c,'--out',o/'source-contracts'],ROOT)
  inc=['-I'+str(ROOT/'include'),'-I'+str(c/'include'),'-I'+str(c.parent/'core_base/include')]
  pre=run('preprocess',['clang','-std=c11','-fno-blocks','-E','-P','-D__FISICS__=1','-DCORE_STRUCTURAL_PHYSICAL_SEMANTICS=1',*inc,c/'src/core_structural_thermal_strain.c'])
  def erase(text,names):return re.sub(r'\[\[\s*fisics::(?:'+names+r')\s*\(.*?\)\s*\]\]',lambda m:''.join('\n' if x=='\n' else ' ' for x in m[0]),text,flags=re.S)
  texts={'base':erase(pre,'dim|unit|quantity_kind|physical_origin|physical_operation|energy_form'),'units':erase(pre,'quantity_kind|physical_origin|physical_operation|energy_form'),'physical':pre}
  fixtures,baseline=suite('baseline',a.native);variants={};objects=[q for q in sorted(a.core_build.glob('*.o')) if q.stem!='core_structural_thermal_strain'];assert len(objects)>50
  expected=None
  for mode in ('matched-clang','base','units','physical'):
   src=o/(mode+'.c');src.write_text(texts['base' if mode=='matched-clang' else mode]);obj=o/(mode+'.o');flags=[] if mode in ('base','matched-clang') else ['--overlay=physics-'+('units' if mode=='units' else 'semantics')]
   if mode=='physical':flags+=['--physical-semantics-policy',c/'physics/frame_policy.json']
   run(mode+'-compile',(['clang','-std=c11','-O0','-ffp-contract=off'] if mode=='matched-clang' else [a.compiler,*flags])+['-c',src,'-o',obj],ROOT)
   archive=o/(mode+'.a');run(mode+'-archive',['ar','rcs',archive,*objects,obj])
   host=o/(mode+'-host');binary=host/'bin/structural_sim'
   run(mode+'-host',['make',binary,'identity','BUILD_DIR='+str(host),'CORE_SOURCES='+str(c.parent/'core_units/src/core_units.c'),'LDFLAGS=-lm '+str(archive)])
   _,workspace=suite(mode,binary)
   # Snapshot numeric/provenance payloads exclude executable identity in receipt.
   snapshots={}
   for q in sorted((workspace/'thermal_runs').glob('*/case-*.json')):
    if q.name.endswith('-thermal.json'):continue
    snapshots[str(q.relative_to(workspace/'thermal_runs'))]=sha(q)
   if expected is None:expected=snapshots
   else:assert snapshots==expected,(mode,'connected snapshot mismatch against matched Clang')
   variants[mode]=dict(object_sha256=sha(obj),binary_sha256=sha(binary),snapshot_count=len(snapshots),exact_snapshot_parity=True)
   # Reciprocal ABI: original public C driver compiled by fisiCs; archive Clang.
   driver=run(mode+'-driver-preprocess',['clang','-std=c11','-fno-blocks','-E','-P',*inc,c/'tests/core_structural_thermal_strain_test.c']);dp=o/(mode+'-driver.c');dp.write_text(driver)
   run(mode+'-reverse-compile',[a.compiler,'-c',dp,'-o',o/(mode+'-driver.o')],ROOT)
   run(mode+'-reverse-link',['clang',o/(mode+'-driver.o'),a.core_build/'libcore_structural.a','-lm','-o',o/(mode+'-reverse')]);run(mode+'-reverse-run',[o/(mode+'-reverse')])
   run(mode+'-cli',['python3','-B','tests/test_cli.py','--binary',binary])
  result['variants']=variants
  binary=o/'physical-host/bin/structural_sim'
  run('external-transports',['python3','-B','tests/qualify_thermal_assembly_transport.py','--binary',binary,'--thermal-native',a.thermal_native,'--fixtures',fixtures,'--scratch',o/'transports','--output',o/'transports.json'])
  run('inspection',['python3','-B','tests/test_thermal_assembly_inspection.py','--binary',binary,'--thermal-native',a.thermal_native,'--studies',o/'physical-solve/workspace','--scratch',o/'inspection','--output',o/'inspection.json'])
  assert all(Path(n).is_file() and sha(Path(n))==h for n,h in result['inputs'].items()),'input drift'
  result['passed']=True
 finally:save()
 print('Connected assembly compiler adoption: PASS')
if __name__=='__main__':main()
