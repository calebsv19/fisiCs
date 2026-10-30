#!/usr/bin/env python3
"""Finite operational refusals for source/build/export and default-off boundaries."""
import argparse,json,subprocess,sys,copy,shutil,os
from pathlib import Path
from unittest.mock import patch
import provenance,cli
from provenance import adapter_receipt,frontend_receipt,build_dependencies,build_environment,sha
from prepare_inputs import materialize
from flow_engine import Engine,Refusal
from envelope import verify_envelope

def main():
 p=argparse.ArgumentParser(description=__doc__)
 for n in ('adapter','adapter-receipt','compiler-root','core-root','readset','frontend-build','compiler','out'):p.add_argument('--'+n,type=Path,required=True)
 a=p.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);rows=[];here=Path(__file__).parent.resolve();root=a.compiler_root.resolve();core=a.core_root.resolve();adapter=a.adapter.resolve()
 def record(cid,passed,diagnostic,stage):
  rows.append(dict(id=cid,passed=passed,diagnostic=str(diagnostic),stage=stage));(out/'results.json').write_text(json.dumps(dict(passed=all(r['passed'] for r in rows),cases=rows),indent=2)+'\n');print(cid,passed,flush=True)
 def refusal(cid,call,needle,stage='provenance'):
  try:call();record(cid,False,'unexpected acceptance',stage)
  except (OSError,ValueError,KeyError,Refusal) as err:record(cid,needle in str(err),err,stage)
 def args(target,readset=None):return ['--adapter',str(adapter),'--adapter-receipt',str(a.adapter_receipt),'--compiler-root',str(root),'--core-root',str(core),'--readset',str(readset or a.readset),'--admit-planar-si-exclusive-owner','--out',str(target)]
 def cli_failure(cid,readset,expected_stage):
  target=out/cid;cmd=[sys.executable,'-B',str(here/'cli.py'),*args(target,readset)];q=subprocess.run(cmd,capture_output=True,text=True);doc=json.loads((target/'report.json').read_text());record(cid,q.returncode!=0 and doc.get('accepted') is False and doc['stage']==expected_stage,doc.get('diagnostic'),expected_stage)
 cli_failure('input.missing',out/'absent.json','input')
 bad=out/'malformed.json';bad.write_text('{bad');cli_failure('input.malformed',bad,'input')

 for label,value in [('list',[]),('null',None),('string','wrong')]:
  receipt=out/('receipt-'+label+'.json');receipt.write_text(json.dumps(value));target=out/('receipt.cli-'+label);argv=args(target);argv[argv.index('--adapter-receipt')+1]=str(receipt)
  q=subprocess.run([sys.executable,'-B',str(here/'cli.py'),*argv],capture_output=True,text=True);doc=json.loads((target/'report.json').read_text());record('receipt.cli-'+label,q.returncode!=0 and doc.get('accepted') is False and doc.get('stage')=='provenance' and 'JSON object' in doc.get('diagnostic','') and 'Traceback' not in q.stderr,doc.get('diagnostic'),'provenance')
  refusal('receipt.frontend-'+label,lambda:frontend_receipt(receipt,root,a.frontend_build/'libfrontend.a'),'JSON object')
  target=out/('receipt.adapter-builder-'+label);cmd=[sys.executable,'-B',str(here/'build_adapter.py'),'--compiler-root',str(root),'--frontend-archive',str(a.frontend_build/'libfrontend.a'),'--frontend-receipt',str(receipt),'--out',str(target)];q=subprocess.run(cmd,capture_output=True,text=True);doc=json.loads((target/'result.json').read_text());record('receipt.adapter-builder-'+label,q.returncode!=0 and doc.get('passed') is False and 'JSON object' in doc.get('diagnostic','') and 'Traceback' not in q.stderr,doc.get('diagnostic'),'provenance')
 for label,value in [('object',{}),('null',None),('string','wrong'),('relative',['relative.h'])]:
  bad=out/('readset-'+label+'.json');bad.write_text(json.dumps(value));cli_failure('input.shape-'+label,bad,'input')
 for label,value in [('list',[]),('functions',dict(schema='fisiCs.internal.resolved_ast.v1',functions=[None]))]:
  refusal('export.shape-'+label,lambda:cli.merge_export({},value,core),'object','export')
 # Exercise the real compiler's dependency output, including an omitted header.
 deps=out/'dependency-fixture';deps.mkdir();header=deps/'external.h';header.write_text('#define VALUE 1\n');source=deps/'fixture.c';source.write_text('#include "external.h"\nint fixture = VALUE;\n');depfile=deps/'fixture.d';build_doc=json.loads((a.frontend_build/'result.json').read_text());config=build_doc['build_configuration'];cmd=[config['tools']['cc']['path'],'-MD','-MF',str(depfile),'-c',str(source),'-o',str(deps/'fixture.o')];q=subprocess.run(cmd,env=config['environment'],capture_output=True,text=True);record('build.dependency-compile',q.returncode==0,q.stdout+q.stderr,'build')
 known={str(source):sha(source)}
 refusal('build.unrecorded-dependency',lambda:build_dependencies(deps,root,{},known),'unrecorded build dependency','build')
 known[str(header)]=sha(header);accepted=build_dependencies(deps,root,{},known);record('build.dependency-positive',str(header) in accepted,accepted,'build')
 header.write_text('#define VALUE 2\n');refusal('build.dependency-drift',lambda:build_dependencies(deps,root,{},known),'changed during compilation','build')
 required=['CORE_'+n.upper()+'_DIR=third_party/codework_shared/core/core_'+n for n in ('base','io','data','pack')]
 env=config['environment'];actual=build_doc['build_inputs'];internal=str((root/'third_party/codework_shared/core/core_base/src/core_base.c').resolve());record('build.source-override-isolated',set(env)=={'PATH','LC_ALL','LANG','TMPDIR','SDKROOT'} and all(v in build_doc['argv'] for v in required) and internal in actual and not any(os.environ.get('CORE_BASE_DIR','__no_override__')+'/' in k for k in actual),'fixed roots, allowlisted environment and real vendored dependency','build')
 record('build.archive-tool-selection',str(Path(shutil.which('ar',path=env['PATH'])).resolve())==config['tools']['ar']['path'],'recorded archive tool matches actual recipe PATH','build')
 alias=out/'alias-archive-tool';alias.mkdir();(alias/'ar').symlink_to('/usr/bin/true');alias_out=out/'alias-build';alias_out.mkdir()
 with patch.dict(os.environ,{'PATH':str(alias)+os.pathsep+os.environ['PATH']}):refusal('build.archive-alias-refusal',lambda:build_environment(alias_out),'archive tool differs','build')
 for kind,path in [('frontend',a.frontend_build/'result.json'),('adapter',a.adapter_receipt)]:
  doc=json.loads(path.read_text());doc['schema']=doc['schema'].replace('.v2','.v1');receipt=out/('legacy-'+kind+'.json');receipt.write_text(json.dumps(doc));refusal('receipt.legacy-'+kind,lambda:frontend_receipt(receipt,root,a.frontend_build/'libfrontend.a') if kind=='frontend' else adapter_receipt(receipt,root,adapter),'schema/status mismatch')

 occupied=out/'output.exists';occupied.mkdir();sentinel=occupied/'report.json';sentinel.write_text('existing-owner-output');q=subprocess.run([sys.executable,'-B',str(here/'cli.py'),*args(occupied)],capture_output=True,text=True);record('output.exists',q.returncode!=0 and sentinel.read_text()=='existing-owner-output',q.stderr,'output')
 # Deliberate mutations touch only copied test inputs, never caller source/builds.
 review=json.loads((here/'reviewed_inputs.json').read_text());input_copy=out/'input-source'
 for row in review['core']:
  rel=row['ref'].split('/',1)[1];src=core.parent/rel;dst=input_copy/rel;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(src,dst)
 victim=input_copy/'core_structural/src/core_structural_frame_inertia.c';victim.write_bytes(victim.read_bytes()+b'\n');refusal('input.unreviewed-dependency',lambda:materialize(root,input_copy/'core_structural',out/'bad-inputs'),'unreviewed input/dependency','input')
 native_doc=json.loads(a.adapter_receipt.read_text());native_copy=out/'native-source'
 for rel in native_doc['source_inputs']:
  src=root/rel;dst=native_copy/rel;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(src,dst)
 victim=native_copy/'src/Compiler/pipeline.h';victim.write_bytes(victim.read_bytes()+b'\n');native_doc['source_root']=str(native_copy);test_receipt=out/'native-drift-receipt.json';test_receipt.write_text(json.dumps(native_doc));refusal('native.source-drift',lambda:adapter_receipt(test_receipt,native_copy,adapter),'native source changed')
 refusal('native.root-mismatch',lambda:adapter_receipt(a.adapter_receipt,root.parent,adapter),'source root mismatch')
 build=a.frontend_build;archive=build/'libfrontend.a'
 refusal('native.archive-mismatch',lambda:frontend_receipt(build/'result.json',root,out/'other.a'),'archive path mismatch')
 archive_copy=out/'archive-case.a';shutil.copyfile(archive,archive_copy);archive_copy.write_bytes(archive_copy.read_bytes()+b'not-the-built-archive');doc=json.loads((build/'result.json').read_text());doc['archive']=str(archive_copy);receipt=out/'archive-drift-receipt.json';receipt.write_text(json.dumps(doc));refusal('native.archive-drift',lambda:frontend_receipt(receipt,root,archive_copy),'archive changed')
 adapter_copy=out/'adapter-case';shutil.copyfile(adapter,adapter_copy);adapter_copy.write_bytes(adapter_copy.read_bytes()+b'not-the-built-adapter');doc=json.loads(a.adapter_receipt.read_text());doc['adapter']=str(adapter_copy);receipt=out/'adapter-drift-receipt.json';receipt.write_text(json.dumps(doc));refusal('adapter.binary-drift',lambda:adapter_receipt(receipt,root,adapter_copy),'adapter binary changed')
 copied=out/'adapter-source';copied.mkdir();shutil.copyfile(here/'ast_export.c',copied/'ast_export.c');(copied/'ast_export.c').write_text((copied/'ast_export.c').read_text()+'\n')
 with patch.object(provenance,'__file__',str(copied/'provenance.py')):refusal('adapter.source-drift',lambda:adapter_receipt(a.adapter_receipt,root,adapter),'adapter source changed')
 doc=json.loads(a.adapter_receipt.read_text());doc['schema']='unknown';receipt=out/'bad-receipt.json';receipt.write_text(json.dumps(doc));refusal('adapter.schema-mismatch',lambda:adapter_receipt(receipt,root,adapter),'schema/status mismatch')
 refusal('export.schema-mismatch',lambda:cli.merge_export({},dict(schema='unknown'),core),'schema mismatch','export')
 fn=dict(name='bound',file=str(core/'src/example.c'),body=dict(tag='block',body=[]));functions={};cli.merge_export(functions,dict(schema='fisiCs.internal.resolved_ast.v1',functions=[fn]),core);changed=copy.deepcopy(fn);changed['body']['body']=[dict(tag='return')]
 refusal('export.binding-conflict',lambda:cli.merge_export(functions,dict(schema='fisiCs.internal.resolved_ast.v1',functions=[changed]),core),'ambiguous cross-TU','export')
 actual_run=subprocess.run;exports=0
 def fail_second_export(cmd,*pos,**kw):
  nonlocal exports
  if str(cmd[0])==str(adapter):
   exports+=1
   if exports==2:return subprocess.CompletedProcess(cmd,1,'','intentional second-export failure')
  return actual_run(cmd,*pos,**kw)
 partial=out/'export.partial-failure'
 with patch.object(sys,'argv',['cli.py',*args(partial)]),patch.object(subprocess,'run',side_effect=fail_second_export):code=cli.main()
 report=json.loads((partial/'report.json').read_text());record('export.partial-failure',code!=0 and report.get('accepted') is False and report['stage']=='export' and (partial/'core_structural_frame_inertia.json').is_file(),report.get('diagnostic'),'export')
 base={}
 for file in sorted((out.parent/'report').glob('*.json')):
  if file.name=='report.json' or file.name.endswith('.deps.json'):continue
  for fn in json.loads(file.read_text()).get('functions',[]):base[fn['name']]=fn
 # The reviewed return boundary is part of the actual surrounding source envelope.
 changed=copy.deepcopy(base);changed['integrate']['return_type']['long_double']=True
 refusal('owner.envelope-drift',lambda:verify_envelope(Engine(changed)),'envelope return type changed','qualifier')
 engine=Engine({});engine.steps=2000000;refusal('analysis.resource-limit',engine.tick,'resource limit','qualifier')
 plain=out/'ordinary.c';plain.write_text('int main(void){return 6*7==42?0:1;}\n');obj=out/'ordinary.o';exe=out/'ordinary';before=set(out.rglob('*'));commands=[[str(a.compiler),'-std=c11','-c',str(plain),'-o',str(obj)],['cc',str(obj),'-o',str(exe)],[str(exe)]];statuses=[]
 for cmd in commands:
  q=subprocess.run(cmd,cwd=root,capture_output=True,text=True);statuses.append(dict(argv=cmd,exit=q.returncode));
  if q.returncode:break
 new=set(out.rglob('*'))-before;record('default.off',len(statuses)==3 and all(r['exit']==0 for r in statuses) and not any(p.name in ('report.json','ast-export') or p.name.endswith('.deps.json') for p in new),statuses,'native')
 result=dict(passed=len(rows)==41 and all(r['passed'] for r in rows),cases=rows,default_commands=statuses,scope='Real input/output/default commands; local build-artifact/source drift; parsed AST identity and finite resource/envelope controls. Partial-export failure is injected after one actual export.');(out/'results.json').write_text(json.dumps(result,indent=2)+'\n');return not result['passed']
if __name__=='__main__':raise SystemExit(main())
