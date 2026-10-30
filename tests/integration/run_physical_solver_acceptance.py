#!/usr/bin/env python3
"""P5.6 joined physical acceptance on a fresh actual structural candidate.

First run run_units_structural_source_contracts.py --physical. This runner refuses
stale candidate/compiler hashes. Generated projections exercise that candidate's
actual carriers; they are explicitly not original solver function bodies.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
from run_units_structural_source_contracts import snapshot, KERNELS
ROOT=Path(__file__).resolve().parents[2]

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def check(compiler,structural,base,out,mechanical=False):
    prior=json.loads((structural/'results.json').read_text());candidate=structural/'candidate'
    assert prior['passed']
    if mechanical:
        assert prior['physical_profile_mode']=='mechanical-v4'
        assert prior['physical_profile']['normalized_erasure_matches_original']
        assert prior['physical_profile']['numerical_source_identical'] is False
    else:
        assert prior['physical_profile']['numerical_source_identical']
    assert prior['compiler_sha256']==sha(compiler) and snapshot(candidate)==prior['candidate_sha256']
    policy=structural/'policy.json';header=candidate/'src/core_structural_frame_physical.h'
    if mechanical:
        assert prior['physical_model_sha256']=={n:sha(structural/n) for n in ['policy.json','catalog.json']},'physical model drift'
    inputs={str(p):sha(p) for p in [policy,structural/'catalog.json',header]}
    env={**os.environ,'FISICS_MAX_PROCS':'0','PYTHONDONTWRITEBYTECODE':'1'}
    env.pop('FISICS_OVERLAY',None);env.pop('DISABLE_CODEGEN',None)
    includes=['-I'+str(ROOT/'include'),'-I'+str(candidate/'src'),'-I'+str(candidate/'include'),'-I'+str(base/'include')]
    flags=['--overlay=physics-semantics','--physical-semantics-policy',str(policy)]
    commands=[];reports={};result=dict(schema='fisics.physical_solver_acceptance.v1',passed=False,compiler_sha256=sha(compiler),runner_sha256=sha(Path(__file__)),commands=commands,reports=reports,inputs=inputs,projection_scope='actual candidate carriers; generated copy/relay acceptance functions, not whole solver provenance')
    def run(label,cmd,expected=0):
        cmd=list(map(str,cmd));p=subprocess.run(cmd,cwd=ROOT,env=env,capture_output=True,text=True,timeout=120)
        text=p.stdout+p.stderr;(out/(label+'.log')).write_text(text)
        commands.append(dict(id=label,command=cmd,exit=p.returncode,expected=expected))
        assert p.returncode==expected,(label,p.returncode,text[-4000:])
        assert expected or 'Error:' not in text,(label,'backend error with zero exit')
        return text
    def report(label,source,fn,expected=0,trust=False,complete=True,permissions=None):
        dest=out/(label+'.json');args=[compiler,*flags,*includes,'--physical-semantics-function',fn,'--emit-physical-semantics-json',dest,'--physical-semantics-strict']
        if permissions is not None:args+=['--physical-semantics-allow-'+p for p in permissions]
        elif trust:args+=['--physical-semantics-allow-literal-assumptions','--physical-semantics-allow-declared-boundaries']
        run(label,[*args,source],expected);d=json.loads(dest.read_text());s=d['selection'];reports[label]=s
        assert s['total']==len(d['obligations'])==s['required']+s['ordinary_controls']
        assert s['required']==s['checked']+s['permitted_trust']+s['blocked']
        assert s['enumeration_complete']==complete and d['dependencies']['complete']
        assert s['accepted']==(expected==0)
        if not expected:assert s['checked']>0 and s['blocked']==0
        else:assert s['blocked']>0
        return dest
    try:
        for fn,part,_ in KERNELS:
            accepted=mechanical and fn in ['cs_physical_apply','cs_physical_recover_end']
            permissions=['literal-assumptions'] if accepted and fn=='cs_physical_apply' else [] if accepted else None
            report(fn,candidate/f'src/core_structural_frame_{part}.c',fn,0 if accepted else 1,True,permissions=permissions)
            if accepted:
                s=reports[fn];expected=(26,2,6) if fn=='cs_physical_apply' else (28,0,6)
                assert (s['checked'],s['permitted_trust'],s['ordinary_controls'])==expected
        if mechanical:
            assert json.loads(policy.read_text())['schema_version']==4
            assert prior['physical_profile']['header_sha256']==sha(header)
            result['physical_profile_mode']='mechanical-v4'
            result['projection_scope']='actual-source apply/recovery bodies plus generated carrier/ABI projections; construction and raw boundaries remain unverified'
            report('apply_literal_permission_required',candidate/'src/core_structural_frame_recovery.c','cs_physical_apply',1)
            # Kind-only mutations leave C dimensions/layout and AST denominators intact.
            original=header.read_text()
            for label,text in [('swapped_couplings',original.replace('quantity.force_per_angle','temporary.swap').replace('quantity.moment_per_displacement','quantity.force_per_angle').replace('temporary.swap','quantity.moment_per_displacement')),
                               ('collapsed_couplings',original.replace('quantity.force_per_angle','quantity.force').replace('quantity.moment_per_displacement','quantity.force'))]:
                header.write_text(text)
                src=candidate/'src/core_structural_frame_recovery.c'
                run(label+'-clang',['clang','-std=c11','-pedantic-errors',*includes,'-fsyntax-only',src])
                for fn in ['cs_physical_apply','cs_physical_recover_end']:
                    report(label+'-'+fn,src,fn,1,True)
                    assert reports[label+'-'+fn]['total']==reports[fn]['total']
                    assert reports[label+'-'+fn]['ordinary_controls']==reports[fn]['ordinary_controls']
            header.write_text(original)
        report('raw_restore',candidate/'src/core_structural_frame_recovery.c','cs_physical_dense_restore_assumed_si',1,True,False)
        prefix='#include "core_structural_frame_physical.h"\n'
        copy='void relay(const CsPhysicalLoads *src,CsPhysicalLoads *dst){for(unsigned i=0;i<4;++i)dst->force[i]=src->force[i];for(unsigned i=0;i<2;++i)dst->moment[i]=src->moment[i];}\n'
        good=out/'relay.c';good.write_text(prefix+copy);report('carrier_relay',good,'relay')
        # Removal keeps numerical C and AST sites; trust cannot restore the kind.
        local=out/'erased';local.mkdir();original=header.read_text()
        for label,pattern in [('kind',r' CS_PHYSICAL_KIND\("[^"]+"\)'),('dimension',r' CS_PHYSICAL_DIM\([^)]*\)')]:
            h=local/'core_structural_frame_physical.h';h.write_text(re.sub(pattern,'',original))
            src=local/(label+'.c');src.write_text(prefix+copy)
            report('removed_'+label,src,'relay',1,True)
            assert reports['removed_'+label]['total']==reports['carrier_relay']['total']
            assert reports['removed_'+label]['ordinary_controls']==reports['carrier_relay']['ordinary_controls']
        h.unlink()
        # Same dimension but stiffness-versus-torque semantic mutation remains legal C.
        bad=out/'wrong_kind.c';bad.write_text(prefix+'void wrong(const CsPhysicalStiffness *k,CsPhysicalLoads *dst){dst->moment[0]=k->rr[0][0];}\n')
        run('wrong-kind-clang',['clang','-std=c11','-pedantic-errors',*includes,'-fsyntax-only',bad])
        run('wrong-kind-physical',[compiler,*flags,*includes,'--emit-diags-json',out/'wrong-kind-diags.json','-c',bad,'-o',out/'wrong.o'],1)
        ds=json.loads((out/'wrong-kind-diags.json').read_text())['diagnostics'];assert any(d['code_id']==4316 for d in ds)
        assert not (out/'wrong.o').exists()
        moment='CS_PHYSICAL_DIM(force*length) CS_PHYSICAL_KIND("quantity.torque")'
        declaration=f'{moment} double solver_moment(double x {moment})'
        callee=out/'callee.c';callee.write_text(prefix+declaration+'{return x;}\n')
        caller=out/'caller.c';caller.write_text(prefix+declaration+';\n'+f'void checked_relay(CsPhysicalLoads *p){{double x {moment}=solver_moment(p->moment[0]);p->moment[1]=x;}}\n')
        main_body='''#include <stdio.h>
#include <stddef.h>
int main(void){CsPhysicalLoads p={{1,2,3,4},{5,6}},q={{0},{0}};relay(&p,&q);checked_relay(&q);printf("%zu %zu %.1f %.1f %.1f\\n",sizeof(q),offsetof(CsPhysicalLoads,moment),q.force[3],q.moment[0],q.moment[1]);return 0;}
'''
        a=report('callee',callee,'solver_moment');b=report('caller',caller,'checked_relay',trust=True)
        assert reports['caller']['permitted_trust']>0
        for n,paths in [('forward',[a,b]),('reverse',[b,a])]:run('cross-tu-'+n,[compiler,'--physical-semantics-check',*paths])
        changed=out/'conflict.c';changed.write_text(prefix+declaration.replace('quantity.torque','quantity.rotational_stiffness')+'{return x;}\n')
        c=report('conflict',changed,'solver_moment')
        for n,paths in [('forward',[b,c]),('reverse',[c,b])]:run('cross-tu-conflict-'+n,[compiler,'--physical-semantics-check',*paths],1)
        # Mixed compilation checks the real record layout and pointers plus scalar
        # quantity ABI. Cross-TU receipt success alone makes no general ABI claim.
        abi_callee=out/'abi-callee.c';abi_callee.write_text(callee.read_text()+copy)
        abi_caller=out/'abi-caller.c';abi_caller.write_text(caller.read_text()+'void relay(const CsPhysicalLoads*,CsPhysicalLoads*);\n'+main_body)
        callee,caller=abi_callee,abi_caller
        paths={}
        for src in [callee,caller]:
            pp=run(src.stem+'-preprocess',['clang','-std=c11','-fno-blocks','-E','-P','-D__FISICS__=1',*includes,src])
            for mode,names in [('base','dim|unit|quantity_kind|physical_origin'),('units','quantity_kind|physical_origin')]:
                path=out/(src.stem+'-'+mode+'.c');path.write_text(re.sub(r'\[\[\s*fisics::(?:'+names+r')\s*\(.*?\)\s*\]\]',lambda m:''.join('\n' if c=='\n' else ' ' for c in m[0]),pp,flags=re.S));paths[src.stem,mode]=path
        compilers={'clang':['clang','-std=c11','-pedantic-errors'],'physical':[compiler,*flags],'base':[compiler],'units':[compiler,'--overlay=physics-units']}
        expected='48 32 4.0 5.0 5.0\n';result['abi_expected']=expected.strip();result['abi_variants']=[]
        for left,right in [('clang','clang'),('physical','physical'),('physical','clang'),('clang','physical'),('base','base'),('units','units')]:
            label=left+'-'+right;objects=[]
            for src,mode in [(callee,left),(caller,right)]:
                obj=out/(label+'-'+src.stem+'.o');source=paths.get((src.stem,mode),src)
                run(label+'-'+src.stem,[*compilers[mode],*includes,'-c',source,'-o',obj]);objects.append(obj)
            exe=out/label;run(label+'-link',['clang',*objects,'-o',exe]);assert run(label+'-run',[exe])==expected
            result['abi_variants'].append(label)
            exe.unlink()
            for obj in objects:obj.unlink()
        # Exact remaining P5.1 design scenarios; no inference from test names.
        domain=out/'domain';domain.mkdir()
        fixture=ROOT/'tests/fixtures/physical_semantics'
        model=json.loads((fixture/'policy.json').read_text())
        model['origins'].append(dict(id='origin.thermal.other',absolute_kind='quantity.temperature.absolute'))
        (domain/'policy.json').write_text(json.dumps(model,indent=2)+'\n')
        (domain/'catalog.json').write_bytes((fixture/'catalog.json').read_bytes())
        supplement=[
            ('angle_assignment','void f(double a ANGLE,double b RATIO){b=a;}',4316),
            ('angle_argument','void take(double b RATIO){} void f(double a ANGLE){take(a);}',4316),
            ('angle_return','RATIO double f(double a ANGLE){return a;}',4316),
            ('angle_merge','void f(double a ANGLE,double b RATIO){double x RATIO=1?a:b;}',4316),
            ('energy_argument','void take(double a ENERGY){} void f(double a ENERGY){take(a);}',0),
            ('angle_disabled','double a ANGLE;',4310),
            ('origin_wrong_absolute_kind','double x FISICS_DIM(temperature) FISICS_QUANTITY_KIND("quantity.temperature.absolute") FISICS_PHYSICAL_ORIGIN("origin.world");',4318),
            ('temperature_assignment','void f(double a TEMPERATURE,double b FISICS_DIM(temperature) FISICS_QUANTITY_KIND("quantity.temperature.absolute") FISICS_PHYSICAL_ORIGIN("origin.thermal.other")){a=b;}',4318),
            ('literal_plus','ANGLE double f(double a ANGLE){return a+1;}',0),
            ('mass_speed_squared','ENERGY double f(double m FISICS_DIM(mass),double v FISICS_DIM(speed)){return m*v*v;}',0)]
        for name,body,code in supplement:
            src=domain/(name+'.c');src.write_text('#include "authoring.h"\n'+body+'\n');diag=domain/(name+'.json')
            args=[compiler,'-I'+str(ROOT/'include'),'-I'+str(fixture)]
            if name!='angle_disabled':args+=['--overlay=physics-semantics','--physical-semantics-policy',domain/'policy.json']
            if code:args+=['--emit-diags-json',diag,'-c',src,'-o',domain/(name+'.o')]
            else:args+=['-c',src,'-o',domain/(name+'.o')]
            run('design-'+name,args,1 if code else 0)
            if code:assert any(d['code_id']==code for d in json.loads(diag.read_text())['diagnostics'])
            if name in ('mass_speed_squared','literal_plus'):
                # Direct compilation may succeed; strict must refuse missing kinds.
                dest=domain/(name+'-strict.json')
                run('design-'+name+'-strict',[compiler,'-I'+str(ROOT/'include'),'-I'+str(fixture),'--overlay=physics-semantics','--physical-semantics-policy',domain/'policy.json','--physical-semantics-function','f','--physical-semantics-strict','--physical-semantics-allow-literal-assumptions','--physical-semantics-allow-declared-boundaries','--emit-physical-semantics-json',dest,src],1)
                assert json.loads(dest.read_text())['selection']['blocked']>0
            for obj in domain.glob('*.o'):obj.unlink()
        assert snapshot(candidate)==prior['candidate_sha256'] and all(sha(Path(p))==h for p,h in inputs.items())
        assert sha(compiler)==result['compiler_sha256']
        # Cost workloads use exactly these verified candidate TUs and projections.
        deps=set(inputs)|{str(p) for p in candidate.rglob('*.h')}|{str(p) for p in (base/'include').rglob('*.h')}|{str(ROOT/'include/fisics/extensions.h'),str(callee),str(caller)}
        tus=[dict(id='structural-'+part,source=str(candidate/f'src/core_structural_frame_{part}.c'),args=includes) for part in ['element','loads','recovery']]
        deps.update(t['source'] for t in tus)
        selections=[dict(id='solver-carrier-relay',source=str(good),function='relay',args=includes,physical=True),dict(id='solver-recovery-report',source=tus[2]['source'],function='cs_physical_recover_end',args=includes,physical=True,accepted=False)]
        if mechanical:
            selections=[dict(id='solver-carrier-relay',source=str(good),function='relay',args=includes,physical=True,permissions=[],expected_checked=2,expected_permitted_trust=0),
                dict(id='solver-apply-strict',source=tus[2]['source'],function='cs_physical_apply',args=includes,physical=True,permissions=['literal-assumptions'],expected_checked=26,expected_permitted_trust=2),
                dict(id='solver-recovery-strict',source=tus[2]['source'],function='cs_physical_recover_end',args=includes,physical=True,permissions=[],expected_checked=28,expected_permitted_trust=0),
                dict(id='solver-construction-report',source=tus[0]['source'],function='cs_physical_build',args=includes,physical=True,accepted=False)]
        manifest=dict(physical_policy=str(policy),dependencies=sorted(deps),translation_units=tus,strict_functions=selections)
        manifest['dependencies'].append(str(good));(out/'cost-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
        result['passed']=True
        print('P6.1.4 mechanical solver acceptance: PASS;' if mechanical else 'P5.6 solver physical acceptance: PASS;',len(commands),'commands; 6 ABI variants; explicit unsupported scopes remain strict refusals')
    finally:(out/'results.json').write_text(json.dumps(result,indent=2)+'\n')

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ['compiler','structural-output','base-root','out']:p.add_argument('--'+name,type=Path,required=True)
    p.add_argument('--mechanical',action='store_true',help='Require a refined v4 candidate, accept application/recovery and retain unsupported scope refusals')
    a=p.parse_args();a.out.mkdir(parents=True,exist_ok=False)
    check(a.compiler.resolve(),a.structural_output.resolve(),a.base_root.resolve(),a.out.resolve(),a.mechanical)
if __name__=='__main__':main()
