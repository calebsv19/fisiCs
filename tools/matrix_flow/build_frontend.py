#!/usr/bin/env python3
"""Build a fresh identified frontend in explicit contained output."""
import argparse,json,subprocess,time
from pathlib import Path
from provenance import native_inputs,sha,require,build_environment,build_dependencies

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler-root',type=Path,required=True);p.add_argument('--out',type=Path,required=True);p.add_argument('--with-compiler',action='store_true');p.add_argument('--jobs',type=int,default=2);a=p.parse_args();root=a.compiler_root.resolve();out=a.out.resolve()
 try:out.mkdir(parents=True,exist_ok=False)
 except OSError as e:print(e);return 2
 result=dict(schema='fisiCs.experimental.frontend_build.v2',passed=False,source_root=str(root));start=time.monotonic()
 try:
  require(1<=a.jobs<=8,'jobs outside1..8');before=native_inputs(root);archive=out/'libfrontend.a';env,tools,config,headers=build_environment(out)
  fixed=['BUILD_PROFILE=unsanitized','BUILD_DIR='+str(out/'build'),'LIB_FRONTEND='+str(archive),'CC='+tools['cc'],'LLVM_CONFIG='+tools['llvm-config'],'DEPFLAGS=-MD -MP','SHARED_ROOT=third_party/codework_shared','SHIM_MODE=off','CODEGEN_DEBUG=0','CODEGEN_TRACE=0','PARSER_DEBUG=0','DISABLE_CODEGEN=0','INCLUDE_PATHS=include','RELEASE_VERSION_FILE=VERSION']
  fixed += ['CORE_'+n.upper()+'_DIR=third_party/codework_shared/core/core_'+n for n in ('base','io','data','pack')]
  cmd=[tools['make'],'-j'+str(a.jobs),*fixed,'frontend'];result.update(build_configuration=config,argv=cmd)
  with (out/'build.log').open('w') as log:q=subprocess.run(cmd,cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT)
  result['exit']=q.returncode;require(q.returncode==0,'frontend build failed; see build.log');require(before==native_inputs(root),'native source changed during frontend build')
  if a.with_compiler:
   cmd=cmd[:-1]+['BIN='+str(out/'fisics'),str(out/'fisics')]
   with (out/'compiler-build.log').open('w') as log:q=subprocess.run(cmd,cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT)
   result['compiler_argv']=cmd;require(q.returncode==0,'compiler build failed; see compiler-build.log');require(before==native_inputs(root),'native source changed during compiler build');result.update(compiler=str(out/'fisics'),compiler_sha256=sha(out/'fisics'))
  dependencies=build_dependencies(out/'build',root,before,headers)
  for tool in config['tools'].values():require(sha(tool['path'])==tool['sha256'],'build tool changed during compilation')
  result.update(passed=True,source_inputs=before,build_inputs=dependencies,archive=str(archive),archive_sha256=sha(archive))
 except (OSError,ValueError,subprocess.SubprocessError) as e:result['diagnostic']=type(e).__name__+': '+str(e)
 result['seconds']=time.monotonic()-start;(out/'result.json').write_text(json.dumps(result,indent=2)+'\n');return 0 if result['passed'] else 1
if __name__=='__main__':raise SystemExit(main())
