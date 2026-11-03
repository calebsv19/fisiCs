"""Small P11.3/P11.4 adapters over the maintained Phase10 builder/reference."""
from pathlib import Path
import sys,json,subprocess,gzip,hashlib,time
ROOT=Path(__file__).resolve().parent;INITIAL=ROOT.parent/'phase10_initial';sys.path.insert(0,str(INITIAL))
from build import Build,PROFILES,dump,sha
from references import thermal,dynamic

class SerialBuild(Build):
 def run(self,label,argv,cwd=None,timeout=300):
  return super().run(str(len(self.commands)).zfill(4)+'-'+label,argv,cwd,timeout)

def command(b,label,argv,expected=0,timeout=300,cwd=None):
 argv=list(map(str,argv));label=str(len(b.commands)).zfill(4)+'-'+label;start=time.monotonic()
 try:q=subprocess.run(argv,cwd=cwd,env=b.env,capture_output=True,timeout=timeout)
 except subprocess.TimeoutExpired:
  b.commands.append(dict(label=label,argv=argv,watchdog_s=timeout,timed_out=True));dump(b.out/'commands.json',b.commands);raise AssertionError((label,'timeout is not detection'))
 for stream,raw in [('stdout',q.stdout),('stderr',q.stderr)]:(b.logs/(label+'.'+stream+'.gz')).write_bytes(gzip.compress(raw,mtime=0))
 row=dict(label=label,argv=argv,expected_exit=expected,exit=q.returncode,watchdog_s=timeout,elapsed_s=time.monotonic()-start,stdout_sha256=hashlib.sha256(q.stdout).hexdigest(),stderr_sha256=hashlib.sha256(q.stderr).hexdigest());b.commands.append(row);dump(b.out/'commands.json',b.commands);assert q.returncode==expected,(label,q.returncode,q.stderr.decode(errors='replace')[-1800:]);return q,row

def physical_flags(b,family):return ['--overlay=physics-semantics','--physical-semantics-policy',b.contract['policies'][family]]
def profile_flags(b,family,profile):return ['--overlay=physics-units'] if profile=='fisics-units' else physical_flags(b,family) if profile=='fisics-physical' else []
def substitute(argv,out):return [str(x).replace('__TASK_ROOT__',str(out)) for x in argv]

def selected_report(b,wf,source,out,raw=False,label='report'):
 argv=wf['raw_report_argv'] if raw else wf['strict_report_argv'];args=list(argv);args[-1]=str(source);args[args.index('--emit-physical-semantics-json')+1]=str(out);q,cmd=command(b,label,args,timeout=300);v=json.loads(out.read_text());assert v['analysis_valid'];assert raw or v['selection']['enumeration_complete'];expected=wf['expected_prior_raw_selection'] if raw else wf['expected_prior_strict_selection'];s=v['selection'];assert not s['accepted'] if raw else s['accepted'];assert {k:s[k] for k in ['total','required','ordinary_controls','checked','permitted_trust','blocked']}==expected
 return dict(path=str(out),sha256=sha(out),selection=s,command=cmd)

def native_reference(b,wf,profile,exe,label):
 family=wf['family'];assert family in ['T1','D1'];kind=2 if family=='T1' else 0;q,cmd=command(b,label,[exe,kind,0],timeout=300 if family=='T1' else 180);assert len(q.stdout)<=(16 if family=='T1' else 32)*1024**2;values=[json.loads(x) for x in q.stdout.splitlines()];case=next(c for c in b.contract['cases'] if c['id']==wf['case']);v=(thermal if family=='T1' else dynamic)(values[0],values[1:],case);assert v['passed'];return dict(passed=True,profile=profile,family=family,reference=v,command=cmd,output_sha256=hashlib.sha256(q.stdout).hexdigest())

def compile_copy(b,family,copy,group,only_source=None,reject=False):
 """Source-local copies preserve quote-include resolution; owners stay read only."""
 copy=Path(copy);original=b.inc;b.inc=['-I'+str(copy/'src'),'-I'+str(copy/'include')]+original;products={};rows=[]
 try:
  for profile in PROFILES:
   out=b.work/'p11-copies'/group/profile;out.mkdir(parents=True);parts=[]
   for name in b.contract['selected_translation_units'][family]:
    if only_source and Path(name).name!=only_source:continue
    source=copy/'src'/Path(name).name;obj=out/(source.stem+'.o')
    if profile=='matched-clang':b.compile_native(source,obj);cmd=None
    else:
     prepared=b.prepared(source,family,profile,out/(source.stem+'.c'));bad=reject and profile in ['fisics-units','fisics-physical'];q,cmd=command(b,group+'-'+profile,[b.fisics,*profile_flags(b,family,profile),'-c',prepared,'-o',obj],expected=1 if bad else 0)
     if bad:
      text=(q.stdout+q.stderr).decode(errors='replace');assert re_dimension(text),(group,profile,'not intended dimension detector',text[-900:]);assert not obj.exists();rows.append(dict(profile=profile,passed=True,rejected=True,command=cmd,diagnostic=text));continue
    assert obj.is_file();parts.append(obj)
   if reject:
    if profile in ['matched-clang','fisics-base']:rows.append(dict(profile=profile,passed=True,rejected=False,command=cmd,legal_C=True))
    continue
   if family=='T1':objects=[b.work/'thermal-driver.o',*parts]
   else:
    common=[]
    for obj in sorted((b.work/'native-objects').glob('*.o')):
     source=b.objects[str(obj)]['source']
     if source not in b.contract['selected_translation_units']['D1'] and source!=str(b.structural/'src/main.c'):common.append(obj)
    objects=[b.work/'dynamic-driver.o',*common,*parts]
   products[profile]=b.link('p11-copies/'+group+'/'+profile+'/probe',objects)
 finally:b.inc=original
 return products,rows

def re_dimension(text):
 import re
 return bool(re.search(r'\b410[1-5]\b|units subtraction mismatch:|dimension.{0,90}(mismatch|incompat)|incompat.{0,90}dimension',text,re.I))
