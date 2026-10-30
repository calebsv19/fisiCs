#!/usr/bin/env python3
"""Verify reviewed source/dependency hashes; materialize a contained core snapshot."""
import argparse,json,subprocess,shutil
from pathlib import Path
from provenance import sha,require

def materialize(compiler,core,out):
 compiler=Path(compiler).resolve();core=Path(core).resolve();out=Path(out).resolve();doc=json.loads(Path(__file__).with_name('reviewed_inputs.json').read_text());require(doc['schema']=='fisiCs.experimental.matrix_flow_inputs.v1','input manifest schema mismatch')
 sdk=Path(subprocess.check_output(['xcrun','--show-sdk-path'],text=True).strip())/'usr/include';resource=Path(subprocess.check_output(['clang','-print-resource-dir'],text=True).strip())/'include'
 roots=dict(core=core.parent,compiler=compiler,sdk=sdk,resource=resource)
 def resolve(ref,selected):
  anchor,rel=ref.split('/',1);require(anchor in selected and not Path(rel).is_absolute() and '..' not in Path(rel).parts,'invalid reviewed input reference');return selected[anchor]/rel
 # Verify before creating/copying anything. New bytes cannot authorize themselves.
 for row in doc['core']+doc['readset']:require(sha(resolve(row['ref'],roots))==row['sha256'],'unreviewed input/dependency: '+row['ref'])
 out.mkdir(parents=True,exist_ok=False)
 for row in doc['core']:
  src=resolve(row['ref'],roots);dst=out/row['ref'];dst.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(src,dst);require(sha(dst)==row['sha256'] and sha(src)==row['sha256'],'input changed during snapshot: '+row['ref'])
 roots['core']=out/'core';readset=[str(resolve(row['ref'],roots)) for row in doc['readset']]
 (out/'readset.json').write_text(json.dumps(readset,indent=2)+'\n');(out/'result.json').write_text(json.dumps(dict(passed=True,core_files=len(doc['core']),reviewed_dependencies=len(readset),manifest_sha256=sha(Path(__file__).with_name('reviewed_inputs.json')),source_root=str(core),snapshot=str(out/'core/core_structural')),indent=2)+'\n')
 return out/'core/core_structural',out/'readset.json'
def main():
 p=argparse.ArgumentParser(description=__doc__)
 for n in ('compiler-root','core-root','out'):p.add_argument('--'+n,type=Path,required=True)
 a=p.parse_args()
 try:materialize(a.compiler_root,a.core_root,a.out);return 0
 except (OSError,ValueError,KeyError,subprocess.SubprocessError) as e:print(type(e).__name__+': '+str(e));return 1
if __name__=='__main__':raise SystemExit(main())
