#!/usr/bin/env python3
"""Build the optional internal AST adapter against this checkout's frontend."""
import argparse,hashlib,json,shlex,subprocess
from pathlib import Path
from provenance import frontend_receipt,native_inputs,sha,require,build_environment,build_dependencies

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler-root',type=Path,required=True);p.add_argument('--frontend-archive',type=Path,required=True);p.add_argument('--out',type=Path,required=True);p.add_argument('--frontend-receipt',type=Path,required=True);a=p.parse_args();root=a.compiler_root.resolve();out=a.out.resolve()
 try:out.mkdir(parents=True,exist_ok=False)
 except OSError as e:print(type(e).__name__+': '+str(e));return 1
 report=dict(schema='fisiCs.experimental.adapter_build.v2',passed=False,source_root=str(root))
 try:
  proof=frontend_receipt(a.frontend_receipt,root,a.frontend_archive);source=Path(__file__).with_name('ast_export.c');source_before=sha(source)
  env,tools,config,headers=build_environment(out)
  require(all(config[k]==proof['build_configuration'][k] for k in ('tools','sdk','resource','llvm_include','cc_version','llvm_version','llvm_flags')),'adapter/frontend toolchain configuration mismatch')
  flags=shlex.split(config['llvm_flags']);include=['src','src/Lexer','src/Parser','src/Syntax','src/Syntax/Expr','src/Syntax/Decls','src/CodeGen','src/CodeGen/Expr'];paths=[root/x for x in include]+[root/'third_party/codework_shared/core'/n/'include' for n in ('core_base','core_io','core_data','core_pack')]
  cmd=list(map(str,[tools['cc'],'-O1','-Wall','-Wextra',*flags,'-MD','-MF',out/'adapter.d',*['-I'+str(x) for x in paths],source,a.frontend_archive,'-o',out/'ast-export']))
  q=subprocess.run(cmd,cwd=root,env=env,capture_output=True,text=True);(out/'build.log').write_text(q.stdout+q.stderr);report.update(argv=cmd,exit=q.returncode);require(q.returncode==0,'adapter build failed; see build.log')
  known=dict(headers);known[str(source.resolve())]=source_before;dependencies=build_dependencies(out,root,proof['source_inputs'],known)
  require(proof['archive_sha256']==sha(a.frontend_archive) and source_before==sha(source),'archive/adapter source changed during link')
  require(proof['source_inputs']==native_inputs(root),'native source changed during adapter build');frontend_receipt(a.frontend_receipt,root,a.frontend_archive)
  for tool in config['tools'].values():require(sha(tool['path'])==tool['sha256'],'build tool changed during adapter link')
  report.update(passed=True,source_inputs=proof['source_inputs'],source_sha256=source_before,frontend_sha256=sha(a.frontend_archive),frontend_receipt_sha256=sha(a.frontend_receipt),adapter=str(out/'ast-export'),adapter_sha256=sha(out/'ast-export'),build_configuration=config,build_inputs_sha256=hashlib.sha256(json.dumps(proof['build_inputs'],sort_keys=True).encode()).hexdigest(),build_input_count=len(proof['build_inputs']),adapter_build_inputs=dependencies)
 except (OSError,ValueError,KeyError,subprocess.SubprocessError) as e:report['diagnostic']=type(e).__name__+': '+str(e)
 (out/'result.json').write_text(json.dumps(report,indent=2)+'\n');return 0 if report['passed'] else 1
if __name__=='__main__':raise SystemExit(main())
