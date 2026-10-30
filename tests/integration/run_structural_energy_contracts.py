#!/usr/bin/env python3
"""P7.5 live energy/work contracts, explicit trust refusals and accounting mutants."""
import argparse, hashlib, json, os, shutil, subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def check(a):
    o=a.out;o.mkdir(parents=True,exist_ok=False);s=a.source_root;c=a.compiler
    numeric=json.loads((a.numerical_output/'results.json').read_text());assert numeric['passed'] and numeric['compiler_sha256']==sha(c), 'numerical qualification must finish first'
    commands=[];cases=[];r=dict(passed=False,commands=commands,cases=cases,compiler_sha256=sha(c),runner_sha256=sha(Path(__file__)))
    header=s/'src/core_structural_frame_energy.h';text=header.read_text()
    pins={str(p):sha(p) for p in [header,s/'src/core_structural_frame_recovery.c',s/'src/core_structural_frame_result.c',s/'physics/frame_policy.json',s/'physics/frame_catalog.json']}
    r['source_sha256']=pins
    mapping={'result':['cs_frame_algebraic_energy_assumed_linear','cs_frame_endpoint_assumed_si'], 'recovery':['cs_frame_member_energy_quadrature','cs_frame_distributed_work_quadrature','cs_frame_joint_energy_assumed_linear','cs_frame_elastic_sum','cs_frame_endpoint_assumed_si','cs_frame_normal_stress_assumed_si','cs_frame_bending_stress_assumed_si']}
    for part,names in mapping.items():
        code=(s/f'src/core_structural_frame_{part}.c').read_text()
        for name in names:assert name+'(' in code,(part,name,'missing actual helper connection')
    r['maintained_call_sites']=mapping
    inc=['-I',ROOT/'include','-I',s/'src','-I',s/'include','-I',a.base_root/'include']
    sem=['--overlay=physics-semantics','--physical-semantics-policy',s/'physics/frame_policy.json','-DCORE_STRUCTURAL_PHYSICAL_SEMANTICS=1']
    env={**os.environ,'FISICS_MAX_PROCS':'0','PYTHONDONTWRITEBYTECODE':'1'};env.pop('FISICS_OVERLAY',None)
    def run(label,cmd,code=0):
        p=subprocess.run(list(map(str,cmd)),capture_output=True,text=True,env=env,timeout=180)
        (o/(label+'.log')).write_text(p.stdout+p.stderr);commands.append(dict(label=label,command=list(map(str,cmd)),exit=p.returncode,expected=code))
        assert p.returncode==code,(label,p.returncode,(p.stdout+p.stderr)[-2000:]);assert code or 'Error:' not in p.stdout+p.stderr
        return p.stdout
    local=o/header.name;wrapper=o/'scope.c'
    def scope(label,fn,code=0,body='',modified=None,units=False,actual=None):
        local.write_text(text if modified is None else modified);wrapper.write_text('#include "core_structural_frame_energy.h"\n'+body+'\n')
        dest=o/(label+'.json');prefix='units-coverage' if units else 'physical-semantics'
        args=['--overlay=physics-units'] if units else sem
        run(label,[c,*args,*inc,'--'+prefix+'-strict','--'+prefix+'-allow-literal-assumptions',*(['--'+prefix+'-allow-declared-boundaries'] if code else []),'--'+prefix+'-function',fn,'--emit-'+prefix+'-json',dest,actual or wrapper],code)
        if dest.exists():
            d=json.loads(dest.read_text());p=d['strict_policy'] if units else d['selection'];assert p['accepted']==(code==0)
            if not code:assert p['checked']>0 and p['blocked']==0
            cases.append(dict(label=label,policy=p))
        else:assert code;cases.append(dict(label=label,refused_before_receipt=True))
    try:
        scope('elastic-sum','cs_frame_elastic_sum')
        for fn in ['cs_frame_member_energy_quadrature','cs_frame_distributed_work_quadrature']:scope(fn,fn,units=True)
        for fn in ['cs_frame_elastic_restore_assumed_linear','cs_frame_endpoint_assumed_si','cs_frame_algebraic_energy_assumed_linear','cs_frame_joint_energy_assumed_linear','cs_frame_member_energy_quadrature','cs_frame_distributed_work_quadrature','cs_frame_normal_stress_assumed_si','cs_frame_bending_stress_assumed_si']:scope('refuse-'+fn,fn,1)
        for fn,part in [('cs_frame_publish','result'),('recover_member','recovery'),('cs_frame_recover','recovery')]:
            scope('whole-scope-refused-'+fn,fn,1,actual=s/f'src/core_structural_frame_{part}.c')
        scope('form-laundering-refused','check',1,'double CS_FRAME_ELASTIC check(double a CS_FRAME_ENERGY CS_FRAME_ENERGY_FORM("energy.thermal")){double raw=a;return (double)raw;}')
        scope('wrong-quadrature-dimensions','cs_frame_member_energy_quadrature',1,modified=text.replace('moment*moment/ei','moment/ei'),units=True)
        for label,args,expr in [
            ('wrong-form','double a CS_FRAME_ELASTIC, double b CS_FRAME_ENERGY CS_FRAME_ENERGY_FORM("energy.thermal")','cs_frame_elastic_sum(a,b)'),
            ('unknown-form','double a CS_FRAME_ELASTIC, double b CS_FRAME_ENERGY','cs_frame_elastic_sum(a,b)'),
            ('unknown-raw','double a CS_FRAME_ELASTIC, double b','cs_frame_elastic_sum(a,b)'),
            ('wrong-torque','double a CS_PHYSICAL_DIM(energy) CS_PHYSICAL_KIND("quantity.energy"), double b CS_PHYSICAL_DIM(scalar) CS_PHYSICAL_KIND("quantity.angle")','cs_physical_rotation_work(a,b)')]:
            scope(label,'check',1,'double CS_FRAME_ELASTIC check('+args+'){return '+expr+';}')
            run(label+'-clang',['clang','-std=c11','-pedantic-errors',*inc,'-fsyntax-only',wrapper])
        # Recovered aggregate has no conversion: thermal+elastic cannot narrow.
        scope('mixed-not-elastic','check',1,'double CS_FRAME_ELASTIC check(double a CS_FRAME_ELASTIC,double b CS_FRAME_ENERGY CS_FRAME_ENERGY_FORM("energy.thermal")){return a+b;}')
        # Independent exact references exercise zero, signs, both DOF families,
        # nonzero member AND joint accounts and stress/quadrature equations.
        runtime=o/'runtime.c';runtime.write_text('''#include "core_structural_frame_energy.h"
#include <math.h>
#include <stdio.h>
#define CHECK(x) do {if(!(x)) return 1;} while(0)
static int near(double a,double b){return isfinite(a)&&fabs(a-b)<1e-12;}
int main(void){
 CHECK(near(cs_frame_elastic_sum(3,5),8));
 CHECK(near(cs_frame_elastic_sum(0,5),5));
 CHECK(near(cs_frame_endpoint_assumed_si(0,8,.25),2));
 CHECK(near(cs_frame_endpoint_assumed_si(2,12,.5),6));
 CHECK(near(cs_frame_endpoint_assumed_si(1,-8,.25),-2));
 CHECK(near(cs_frame_algebraic_energy_assumed_linear(2,.5,12),3));
 CHECK(near(cs_frame_joint_energy_assumed_linear(0,8,.25),1));
 CHECK(near(cs_frame_joint_energy_assumed_linear(2,12,.5),3));
 CHECK(near(cs_frame_member_energy_quadrature(2,1,4,6,8,12),2.5));
 CHECK(near(cs_frame_distributed_work_quadrature(2,1,4,.25,6,.5),4));
 CHECK(near(cs_frame_normal_stress_assumed_si(12,3),4));
 CHECK(near(cs_frame_bending_stress_assumed_si(18,3),6));
 puts("12 independent energy/work/stress references passed");return 0;}
''')
        def runtime_test(label,tool,args,expected=0):
            obj=o/(label+'.o');binary=o/label
            run(label+'-compile',[tool,*args,*inc,'-c',runtime,'-o',obj]);run(label+'-link',['clang',obj,'-o',binary]);run(label+'-run',[binary],expected)
        local.write_text(text)
        for label,tool,args in [('clang','clang',['-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O0','-ffp-contract=off']),('units',c,['--overlay=physics-units']),('semantic',c,sem)]:runtime_test(label,tool,args)
        mutants=[('omit-joint','return member+joint;','return member;'),('double-joint','return member+joint;','return member+joint+joint;'),('wrong-sign','return member+joint;','return member-joint;'),('missing-half','dof,.5*force,relative','dof,force,relative'),('quadrature-half','length*weight/4*','length*weight/2*'),('omit-bending','axial*axial/ea+moment*moment/ei','axial*axial/ea'),('wrong-work-sign','return moment*angle;','return -moment*angle;')]
        for label,old,new in mutants:
            if old not in text:
                # Prepared exact-body work kernel has its own separate source.
                h=o/'core_structural_frame_physical.h';original=(s/'src'/h.name).read_text();assert old in original
                local.write_text(text);h.write_text(original.replace(old,new,1))
            else:
                assert text.count(old)==1;local.write_text(text.replace(old,new,1));h=None
            runtime_test('numeric-'+label,c,['--overlay=physics-units'],1)
            if h:h.unlink()
        local.write_text(text)
        numeric=json.loads((a.numerical_output/'results.json').read_text());assert numeric['passed'] and numeric['compiler_sha256']==sha(c)
        for p in [header,s/'src/core_structural_frame_recovery.c',s/'src/core_structural_frame_result.c']:
            assert numeric['candidate_sha256'][str(p.relative_to(s))]==sha(p)
        # A nonzero translational AND rotational spring plus a beam validates
        # component identity, signs, endpoint torque and both energy accounts.
        fixture=ROOT/'tests/fixtures/solver_physics/structural_energy/joint_probe.c'
        r['joint_fixture_sha256']=sha(fixture)
        outputs={}
        for label in ['baseline-clang','candidate-clang','candidate-fisics']:
            obj=o/(label+'-joint.o');binary=o/(label+'-joint')
            run(label+'-joint-compile',['clang','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror',*inc,'-c',fixture,'-o',obj])
            run(label+'-joint-link',['clang',obj,a.numerical_output/label/'structural.a','-lm','-o',binary])
            outputs[label]=run(label+'-joint-run',[binary])
        assert len(set(outputs.values()))==1, outputs
        r['joint_parity']=outputs
        library=a.numerical_output/'candidate-fisics/structural.a';probe=o/'candidate-fisics-joint.o'
        r['numerical_receipt_sha256']=sha(a.numerical_output/'results.json');r['library_sha256']=sha(library)
        for label,old,new in mutants[:6]:
            local.write_text(text.replace(old,new,1));dest=o/label;dest.mkdir()
            src=dest/'core_structural_frame_recovery.c';shutil.copy2(s/'src'/src.name,src)
            obj=dest/(src.stem+'.o');archive=dest/'structural.a';shutil.copy2(library,archive)
            run('solver-'+label+'-compile',[c,*sem,'-I',o,*inc,'-c',src,'-o',obj])
            run('solver-'+label+'-archive',['ar','r',archive,obj]);binary=dest/'probe'
            run('solver-'+label+'-link',['clang',probe,archive,'-lm','-o',binary])
            run('solver-'+label+'-oracle',[binary],1)
            cases.append(dict(label='solver-'+label,numerically_rejected=True))
        local.write_text(text)
        assert all(sha(Path(p))==v for p,v in pins.items()),'live selected-source drift'
        r['passed']=True
    finally:(o/'results.json').write_text(json.dumps(r,indent=2)+'\n')
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for n in ['compiler','source-root','base-root','numerical-output','out']:p.add_argument('--'+n,type=Path,required=True)
    check(p.parse_args())
