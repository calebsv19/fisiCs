#!/usr/bin/env python3
"""Opt-in final fixed-resistor/Thermal acceptance; explicit owners and tools."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import resource
import shutil
import statistics
import subprocess
import sys
from electrothermal_binding import encoded, load, require, sha, verify_receipt
from thermal_owner_numerics import qualify as numerics, TESTS
from electrothermal_acceptance_consumers import qualify as consumers
from electrothermal_acceptance_cost import qualify as cost

ROOT=Path(__file__).resolve().parents[2]
SCHEMA='fisics.electrothermal_final_acceptance.v1'
VARIANTS=['original-clang','original-fisics','adopted-clang','portable-clang','units','physical']


def inputs(a):
    paths=[]
    names=subprocess.check_output(['git','-C',str(ROOT),'ls-files','--cached','--others','--exclude-standard','--','src','include','runtime','tests','scripts','examples'],text=True).splitlines()
    paths += [ROOT/n for n in names if (ROOT/n).is_file()]
    paths += [ROOT/'makefile',ROOT/'VERSION',a.compiler,a.baseline_bin,a.legacy_native]
    for part in ('core_base','core_io','core_data','core_pack'):
        paths += [p for p in (ROOT/'third_party/codework_shared/core'/part).rglob('*') if p.is_file() and p.suffix in ('.c','.h')]
    for base in [a.core_root,a.baseline_core,a.core_root.parents[1]/'core/core_sim']:
        for folder in ('src','include','physics','tests'):
            paths += [p for p in (base/folder).rglob('*') if p.is_file() and p.suffix in ('.c','.h','.json','.py')]
        if (base/'VERSION').exists():paths.append(base/'VERSION')
    for base in [a.host_root,a.esp_root/'simulation/scenarios/ina219_system_v1']:
        for folder in (['src','include','tools','scripts','tests','config','examples'] if base==a.host_root else ['.']):
            paths += [p for p in (base/folder).rglob('*') if p.is_file() and '__pycache__' not in p.parts and p.suffix in ('.c','.h','.py','.json','.rod')]
    paths += [a.host_root/'Makefile',a.host_root/'VERSION']
    for folder in ('include','src/tools'):
        paths += list((a.physics_root/folder).rglob('*.h'))
    paths += [a.physics_root/'src/tools/physics_sim_heated_face.c']+[a.physics_root/f'src/app/{n}.c' for n in ['cfd_open_atmosphere3d','cfd_open_atmosphere3d_box','cfd_open_atmosphere3d_box_preconditioner','cfd_open_atmosphere3d_lateral','cfd_passive3d','cfd_cartesian3d','cfd_sparse_mg','cfd_memory']]
    return {str(p.resolve()):sha(p) for p in sorted(set(paths))}


def verify_cost(value,a):
    limits=load(ROOT/'tests/performance/solver_overlay_budget.json')
    require(value['passed'] is True and value['budget']==limits and value['budget_sha256']==sha(ROOT/'tests/performance/solver_overlay_budget.json'),'cost budget changed')
    require(value['compiler_sha256']==sha(a.compiler) and len(value['samples'])==240 and len(value['summary'])==24,'cost tool/denominator')
    require(len(value['prepared_inputs'])==5 and all(x['numerical_source_identical_outside_attributes'] for x in value['prepared_inputs']),'matched cost inputs')
    for row in value['summary']:
        require(row['accepted'] is True,'cost refusal')
        if row['kind']=='compile':
            groups={mode:[x for x in value['samples'] if not x['warmup'] and x['label'].startswith(row['id']+'-') and x['label'].endswith('-'+mode)] for mode in [row['baseline'],row['overlay']]}
            require(all(len(x)==9 for x in groups.values()),'cost sample count')
            baseline,overlay=[groups[m] for m in [row['baseline'],row['overlay']]]
            aw,bw=[statistics.median(x['seconds'] for x in group) for group in [baseline,overlay]]
            ar,br=[statistics.median(x['peak_rss_bytes'] for x in group) for group in [baseline,overlay]]
            require((aw,bw,ar,br)==tuple(row[k] for k in ['base_median_seconds','overlay_median_seconds','base_median_rss_bytes','overlay_median_rss_bytes']),'cost median mismatch')
            require(bw<=limits['wall_ratio']*aw+limits['wall_allowance_seconds'] and br<=limits['rss_ratio']*ar+limits['rss_allowance_bytes'],'compile cost exceeded')
        else:
            group=[x for x in value['samples'] if not x['warmup'] and x['label'].startswith(row['id']+'-strict-')]
            require(len(group)==9 and row['selected_body_accepted'] is True and row['selection_counts']['blocked']==0,'strict cost selection/count')
            require(row['median_seconds']==statistics.median(x['seconds'] for x in group) and row['median_rss_bytes']==statistics.median(x['peak_rss_bytes'] for x in group),'strict cost median mismatch')
            require(max(x['seconds'] for x in group)==row['max_seconds']<=limits['strict_seconds'] and max(x['peak_rss_bytes'] for x in group)==row['max_rss_bytes']<=limits['strict_rss_bytes'],'strict cost exceeded')


def verify(path,a):
    path=Path(path);a.out=path.parent;v=load(path)
    require(v['schema']==SCHEMA and v['passed'] is True and v['whole_owner_strict_claim'] is False,'final acceptance scope')
    require(v['inputs']==inputs(a),'current final source/tool pins differ')
    for name,digest in v['artifacts'].items():
        p=(a.out/name).resolve();require(p.is_relative_to(a.out.resolve()) and sha(p)==digest,'final artifact drift: '+name)
    required={'boundary/results.json','numerical.json','consumers.json','cost/results.json','broad.json','tools/thermal_assembly_agent','tools/thermal_semantic_identity.json'}
    require(required<=set(v['artifacts']),'missing final evidence')
    require(v['gates']=={name:sha(a.out/file) for name,file in [('boundary','boundary/results.json'),('numerical','numerical.json'),('consumers','consumers.json'),('cost','cost/results.json'),('broad','broad.json')]},'final gate joins')
    numerical=load(a.out/'numerical.json');require(numerical['passed'] and numerical['variants']==VARIANTS and len(set(numerical['layout'].values()))==1 and numerical['exact_test_output_parity'] is True,'numerical/layout parity')
    require(len(numerical['numerical_mutations'])==7,'numerical failure controls')
    consumer=load(a.out/'consumers.json');require(consumer['passed'] and consumer['variants']==['native','units','physical'] and consumer['reverse_old_header_drivers']==[t[0] for t in TESTS],'ABI/consumer coverage')
    a.native=a.out/'tools/thermal_assembly_agent';a.native_identity=a.out/'tools/thermal_semantic_identity.json';a.binding=ROOT/'tests/fixtures/physical_semantics/electrothermal/fixed_resistor_boundary.json'
    sys.path[:0]=[str(a.host_root/'tools'),str(a.esp_root/'simulation/scenarios/ina219_system_v1')]
    import electrical_heat as heat
    boundary=verify_receipt(a.out/'boundary/results.json',a,heat)
    require(boundary['native_sha256']==consumer['native_sha256']==sha(a.native) and boundary['native_engine']==consumer['engine'],'native boundary/consumer join')
    broad=load(a.out/'broad.json');require(broad['passed'] and broad['compiler_sha256']==sha(a.compiler) and broad['targets']==['test','frontend-contract-test'],'broad/frontend scope')
    require(broad['exit']==0 and broad['frontend_pass_marker'] is True,'broad/frontend failure')
    verify_cost(load(a.out/'cost/results.json'),a)
    require(all(row['exit'] in ([1,-6] if row['expected']=='failure' else [row['expected']]) for row in v['commands']),'child command failure')
    return v


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ['compiler','core-root','host-root','esp-root','physics-root','baseline-core','baseline-bin','legacy-native']:
        parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--out',type=Path);parser.add_argument('--scratch',type=Path)
    parser.add_argument('--frontend-build-root',type=Path,help='Optional compatible task-owned build; default fresh scratch/compiler')
    parser.add_argument('--verify-receipt',type=Path)
    a=parser.parse_args()
    for name,p in vars(a).items():
        if isinstance(p,Path):setattr(a,name,p.resolve())
    require(sys.flags.optimize==0,'qualification requires assertions enabled')
    if a.verify_receipt:
        verify(a.verify_receipt,a);print('PASS final electrothermal receipt readback');return
    require(a.out is not None and a.scratch is not None,'fresh --out and --scratch required')
    require(not a.out.exists() and not a.scratch.exists() and not a.out.is_relative_to(a.scratch) and not a.scratch.is_relative_to(a.out),'fresh separate evidence and scratch roots required')
    pins=inputs(a);a.out.mkdir(parents=True);a.scratch.mkdir(parents=True)
    (a.out/'logs').mkdir();(a.out/'tools').mkdir();(a.scratch/'tmp').mkdir()
    commands=[];outputs={};env={**os.environ,'PYTHONDONTWRITEBYTECODE':'1','FISICS_MAX_PROCS':'0','TMPDIR':str(a.scratch/'tmp')}
    for key in ['FISICS_OVERLAY','DISABLE_CODEGEN']:env.pop(key,None)
    def no_core():resource.setrlimit(resource.RLIMIT_CORE,(0,0))
    def run(name,args,expected=0,cwd=None,timeout=600):
        child=env.copy();cwd=cwd or a.host_root
        if Path(str(args[0])).name=='make':
            for key in ['MAKEFLAGS','MAKEOVERRIDES','MFLAGS','MAKELEVEL','BIN','BUILD_DIR','LIB_FRONTEND','EXAMPLES_BUILD_DIR','CC','CFLAGS','CPPFLAGS','LDFLAGS','LDLIBS']:child.pop(key,None)
        job=subprocess.run(list(map(str,args)),cwd=cwd,env=child,capture_output=True,text=True,timeout=timeout,preexec_fn=no_core)
        text=job.stdout+job.stderr
        with gzip.open(a.out/'logs'/(name+'.log.gz'),'wt') as f:f.write(text)
        commands.append(dict(id=name,command=list(map(str,args)),cwd=str(cwd),exit=job.returncode,expected=expected))
        require(job.returncode in ([1,-6] if expected=='failure' else [expected]),name+': status '+str(job.returncode)+' '+text[-1800:])
        outputs[name]=text;return job.stdout
    try:
        numerical=numerics(a,a.scratch/'numerics',run)
        for test,_ in TESTS:
            require(len({outputs[v+'-'+test+'-run'] for v in VARIANTS})==1,'six-variant output parity: '+test)
        numerical['exact_test_output_parity']=True
        consumer=consumers(a,a.scratch/'consumers',run,numerical)
        numerical.pop('objects')  # Rerunnable scratch objects are not retained evidence.
        (a.out/'numerical.json').write_bytes(encoded(numerical));(a.out/'consumers.json').write_bytes(encoded(consumer))
        boundary_args=[sys.executable,'-B',ROOT/'tests/integration/run_electrothermal_boundary_contracts.py']
        for name in ['compiler','core_root','host_root','esp_root','physics_root','native','native_identity','legacy_native']:
            boundary_args += ['--'+name.replace('_','-'),getattr(a,name)]
        run('boundary',boundary_args+['--out',a.out/'boundary'],cwd=ROOT)
        build=a.frontend_build_root or a.scratch/'compiler';build.mkdir(parents=True,exist_ok=True)
        archive=build/'libfisics_frontend.a';relative=os.path.relpath(a.compiler,ROOT)
        # Keep the selected compiler read-only; some old recipes prepend './'.
        # All frontend objects, tests and example outputs remain task-owned.
        overrides=a.scratch/'regression.mk'
        overrides.write_text('integration-external-evidence-receipts:\n\t@python3 ./tests/integration/test_external_evidence_receipts.py\n\t@python3 ./scripts/validate_external_evidence.py --evidence-root ./tests/fixtures/external_evidence_receipts/v1 --observation receipts/observation.json --output '+str(a.scratch/'external-receipt.json')+'\n'+
            'integration-examples-semantic-metadata: $(BIN)\n\t@bash ./examples/semantic_metadata/run_basic.sh "$(abspath $(BIN))" '+str(a.scratch/'semantic-basic')+'\n\t@bash ./examples/semantic_metadata/compare_compilers.sh "$(abspath $(BIN))" '+str(a.scratch/'semantic-portable')+'\n\t@bash ./examples/semantic_metadata/run_hardware_project.sh "$(abspath $(BIN))" '+str(a.scratch/'semantic-hardware')+'\n'+
            'examples-semantic-hardware: $(BIN)\n\t@bash ./examples/semantic_metadata/run_hardware_project.sh "$(abspath $(BIN))" '+'$(EXAMPLES_BUILD_DIR)/semantic_metadata/hardware_project'+'\n')
        text=run('broad-frontend',['make','-j4','-f','makefile','-f',overrides,'-o',relative,'BIN='+relative,'BUILD_DIR='+str(build),'LIB_FRONTEND='+str(archive),'EXAMPLES_BUILD_DIR='+str(a.scratch/'examples'),'test','frontend-contract-test'],cwd=ROOT,timeout=1800)
        require('Frontend contract bucket: PASS' in text,'frontend pass marker missing')
        (a.out/'broad.json').write_bytes(encoded(dict(passed=True,targets=['test','frontend-contract-test'],exit=0,compiler_sha256=sha(a.compiler),frontend_sha256=sha(archive),frontend_pass_marker=True)))
        (a.out/'regression.mk').write_bytes(overrides.read_bytes())
        # No child compiles or test workers from this gate overlap measurements.
        value=cost(a,a.scratch/'cost',run);verify_cost(value,a)
        dest=a.out/'cost';dest.mkdir()
        for p in (a.scratch/'cost').iterdir():
            if not p.is_file():continue
            if p.name=='results.json':shutil.copy2(p,dest/p.name)
            else:
                with gzip.open(dest/(p.name+'.gz'),'wb') as f:f.write(p.read_bytes())
        require(inputs(a)==pins,'source drift through final acceptance')
        artifacts={str(p.relative_to(a.out)):sha(p) for p in a.out.rglob('*') if p.is_file() and p.suffix!='.o' and 'tmp' not in p.relative_to(a.out).parts}
        result=dict(schema=SCHEMA,passed=True,whole_owner_strict_claim=False,inputs=pins,artifacts=artifacts,commands=commands,
                    gates={name:sha(a.out/file) for name,file in [('boundary','boundary/results.json'),('numerical','numerical.json'),('consumers','consumers.json'),('cost','cost/results.json'),('broad','broad.json')]})
        candidate=a.out/'candidate.json';candidate.write_bytes(encoded(result));verify(candidate,a);os.replace(candidate,a.out/'results.json')
        print(json.dumps(dict(passed=True,commands=len(commands),boundary_commands=len(load(a.out/'boundary/results.json')['commands']),numerical_variants=6,cost_summaries=24,cost_invocations=240)))
    except BaseException as error:
        (a.out/'failure.json').write_bytes(encoded(dict(passed=False,error=str(error),commands=commands)));raise


if __name__=='__main__':
    try:main()
    except (ValueError,OSError,KeyError,AssertionError,subprocess.SubprocessError) as error:
        sys.exit('final electrothermal acceptance refused: '+str(error))
