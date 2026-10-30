"""Local build/input identities for the explicit experimental adapter workflow."""
import hashlib,json,os,shlex,shutil,subprocess
from pathlib import Path

class ProvenanceError(ValueError):pass
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def require(condition,message):
 if not condition:raise ProvenanceError(message)
def object_value(value,label):
 require(isinstance(value,dict),label+' must be a JSON object');return value
def read_object(path,label):return object_value(json.loads(Path(path).read_text()),label)
def hash_map(value,label):
 object_value(value,label)
 require(bool(value) and all(isinstance(k,str) and k and isinstance(v,str) and len(v)==64 and all(c in '0123456789abcdef' for c in v) for k,v in value.items()),label+' must contain path/SHA256 entries')
 return value
def path_list(value,label):
 require(isinstance(value,list) and bool(value) and all(isinstance(p,str) and Path(p).is_absolute() for p in value),label+' must be an array of absolute path strings');return value
def native_inputs(root):
 root=Path(root).resolve();paths=[]
 for prefix in ['src','include',*[f'third_party/codework_shared/core/{n}' for n in ('core_base','core_io','core_data','core_pack')]]:
  directory=root/prefix
  require(directory.is_dir(),'native source directory missing: '+prefix)
  require(directory.resolve().is_relative_to(root),'native source directory escapes compiler root: '+prefix)
  paths.extend(p for p in directory.rglob('*') if p.is_file() and p.suffix in ('.c','.h','.def','.inc'))
 paths.extend((root/'make').glob('*.mk'));paths.extend(root/n for n in ('makefile','VERSION'))
 require(all(not p.is_symlink() or p.resolve().is_relative_to(root) for p in paths),'native source escapes compiler root')
 return {str(p.relative_to(root)):sha(p) for p in sorted(paths)}

def build_environment(out):
 """Only tool discovery uses caller PATH; no caller Make/CPP/SDK overrides survive."""
 tools={}
 for name in ('make','cc','llvm-config','ar','xcrun'):
  selected=shutil.which(name);require(selected is not None,'required build tool missing: '+name);tools[name]=str(Path(selected).resolve())
 paths=list(dict.fromkeys([str(Path(p).parent) for p in tools.values()]+['/usr/bin','/bin','/usr/sbin','/sbin']))
 tmp=Path(out)/'tmp';tmp.mkdir()
 env=dict(PATH=os.pathsep.join(paths),LC_ALL='C',LANG='C',TMPDIR=str(tmp))
 require(str(Path(shutil.which('ar',path=env['PATH'])).resolve())==tools['ar'],'archive tool differs from fixed build PATH')
 def query(name,*args):return subprocess.check_output([tools[name],*args],env=env,text=True).strip()
 sdk=Path(query('xcrun','--show-sdk-path')).resolve();resource=Path(query('cc','-print-resource-dir')).resolve();llvm=Path(query('llvm-config','--includedir')).resolve()
 env['SDKROOT']=str(sdk)
 config=dict(schema='fisiCs.experimental.native_build_configuration.v2',environment=env,tools={n:dict(path=p,sha256=sha(p)) for n,p in tools.items()},sdk=str(sdk),resource=str(resource),llvm_include=str(llvm),cc_version=query('cc','--version'),llvm_version=query('llvm-config','--version'),llvm_flags=query('llvm-config','--cflags','--ldflags','--libs','core','native','nativecodegen','x86codegen','passes'))
 headers={}
 for anchor in (sdk/'usr/include',resource/'include',llvm):
  require(anchor.is_dir(),'toolchain header root missing: '+str(anchor))
  for p in anchor.rglob('*'):
   if p.is_file():headers[str(p.resolve())]=sha(p)
 return env,tools,config,headers

def build_dependencies(build,root,source_inputs,headers):
 """Check real compiler-emitted dependencies against pre-build native/toolchain bytes."""
 root=Path(root).resolve();known={str((root/p).resolve()):h for p,h in source_inputs.items()};known.update(headers);actual={};files=list(Path(build).rglob('*.d'))
 require(bool(files),'build emitted no dependency files')
 for file in files:
  rule=file.read_text().replace('\\\n',' ').splitlines()[0]
  require(':' in rule,'malformed build dependency file: '+str(file))
  for token in shlex.split(rule.split(':',1)[1]):
   path=Path(token);path=(root/path).resolve() if not path.is_absolute() else path.resolve();key=str(path)
   require(key in known,'unrecorded build dependency: '+key)
   actual[key]=sha(path);require(actual[key]==known[key],'build dependency changed during compilation: '+key)
 require(bool(actual),'build dependency closure empty');return dict(sorted(actual.items()))

def validate_configuration(value):
 doc=object_value(value,'build configuration');require(doc.get('schema')=='fisiCs.experimental.native_build_configuration.v2','build configuration schema mismatch')
 env=object_value(doc.get('environment'),'build environment');require(set(env)=={'PATH','LC_ALL','LANG','TMPDIR','SDKROOT'} and all(isinstance(v,str) and v for v in env.values()),'unsupported build environment')
 tools=object_value(doc.get('tools'),'build tools');require(set(tools)=={'make','cc','llvm-config','ar','xcrun'},'build tools incomplete')
 for value in tools.values():
  value=object_value(value,'build tool');require(isinstance(value.get('path'),str) and Path(value['path']).is_absolute() and isinstance(value.get('sha256'),str) and len(value['sha256'])==64,'invalid build tool identity')
 for key in ('sdk','resource','llvm_include','cc_version','llvm_version','llvm_flags'):require(isinstance(doc.get(key),str) and bool(doc[key]),'invalid build configuration field: '+key)
 return doc

def frontend_receipt(path,root,archive):
 doc=read_object(path,'frontend build receipt');root=Path(root).resolve();archive=Path(archive).resolve()
 require(doc.get('schema')=='fisiCs.experimental.frontend_build.v2' and doc.get('passed') is True,'frontend build receipt schema/status mismatch')
 require(doc.get('source_root')==str(root),'frontend source root mismatch')
 require(doc.get('archive')==str(archive),'frontend archive path mismatch')
 require(doc.get('archive_sha256')==sha(archive),'frontend archive changed')
 require(hash_map(doc.get('source_inputs'),'native source inputs')==native_inputs(root),'native source changed since frontend build')
 config=validate_configuration(doc.get('build_configuration'));inputs=hash_map(doc.get('build_inputs'),'build inputs')
 anchors=[root,*[Path(config[k]).resolve() for k in ('sdk','resource','llvm_include')]]
 for name,h in inputs.items():
  p=Path(name);require(p.is_absolute() and any(p.resolve().is_relative_to(a) for a in anchors),'build input outside native/toolchain roots: '+name);require(sha(p)==h,'build input changed since frontend build: '+name)
 for tool in config['tools'].values():require(sha(tool['path'])==tool['sha256'],'build tool changed since frontend build')
 return doc

def adapter_receipt(path,root,adapter):
 doc=read_object(path,'adapter build receipt');root=Path(root).resolve();adapter=Path(adapter).resolve()
 require(doc.get('schema')=='fisiCs.experimental.adapter_build.v2' and doc.get('passed') is True,'adapter build receipt schema/status mismatch')
 require(doc.get('source_root')==str(root),'adapter native source root mismatch')
 require(doc.get('adapter')==str(adapter) and doc.get('adapter_sha256')==sha(adapter),'adapter binary changed or path mismatch')
 require(doc.get('source_sha256')==sha(Path(__file__).with_name('ast_export.c')),'adapter source changed')
 require(hash_map(doc.get('source_inputs'),'native source inputs')==native_inputs(root),'native source changed since adapter build')
 validate_configuration(doc.get('build_configuration'))
 require(isinstance(doc.get('build_inputs_sha256'),str) and len(doc['build_inputs_sha256'])==64 and type(doc.get('build_input_count')) is int and doc['build_input_count']>0,'adapter native dependency identity missing')
 require(isinstance(doc.get('frontend_sha256'),str) and len(doc['frontend_sha256'])==64,'adapter frontend identity missing')
 # A static adapter binds its completed build; past headers/tools/archive need
 # not remain installed at analysis time. Current analyzed inputs are CLI-pinned.
 return doc
