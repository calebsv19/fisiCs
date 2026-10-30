#!/usr/bin/env python3
"""P7.7 actual thermal adoption acceptance; explicit owners and frozen baseline."""
import argparse,hashlib,json,os,resource,subprocess,shutil
from pathlib import Path
from thermal_owner_contracts import qualify as contracts
from thermal_owner_numerics import qualify as numerics
from thermal_owner_receiver import qualify as receiver
from thermal_owner_cost import qualify as cost

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ['compiler','core-root','host-root','baseline-core','baseline-bin','out']:p.add_argument('--'+name,type=Path,required=True)
    p.add_argument('--physics-root',type=Path,required=True);p.add_argument('--probe',type=Path)
    p.add_argument('--skip-host',action='store_true',help='Diagnostic partial run; never full acceptance')
    a=p.parse_args()
    for name,value in vars(a).items():
        if isinstance(value,Path):setattr(a,name,value.resolve())
    a.out.mkdir(parents=True,exist_ok=False);commands=[];result=dict(passed=False,commands=commands,compiler_sha256=sha(a.compiler),whole_owner_strict_claim=False)
    env={**os.environ,'FISICS_MAX_PROCS':'0','PYTHONDONTWRITEBYTECODE':'1','TMPDIR':str(a.out/'tmp')};env.pop('FISICS_OVERLAY',None);(a.out/'tmp').mkdir()
    pins={str(f):sha(f) for base in [a.core_root/'src',a.core_root/'include',a.core_root/'physics'] for f in base.rglob('*') if f.is_file()}
    pins.update({str(a.host_root/s):sha(a.host_root/s) for s in ['src/experiment/participant.c','src/assembly/cycle.c','src/coupling/receiver.c','scripts/engine_identity.py','config/semantic_engine_baseline.json','tools/electrical_heat.py','tools/electrical_identity.py']})
    result['inputs']=pins
    def no_core():resource.setrlimit(resource.RLIMIT_CORE,(0,0))
    def run(name,args,expected=0,cwd=None):
        cmd=list(map(str,args));child_env=env.copy()
        if Path(cmd[0]).name=='make':
            # Parent compiler Make overrides must never select the host BIN or
            # leak compiler/linker flags into another owner's build.
            for key in ['MAKEFLAGS','MAKEOVERRIDES','MFLAGS','MAKELEVEL','BIN','BUILD_DIR','LIB_FRONTEND','EXAMPLES_BUILD_DIR','CC','CFLAGS','CPPFLAGS','LDFLAGS','LDLIBS']:
                child_env.pop(key,None)
        job=subprocess.run(cmd,cwd=cwd or a.host_root,env=child_env,capture_output=True,text=True,timeout=300,preexec_fn=no_core)
        (a.out/(name+'.log')).write_text(job.stdout+job.stderr);commands.append(dict(id=name,command=cmd,exit=job.returncode,expected=expected,cwd=str(cwd or a.host_root)))
        assert (job.returncode in [1,-6] if expected=='failure' else job.returncode==expected),(name,job.returncode,(job.stdout+job.stderr)[-2500:])
        assert expected or 'Error:' not in job.stdout+job.stderr,(name,(job.stdout+job.stderr)[-1500:])
        return job.stdout
    try:
        result['numerics']=numerics(a,a.out/'numerics',run)
        result['contracts']=contracts(a,a.out/'contracts',run)
        if a.probe:
            pol=json.loads((a.core_root/'physics/thermal_policy.json').read_text());pol['catalog_paths']=[str(a.core_root/'physics/thermal_catalog.json')]
            bad=json.loads(json.dumps(pol));bad['operation_rules'].append(dict(bad['operation_rules'][0],id='rule.ambiguous'))
            dest=a.out/'bad-policy.json';dest.write_text(json.dumps(bad))
            # Native driver requires a v1 starting model and a good/bad v7 pair.
            v1=json.loads(json.dumps(pol));v1={k:v for k,v in v1.items() if k in ['schema','schema_version','policy_id','catalog_paths','linear_kinds','affine_pairs','origins']};v1['schema_version']=1
            old=a.out/'v1-policy.json';old.write_text(json.dumps(v1))
            result['allocation']=json.loads(run('native-policy',[a.probe,old,a.core_root/'physics/thermal_policy.json',dest]))
        if not a.skip_host:
            build=a.out/'host';shared=a.core_root.parents[1]
            run('host-build-test',['make','-j4','BUILD_DIR='+str(build),'SHARED_ROOT='+str(shared),'build','test','test-cycles','test-external-heat'],cwd=a.host_root)
            assert sha(a.compiler)==result['compiler_sha256'],'host build changed compiler'
            new=build/'bin/thermal_sim';old=a.baseline_bin
            # Old states remain readable by their exact retained helper; the new
            # source identity refuses them byte-preservingly, with no relabeling.
            legacy=a.out/'legacy';model=a.out/'model.rod';model.write_text((a.host_root/'tests/fixtures/material_contact.rod').read_text().replace('2 1 1 100','2 1 0.1 20'))
            run('legacy-create',[old,'--experiment-create',model,legacy,901,1,10]);run('legacy-seven',[old,'--experiment-run',legacy,7]);checkpoint=(legacy/'checkpoint.txt').read_bytes()
            run('new-refuses-old',[new,'--experiment-inspect',legacy],expected=2);assert (legacy/'checkpoint.txt').read_bytes()==checkpoint
            run('old-replay-inspect',[old,'--experiment-inspect',legacy]);run('old-replay-complete',[old,'--experiment-run',legacy,20])
            # Actual C host objects link against unit/physical core objects.
            # These are disposable ABI/numerical probes; never retained as
            # restart engines with the Clang identity compiled in their host.
            allobjs=list((build/'obj/src').rglob('*.o'))
            app=[x for x in allobjs if '/assembly/' not in str(x) and '/workbench/' not in str(x) and '/tests/' not in str(x)]
            sim=[build/'obj'/(n+'.o') for n in ['core_sim_time','core_sim_schedule','core_sim_exchange']]
            consumers=[]
            for mode in ['units','physical']:
                exe=a.out/('consumer-'+mode);coreobjs=[Path(x) for x in result['numerics']['objects'][mode].values()]
                flags=['--overlay=physics-units','-DCORE_THERMAL_UNITS=1'] if mode=='units' else ['--overlay=physics-semantics','-DCORE_THERMAL_PHYSICAL=1','--physical-semantics-policy',a.core_root/'physics/thermal_policy.json']
                inc=['-I'+str(a.core_root/'include'),'-I'+str(shared/'core/core_sim/include'),'-I'+str(shared/'shape/external/cjson'),'-I'+str(a.host_root/'include'),'-I'+str(a.host_root/'src'),'-I'+str(Path(__file__).resolve().parents[2]/'include')]
                participant=a.out/('participant-'+mode+'.o');cycle=a.out/('cycle-'+mode+'.o')
                run('participant-'+mode+'-compile',[a.compiler,*flags,*inc,'-ffp-contract=off','-c',a.host_root/'src/experiment/participant.c','-o',participant])
                run('cycle-'+mode+'-compile',[a.compiler,*flags,*inc,'-ffp-contract=off','-c',a.host_root/'src/assembly/cycle.c','-o',cycle])
                app_without_participant=[x for x in app if x!=build/'obj/src/experiment/participant.o']
                run('consumer-'+mode+'-link',['clang',*app_without_participant,participant,*sim,*coreobjs,'-lm','-o',exe])
                assembly=a.out/('assembly-'+mode)
                assembly_objects=[x for x in allobjs if x not in [build/'obj/src/main.o',build/'obj/src/experiment/participant.o',build/'obj/src/assembly/cycle.o']]
                run('assembly-'+mode+'-link',['clang',*assembly_objects,participant,cycle,*sim,*coreobjs,build/'obj/cJSON.o','-lm','-o',assembly])
                run('assembly-'+mode+'-cycles',['python3','-B',a.host_root/'tests/test_cycles.py',assembly],cwd=a.host_root)
                run('assembly-'+mode+'-electrical',['python3','-B',a.host_root/'tests/test_external_heat.py',assembly],cwd=a.host_root)
                for script in ['test_two_body.py','test_rod.py','test_material.py']:
                    run('consumer-'+mode+'-'+script,['python3','-B',a.host_root/'tests'/script,exe],cwd=a.host_root)
                consumers.append(mode)
            result['consumer_variants']=['native',*consumers]
            result['restart_policy']='new source identity; old checkpoints refuse unchanged in new engine and replay in retained exact old helper'
        if not a.skip_host:
            result['receiver']=receiver(a,a.out/'receiver',run)
            result['cost']=cost(a,a.out/'cost',run)
        assert all(sha(k)==v for k,v in pins.items()),'source drift'
        assert sha(a.compiler)==result['compiler_sha256'],'compiler drift'
        result['passed']=not a.skip_host;result['partial_passed']=True
        print(json.dumps(dict(passed=result['passed'],partial=a.skip_host,commands=len(commands),strict_kernels=result['contracts']['strict_kernel_count'],variants=result['numerics']['variants'])))
    finally:(a.out/'results.json').write_text(json.dumps(result,indent=2)+'\n')
if __name__=='__main__':main()
