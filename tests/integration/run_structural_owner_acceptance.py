#!/usr/bin/env python3
"""P7.6 combined opt-in static structural owner qualification, with exact inputs."""
import argparse, hashlib, json, os
from pathlib import Path
import shutil, subprocess
from structural_acceptance_inventory import inventory
from structural_acceptance_interfaces import abi, consumer
ROOT=Path(__file__).resolve().parents[2]
SELECTED=[f'src/core_structural_frame_{p}.c' for p in ['element','compile','connections','assemble','loads','linear','result','recovery']]+[f'src/core_structural_frame_{p}.h' for p in ['physical','boundaries','energy']]+['physics/frame_policy.json','physics/frame_catalog.json']
MODULES=['core_structural','core_base','core_units','core_sim','core_scene_compile','core_scene','core_object','core_io']
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def hashes(root):
    return {str(p.relative_to(root)):sha(p) for p in sorted(root.rglob('*')) if p.is_file() and '__pycache__' not in p.parts and p.suffix not in ['.pyc']}
def copy_owner(src,dest):
    dest.mkdir(parents=True)
    for n in ['src','include','tests','physics']:
        if (src/n).exists():shutil.copytree(src/n,dest/n,ignore=shutil.ignore_patterns('__pycache__','*.pyc'))
    for n in ['Makefile','VERSION','README.md']:
        if (src/n).exists():shutil.copy2(src/n,dest/n)
def check(a):
    o=a.out;o.mkdir(parents=True,exist_ok=a.resume);previous=json.loads((o/'results.json').read_text()) if a.resume else None;commands=list(previous['commands']) if previous else [];result=dict(schema='fisics.structural_owner_acceptance.v1',passed=False,commands=commands,compiler_sha256=sha(a.compiler),runner_sha256=sha(Path(__file__)),whole_function_strict_claim=False)
    env={**os.environ,'FISICS_MAX_PROCS':'0','PYTHONDONTWRITEBYTECODE':'1','TMPDIR':str(o/'tmp')};env.pop('FISICS_OVERLAY',None);env.pop('DISABLE_CODEGEN',None);(o/'tmp').mkdir(exist_ok=a.resume)
    def run(label,cmd,cwd=ROOT,code=0,timeout=180):
        p=subprocess.run(list(map(str,cmd)),cwd=cwd,env=env,capture_output=True,text=True,timeout=timeout)
        (o/(label+'.log')).write_text(p.stdout+p.stderr);commands.append(dict(label=label,command=list(map(str,cmd)),cwd=str(cwd),exit=p.returncode,expected=code))
        assert p.returncode==code,(label,p.returncode,(p.stdout+p.stderr)[-2500:]);assert code or 'Error:' not in p.stdout+p.stderr,label
        return p.stdout
    def gate(label,script,args):
        if previous and label in previous.get('gates',{}):
            path=o/label/'results.json';assert sha(path)==previous['gates'][label]['receipt_sha256'],('cached receipt changed',label)
            d=json.loads(path.read_text());assert d['passed'] and d['compiler_sha256']==sha(a.compiler) and d['runner_sha256']==sha(ROOT/'tests/integration'/script)
            if label=='energy':assert sha(o/'numerical/candidate-fisics/structural.a')==d['library_sha256'], 'cached semantic archive changed'
            if label=='boundaries':assert all(sha(Path(p))==h for p,h in d['numerical_library_sha256'].items()), 'cached numerical archives changed'
            result.setdefault('gates',{})[label]=previous['gates'][label];return d
        run(label,['python3','-B',ROOT/'tests/integration'/script,'--compiler',a.compiler,*args,'--out',o/label],timeout=1800)
        d=json.loads((o/label/'results.json').read_text());assert d['passed'];result.setdefault('gates',{})[label]=dict(passed=True,receipt_sha256=sha(o/label/'results.json'),commands=len(d.get('commands',[])));return d
    try:
        # Freeze current shared/application input owners, not mutable build trees.
        shared=o/'inputs/shared';owner=shared/'core/core_structural';base=shared/'core/core_base'
        if previous:
            assert previous['compiler_sha256']==sha(a.compiler)
            initial=previous['inputs']['shared'];live_modules={n:a.shared_root/'core'/n for n in MODULES}
            for n,pins in initial.items():assert hashes(shared/'core'/n)==pins,('frozen input changed',n)
            app=o/'inputs/structural_sim';app_pins=previous['inputs']['consumer'];assert hashes(app)==app_pins
            baseline=o/'baseline';result['inputs']=previous['inputs'];result['resumed_from_runner_sha256']=previous['runner_sha256']
        else:
            initial={};live_modules={}
            for n in MODULES:
                live=a.shared_root/'core'/n;dest=shared/'core'/n;copy_owner(live,dest)
                pins=hashes(dest);assert all(sha(live/p)==h for p,h in pins.items()),('copy drift',n);initial[n]=pins;live_modules[n]=live
            shape=shared/'shape/external/cjson';shape.mkdir(parents=True)
            for n in ['cJSON.c','cJSON.h']:shutil.copy2(a.shared_root/'shape/external/cjson'/n,shape/n)
            app=o/'inputs/structural_sim';app.mkdir()
            for n in ['src','include','make','tools','tests','examples']:shutil.copytree(a.consumer_root/n,app/n,ignore=shutil.ignore_patterns('__pycache__','*.pyc'))
            for n in ['Makefile','VERSION']:shutil.copy2(a.consumer_root/n,app/n)
            app_pins=hashes(app);assert all(sha(a.consumer_root/p)==h for p,h in app_pins.items()),'consumer copy drift'
            # Preserve the prior P7.5 numerical comparator, now with all current
            # unrelated dependencies mirrored identically on both sides.
            baseline=o/'baseline';shutil.copytree(owner,baseline)
            for n in ['src/core_structural_frame_result.c','src/core_structural_frame_recovery.c']:shutil.copy2(a.p75_baseline/n,baseline/n)
            (baseline/'src/core_structural_frame_energy.h').unlink()
            for n in ['src/core_structural_frame_result.c','src/core_structural_frame_recovery.c','src/core_structural_frame_energy.h']:
                assert sha(owner/n)==sha(a.p75_reference/n),('P7.5 source changed: qualify a new baseline explicitly',n)
            result['inputs']=dict(owner=str(a.shared_root/'core/core_structural'),owner_version=(owner/'VERSION').read_text().strip(),shared=initial,consumer_root=str(a.consumer_root),consumer=app_pins,baseline_comparator=str(a.p75_baseline),baseline_comparator_sha256={n:sha(baseline/n) for n in ['src/core_structural_frame_result.c','src/core_structural_frame_recovery.c']},budget_sha256=sha(a.budget))
        include=['-I',ROOT/'include','-I',owner/'src','-I',owner/'include','-I',base/'include']
        numeric=gate('numerical','run_units_structural_source_contracts.py',['--source-root',owner,'--base-root',base,'--maintained-baseline',baseline,'--structural-policy',owner/'physics/frame_policy.json','--boundary-adoption','--energy-adoption'])
        common=['--source-root',owner,'--base-root',base]
        physical=gate('physical','run_physical_structural_contracts.py',common)
        gate('energy','run_structural_energy_contracts.py',[*common,'--numerical-output',o/'numerical'])
        gate('boundaries','run_structural_frame_boundary_contracts.py',[*common,'--numerical-output',o/'numerical'])
        for label,build in [('inventory',lambda:inventory(owner,base,o/'inventory',a.compiler,include)),('abi',lambda:abi(a.compiler,owner,base,o/'abi',run,include)),('consumer',lambda:consumer(app,shared,o/'numerical',o/'consumer',run))]:
            if previous and label in previous:
                cached=json.loads((o/label/'results.json').read_text());assert cached==previous[label] and cached['passed'],('cached stage changed',label)
                if label=='abi':assert all(sha(Path(p))==h for p,h in cached['source_sha256'].items()), 'cached ABI projection changed'
                if label=='consumer':
                    for name,identity in cached['identities'].items():
                        assert sha(o/'consumer'/name/'build_identity.json')==identity['build_identity_sha256'] and sha(o/'consumer'/name/'bin/structural_sim')==identity['binary_sha256'], 'cached consumer identity changed'
                        if identity['archive_sha256']:assert sha(o/'numerical'/name/'structural.a')==identity['archive_sha256'], 'cached consumer archive changed'
                result[label]=cached
            else:result[label]=build()
        result['native_model_memory']=previous['native_model_memory'] if previous and 'native_model_memory' in previous else json.loads(run('native-model-memory',[a.model_probe,owner/'physics/frame_policy.json',owner/'physics/frame_policy.json']))
        # Measure after all other builds/tests finish, never in contention with
        # our own compilation. Budget and nine-sample policy are unchanged.
        deps=[p for n in MODULES for p in (shared/'core'/n).rglob('*') if p.is_file() and (p.suffix in ['.h','.c','.json'] or p.name=='VERSION')]+[ROOT/'include/fisics/extensions.h']
        args=[*include,'-DCORE_STRUCTURAL_PHYSICAL_SEMANTICS=1']
        tus=[dict(id='structural-'+part,source=str(owner/f'src/core_structural_frame_{part}.c'),args=list(map(str,args))) for part in ['element','loads','recovery','result']]
        strict=[dict(id='build',source=tus[0]['source'],function='cs_physical_build',args=list(map(str,args)),physical=True,permissions=['literal-assumptions'],expected_checked=61,expected_permitted_trust=7),dict(id='apply',source=tus[2]['source'],function='cs_physical_apply',args=list(map(str,args)),physical=True,permissions=['literal-assumptions'],expected_checked=26,expected_permitted_trust=2),dict(id='recover-end',source=tus[2]['source'],function='cs_physical_recover_end',args=list(map(str,args)),physical=True,permissions=[],expected_checked=28,expected_permitted_trust=0),dict(id='elastic-sum',source=tus[3]['source'],function='cs_frame_elastic_sum',args=list(map(str,args)),physical=True,permissions=[],expected_checked=2,expected_permitted_trust=0),dict(id='member-quadrature',source=tus[2]['source'],function='cs_frame_member_energy_quadrature',args=list(map(str,include)),expected_checked=9,expected_permitted_trust=0),dict(id='publication-report',source=tus[3]['source'],function='cs_frame_publish',args=list(map(str,args)),physical=True,accepted=False)]
        manifest=o/'cost-manifest.json';manifest.write_text(json.dumps(dict(physical_policy=str(owner/'physics/frame_policy.json'),dependencies=sorted(map(str,deps)),translation_units=tus,strict_functions=strict),indent=2)+'\n')
        cost_out=a.cost_output or o/'cost'
        if not (previous and (cost_out/'results.json').exists() and json.loads((cost_out/'results.json').read_text())['passed']):
            run('cost',['python3','-B',ROOT/'tests/performance/measure_overlay_cost.py','--compiler',a.compiler,'--manifest',manifest,'--budget',a.budget,'--out',cost_out],timeout=1800)
        cost=json.loads((cost_out/'results.json').read_text());assert cost['passed'] and cost['compiler_sha256']==sha(a.compiler) and cost['budget_sha256']==sha(a.budget) and cost['manifest_sha256']==sha(manifest)
        assert all(sha(Path(p))==h for p,h in cost['source_sha256'].items());result['cost_summary']=cost['summary'];result['cost_output']=str(cost_out)
        # All selected functions have a category and proof lane. No outer/raw
        # scope obtains a strict claim from report-mode AST enumeration.
        for row in result['inventory']['functions']:
            if row['category']=='physical_kernel':
                if row['name']=='cs_frame_elastic_sum':proof=next(x['policy'] for x in json.loads((o/'energy/results.json').read_text())['cases'] if x['label']=='elastic-sum')
                else:proof=next(x['selection'] for x in physical['cases'] if x['label']==row['name'])
                assert proof['accepted'];row['physical_strict_claim']=True;row['strict_proof']=proof
        result['inventory']['counts']['physical_strict_accepted']=sum(x['physical_strict_claim'] for x in result['inventory']['functions'])
        (o/'inventory/results.json').write_text(json.dumps(result['inventory'],indent=2)+'\n')
        drifts={}
        for n,live in live_modules.items():
            for p,h in initial[n].items():
                if p=='README.md':continue
                if not (live/p).exists() or sha(live/p)!=h:drifts[n+'/'+p]='changed after freeze'
            known=set(initial[n]);added=[str(p.relative_to(live)) for part in ['src','include','tests','physics'] if (live/part).exists() for p in (live/part).rglob('*') if p.is_file() and '__pycache__' not in p.parts and str(p.relative_to(live)) not in known]
            for p in added:drifts[n+'/'+p]='added after freeze'
        result['live_shared_drift']=drifts;assert not drifts,('current shared owner drift',drifts)
        result['live_consumer_drift']=[p for p,h in app_pins.items() if not (a.consumer_root/p).exists() or sha(a.consumer_root/p)!=h]
        assert not result['live_consumer_drift'],'current consumer source drift'
        assert sha(a.compiler)==result['compiler_sha256'];result['passed']=True
        print('Structural owner P7.6: PASS; combined numerical/strict/mutation/inventory, six ABI variants, three consumer links and unchanged matched cost budgets')
    finally:(o/'results.json').write_text(json.dumps(result,indent=2)+'\n')
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for n in ['compiler','shared-root','consumer-root','p75-baseline','p75-reference','model-probe','out']:p.add_argument('--'+n,type=Path,required=True)
    p.add_argument('--resume',action='store_true',help='Reuse completed, hash-verified input/gate receipts after a qualification interruption')
    p.add_argument('--cost-output',type=Path,help='Fresh measurement root after a failed resource-read attempt')
    p.add_argument('--budget',type=Path,default=ROOT/'tests/performance/solver_overlay_budget.json')
    a=p.parse_args()
    for n,v in vars(a).items():
        if isinstance(v,Path):setattr(a,n,v.resolve())
    check(a)
