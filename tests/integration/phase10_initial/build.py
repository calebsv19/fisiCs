"""Contained, explicit tool/object construction for the bound P10.2 cohort."""
from pathlib import Path
import hashlib,json,os,re,shlex,subprocess,time
PROFILES=('matched-clang','fisics-base','fisics-units','fisics-physical')
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def dump(p,v):Path(p).write_text(json.dumps(v,indent=2,allow_nan=False)+'\n')
class Build:
 def __init__(self,binding,out):
  self.binding=Path(binding);self.contract=json.loads((self.binding/'bindings.json').read_text());self.pins=json.loads((self.binding/'owner-inputs.json').read_text());self.out=Path(out);self.out.mkdir(exist_ok=False);self.work=self.out/'work';self.work.mkdir();self.logs=self.out/'logs';self.logs.mkdir();self.commands=[];self.dependencies={};self.objects={};self.source=Path(__file__).resolve().parent
  self.workspace=Path(self.contract['preparation_parameters']['fisiCs_include_root']).parents[1];self.structural=self.workspace/'_worktrees/structural_sim_main_edit';self.thermal=self.workspace/'_worktrees/thermal_sim_main_edit';self.core=self.workspace/'shared/core';self.tool=json.loads((self.binding/'toolchain.json').read_text());self.clang=self.tool['clang_path'];self.fisics=self.contract['profiles'][1]['compiler']
  self.env={k:v for k,v in os.environ.items() if k not in ['MAKEFLAGS','MAKEOVERRIDES','MFLAGS','MAKELEVEL','CC','CFLAGS','CPPFLAGS','LDFLAGS','CPATH','C_INCLUDE_PATH','CPLUS_INCLUDE_PATH','SDKROOT','MACOSX_DEPLOYMENT_TARGET','FISICS_OVERLAY','LIBRARY_PATH']}
  temp=self.work/'tmp';temp.mkdir();self.env.update(PYTHONDONTWRITEBYTECODE='1',FISICS_MAX_PROCS='1',TMPDIR=str(temp),PATH='/opt/homebrew/bin:/usr/bin:/bin:/usr/sbin:/sbin')
  self.flags=['-std=c11','-O0','-ffp-contract=off','-isysroot',self.tool['sdk_path']];self.inc=['-I'+p for p in self.contract['preparation_parameters']['core_include_roots']]+['-I'+str(self.core/'core_structural/src'),'-I'+str(self.workspace/'fisiCs/include')]
  self.freshness()
 def freshness(self):
  drift=[p for p,h in self.pins.items() if not Path(p).is_file() or sha(p)!=h];assert not drift,drift
  assert sha(self.clang)==self.tool['clang_sha256'];assert sha(self.fisics)=='0e253c3057e83e69f727202a74a9db254d563a0af5039bd7a47c53793a62cd05'
 def run(self,label,argv,cwd=None,timeout=300):
  command=list(map(str,argv));t=time.monotonic()
  try:q=subprocess.run(command,cwd=cwd,env=self.env,capture_output=True,timeout=timeout)
  except subprocess.TimeoutExpired as e:
   self.commands.append(dict(label=label,argv=command,timeout=timeout,timed_out=True));dump(self.out/'commands.json',self.commands);raise
  (self.logs/(label+'.stdout')).write_bytes(q.stdout);(self.logs/(label+'.stderr')).write_bytes(q.stderr);self.commands.append(dict(label=label,argv=command,cwd=str(cwd) if cwd else None,exit=q.returncode,elapsed_s=time.monotonic()-t,watchdog_s=timeout));dump(self.out/'commands.json',self.commands)
  assert q.returncode==0,(label,q.stderr.decode(errors='replace')[-2400:]);return q.stdout
 def compile_native(self,source,target,inc=None,cwd=None):
  target.parent.mkdir(parents=True,exist_ok=True);dep=target.with_suffix('.d');self.run('cc-'+str(len(self.commands)),[self.clang,*self.flags,*(inc or self.inc),'-Wno-deprecated-declarations','-MD','-MF',dep,'-c',source,'-o',target],cwd)
  raw=dep.read_text().replace('\\\n',' ');names=shlex.split(raw.split(':',1)[1]);deps={}
  for n in names:
   p=Path(n);p=(Path(cwd)/p if cwd and not p.is_absolute() else p).resolve();deps[str(p)]=sha(p)
  self.dependencies.update(deps);self.objects[str(target)]=dict(source=str(source),sha256=sha(target),dependencies=deps)
  return target
 def prepared(self,source,family,profile,out):
  macros=self.contract['preparation_parameters']['family_macros'][family]
  raw=self.run('pre-'+family+'-'+profile+'-'+Path(source).stem,[self.clang,'-std=c11','-fno-blocks','-E','-P','-isysroot',self.tool['sdk_path'],*self.inc,*['-D'+s for s in macros],source]).decode()
  allowed={'dim','unit','quantity_kind','physical_origin','physical_operation','energy_form'}
  def erase(m):
   name=m[1];assert name in allowed,('unexpected attribute',name)
   keep=profile=='fisics-physical' or (profile=='fisics-units' and name in {'dim','unit'})
   return m[0] if keep else ''.join('\n' if c=='\n' else ' ' for c in m[0])
  raw=re.sub(r'\[\[fisics::(\w+)\s*\([^\]]*\)\]\]',erase,raw)
  assert not re.search(r'\[\[fisics::',re.sub(r'\[\[fisics::(dim|unit|quantity_kind|physical_origin|physical_operation|energy_form)\s*\([^\]]*\)\]\]','',raw))
  out.write_text(raw);return out
 def selected(self,family):
  result={}
  for profile in PROFILES:
   d=self.work/family/profile;d.mkdir(parents=True);result[profile]={}
   for name in self.contract['selected_translation_units'][family]:
    source=Path(name);target=d/(source.stem+'.o')
    if profile=='matched-clang':self.compile_native(source,target)
    else:
     prepared=self.prepared(source,family,profile,d/(source.stem+'.c'));flags=[]
     if profile=='fisics-units':flags=['--overlay=physics-units']
     if profile=='fisics-physical':flags=['--overlay=physics-semantics','--physical-semantics-policy',self.contract['policies'][family]]
     self.run('fis-'+family+'-'+profile+'-'+source.stem,[self.fisics,*flags,'-c',prepared,'-o',target])
     self.objects[str(target)]=dict(source=str(source),prepared=str(prepared),prepared_sha256=sha(prepared),sha256=sha(target),profile=profile)
    info=self.run('arch-'+str(len(self.commands)),['/usr/bin/file',target]);assert b'arm64' in info and b'Mach-O' in info
    result[profile][source.stem]=target
  return result
 def link(self,name,objects):
  p=self.work/name;p.parent.mkdir(parents=True,exist_ok=True);self.run('link-'+str(len(self.commands)),[self.clang,'-isysroot',self.tool['sdk_path'],*objects,'-lm','-o',p]);return p
 def build(self):
  # Query the maintained source/include lists without invoking any build/clean target.
  query=self.work/'sources.mk';query.write_text('include '+str(self.structural/'make/config.mk')+'\n.PHONY: p102-query\np102-query:\n\t@printf \'%s\\n\' \'$(SOURCES)\' \'$(CPPFLAGS)\'\n')
  lines=self.run('source-query',['/usr/bin/make','-s','-f',query,'p102-query','SHARED_ROOT='+str(self.workspace/'shared')],self.structural).decode().splitlines();assert len(lines)==2
  sources=[(self.structural/p).resolve() for p in shlex.split(lines[0])];includes=shlex.split(lines[1]);assert len(sources)==len(set(sources));common={}
  for i,p in enumerate(sources):common[str(p)]=self.compile_native(p,self.work/'native-objects'/(str(i)+'-'+p.stem+'.o'),includes,self.structural)
  dyn=self.selected('D1');ts=self.selected('TS1');thermal=self.selected('T1')
  ddriver=self.compile_native(self.source/'dynamic.c',self.work/'dynamic-driver.o');tdriver=self.compile_native(self.source/'thermal.c',self.work/'thermal-driver.o')
  products={}
  for profile in PROFILES:
   d=dict(common);d.update({str(Path(p)):dyn[profile][Path(p).stem] for p in self.contract['selected_translation_units']['D1']});d.pop(str(self.structural/'src/main.c'));dexe=self.link(profile+'/dynamic',[ddriver,*d.values()])
   d=dict(common);d.update({str(Path(p)):ts[profile][Path(p).stem] for p in self.contract['selected_translation_units']['TS1']});host=self.link(profile+'/structural/bin/structural_sim',list(d.values()))
   # Existing native identity serializer, with actual selected objects and dependencies included.
   inputs=set(self.dependencies)|set(map(str,ts[profile].values()))|{str(self.core/'core_structural/VERSION'),str(self.structural/'VERSION')}
   inputs|={str(Path(self.contract['policies']['TS1']))}
   args=['/opt/homebrew/bin/python3','-B',self.structural/'tools/build_identity.py','--output',host.parent.parent/'build_identity.json','--binary',host,'--compiler',self.clang,'--flags',json.dumps(dict(native=self.flags,profile=profile,selected='TS1')),'--inputs',*sorted(inputs)]
   self.run('identity-capture-'+profile,[*args,'--mode','capture']);self.run('identity-finalize-'+profile,[*args,'--mode','finalize'])
   texe=self.link(profile+'/thermal',[tdriver,*thermal[profile].values()]);products[profile]=dict(dynamic=str(dexe),structural=str(host),thermal=str(texe))
  # Prescribed thermal source producer: unchanged owner Make route, fresh dedicated output.
  native=self.work/'thermal-native'
  self.run('thermal-native-build',['/usr/bin/make','-j1','build-assembly','BUILD_DIR='+str(native),'SHARED_ROOT='+str(self.workspace/'shared'),'CC='+self.clang,'PYTHON=/opt/homebrew/bin/python3','CFLAGS=-std=c11 -O0 -ffp-contract=off -MMD -MP -isysroot '+self.tool['sdk_path']],self.thermal,300)
  for p in native.rglob('*.d'):
   raw=p.read_text().replace('\\\n',' ');first=raw.split('\n')[0]
   for name in shlex.split(first.split(':',1)[1]):
    f=Path(name);f=(self.thermal/f if not f.is_absolute() else f).resolve();self.dependencies[str(f)]=sha(f)
  self.freshness();dump(self.out/'dependencies.json',self.dependencies);dump(self.out/'objects.json',self.objects);dump(self.out/'products.json',dict(profiles=products,thermal_native=str(native/'bin/thermal_assembly_agent')))
  return products,native/'bin/thermal_assembly_agent'
