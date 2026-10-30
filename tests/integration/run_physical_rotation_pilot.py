#!/usr/bin/env python3
"""Bounded copied structural rotation pilot. Body/basis trust is explicit.

Owner sources are copied unchanged and pinned. The raw status/array bridge is
compiled as ordinary C; a separate annotated caller qualifies its declared
interface. No whole-body physical or runtime-basis proof is claimed.
"""
import argparse,hashlib,json,math,os,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
FIX=ROOT/'tests/fixtures/physical_semantics/rotations'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def check(compiler,probe,out,owner):
    pins=json.loads((FIX/'source_pins.json').read_text())
    assert all(sha(owner/k)==v for k,v in pins.items()),'owner drift; audit before repinning'
    snapshot=out/'snapshot';snapshot.mkdir()
    for k in pins:
        p=snapshot/k;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes((owner/k).read_bytes())
    inc=['-I'+str(snapshot/'include'),'-I'+str(snapshot/'src'),'-I'+str(ROOT/'include'),'-I'+str(out)]
    commands=[];result=dict(passed=False,compiler_sha256=sha(compiler),source_pins=pins,commands=commands)
    env={**os.environ,'FISICS_MAX_PROCS':'0','PYTHONDONTWRITEBYTECODE':'1'};env.pop('FISICS_OVERLAY',None);env.pop('DISABLE_CODEGEN',None)
    def write(n,s):
        p=out/n;p.write_text(s if isinstance(s,str) else json.dumps(s,indent=2)+'\n');return p
    def run(n,args,code=0):
        p=subprocess.run(list(map(str,args)),cwd=ROOT,env=env,capture_output=True,text=True,timeout=120)
        write(n+'.log',p.stdout+p.stderr);commands.append(dict(id=n,command=list(map(str,args)),exit=p.returncode,expected=code))
        assert p.returncode==code,(n,p.returncode,(p.stdout+p.stderr)[-4500:]);return p.stdout
    policy=json.loads((ROOT/'tests/fixtures/physical_semantics/frames/policy.json').read_text());policy['frame_transforms']=[]
    write('catalog.json',json.loads((ROOT/'tests/fixtures/physical_semantics/frames/catalog.json').read_text()))
    def attr(k,f):
        dim=dict(force='force',displacement='length',torque='energy',position='length')[k]
        return f'FISICS_DIM({dim}) FISICS_QUANTITY_KIND("quantity.{k}") FISICS_REPRESENTATION("rep.record.double.r1.n3.{"axial" if k=="torque" else "polar"}") FISICS_FRAME("frame.{f}")'+(' FISICS_MOMENT_REFERENCE("point.world_zero")' if k=='torque' else '')+(' FISICS_APPLIED_AT("point.world_zero")' if k=='force' else '')+(' FISICS_PHYSICAL_ORIGIN("origin.'+f+'")' if k=='position' else '')
    head='#include <fisics/extensions.h>\ntypedef struct RotationValue { double x,y,z; } RotationValue;\n'
    bodies=[];declarations=[]
    for k in ['force','displacement','torque','position']:
        for src,dst,direction in [('member_a','world','true'),('world','member_a','false')]:
            name=f'{k}_{src}_to_{dst}';tid='transform.'+name
            rep='rep.record.double.r1.n3.'+('axial' if k=='torque' else 'polar')
            policy['frame_transforms'].append(dict(id=tid,source_frame='frame.'+src,target_frame='frame.'+dst,operation='map_point' if k=='position' else 'rotate_vector',input_kind='quantity.'+k,output_kind='quantity.'+k,input_representation=rep,output_representation=rep,provenance='declared_implementation_and_basis'))
            signature=f'RotationValue {name}(RotationValue value {attr(k,src)}) {attr(k,dst)} FISICS_FRAME_TRANSFORM("{tid}")'
            declarations.append(signature+';')
            body=signature+' {\n double input[3]={value.x,value.y,value.z}, output[3];\n CoreStructuralStatus status=core_structural_spatial_vector_map(&member_a_basis,'+direction+',input,output,0);\n if(status!=CORE_STRUCTURAL_OK) _Exit(73);\n RotationValue result={output[0],output[1],output[2]};\n return result;\n}\n'
            if k=='position':body=body.replace('core_structural_spatial_vector_map','core_structural_spatial_point_map')
            bodies.append(body)
    header=write('rotation.h',head+'\n'.join(declarations)+'\n');pol=write('policy.json',policy)
    # Closed basis for this named member at its declared static observer/state.
    # Local x is world +y, local y is world -x, local z is world +z.
    basis='static const CoreStructuralSpatialBasis member_a_basis={{1,2,3},{{0,1,0},{-1,0,0},{0,0,1}}};'
    source='#include "rotation.h"\n#include "core_structural_spatial.h"\n#include <stdlib.h>\n'+basis+'\n'+'\n'.join(bodies)
    impl=write('rotation.c',source)
    caller=write('acceptance.c','#include "rotation.h"\n'+f'RotationValue check(RotationValue a {attr("force","member_a")}) {attr("force","world")} {{a.x=a.x;return force_member_a_to_world(a);}}\n')
    physical=['--overlay=physics-semantics','--physical-semantics-policy',pol]
    try:
        run('interface',[compiler,*inc,*physical,'--dump-sema',caller])
        pp=run('preprocess_interface',['clang','-D__FISICS__=1',*inc,'-E','-P',caller]);raw=write('acceptance.raw.c',pp)
        run('strict_interface',[probe,pol,raw,0,1,0,1])
        # The same header erases fully under ordinary C. This intentionally does
        # not claim strict qualification for the raw array/status bridge body.
        pp=run('preprocess_body',['clang','-fno-blocks',*inc,'-E','-P',impl]);plain=write('rotation.plain.c',pp)
        abi=[]
        for tool,exe in [('clang','clang'),('fisics',compiler)]:
            flags=['-std=c11','-pedantic-errors','-Werror'] if tool=='clang' else []
            run('compile_wrapper_'+tool,[exe,*flags,*inc,'-c',impl if tool=='clang' else plain,'-o',out/('wrapper_'+tool+'.o')])
            for stem in ['core_structural_spatial_basis','core_structural_model']:
                run('compile_'+stem+'_'+tool,[exe,*inc,'-DCORE_STRUCTURAL_VERSION="'+(snapshot/'VERSION').read_text().strip()+'"','-c',snapshot/'src'/(''+stem+'.c'),'-o',out/(stem+'_'+tool+'.o')])
        harness='#include \"core_structural_spatial.h\"\n#include <stdio.h>\n#include <stdlib.h>\n#include <math.h>\n#include "rotation.h"\nint main(int argc,char **argv){RotationValue a={2,-3,5};if(argc>1){a.x=NAN;force_member_a_to_world(a);return 99;}\n'
        for k in ['force','displacement','torque','position']:
            harness+='{RotationValue w='+k+'_member_a_to_world(a),l='+k+'_world_to_member_a(a),b='+k+'_world_to_member_a(w);printf("'+k+' %.17g %.17g %.17g %.17g %.17g %.17g %.17g %.17g %.17g\\n",w.x,w.y,w.z,l.x,l.y,l.z,b.x,b.y,b.z);}'
        # Additional labelled vectors allow an independent dot/cross oracle.
        harness+='RotationValue u={1,2,3},v={4,-1,2},t={7,10,-9};u=displacement_member_a_to_world(u);v=force_member_a_to_world(v);t=torque_member_a_to_world(t);printf("covariance %.17g %.17g %.17g %.17g %.17g %.17g %.17g %.17g %.17g\\n",u.x,u.y,u.z,v.x,v.y,v.z,t.x,t.y,t.z);'
        harness+='double packed[6]={2,-3,5,2,-3,5},out[6],bad[6]={2,-3,5,NAN,0,0},saved[6]={9,8,7,6,5,4};CoreStructuralSpatialBasis basis={{1,2,3},{{0,1,0},{-1,0,0},{0,0,1}}};if(core_structural_spatial_wrench_map(&basis,1,packed,out,0))return 10;RotationValue f=force_member_a_to_world(a),m=torque_member_a_to_world(a);if(out[0]!=f.x||out[1]!=f.y||out[2]!=f.z||out[3]!=m.x||out[4]!=m.y||out[5]!=m.z)return 11;for(int i=0;i<6;i++)out[i]=saved[i];if(!core_structural_spatial_wrench_map(&basis,1,bad,out,0))return 12;for(int i=0;i<6;i++)if(out[i]!=saved[i])return 13;printf("wrench %g %g %g %g %g %g\\n",f.x,f.y,f.z,m.x,m.y,m.z);return 0;}\n'
        main=write('main.c',harness)
        main_plain=write('main.plain.c',run('preprocess_harness',['clang','-fno-blocks',*inc,'-E','-P',main]))
        for tool,exe in [('clang','clang'),('fisics',compiler)]:
            run('harness_'+tool,[exe,*inc,'-c',main if tool=='clang' else main_plain,'-o',out/('main_'+tool+'.o')])
        def oracle(text,identity=False):
            lines=text.splitlines();assert len(lines)==6,text
            wanted=[2,-3,5]*3 if identity else [3,2,5,-3,-2,5,2,-3,5]
            for line in lines[:3]:assert list(map(float,line.split()[1:]))==wanted,line
            point=list(map(float,lines[3].split()[1:]));assert point==([3,-1,8,1,-5,2,2,-3,5] if identity else [4,4,8,-5,-1,2,2,-3,5]),point
            vals=list(map(float,lines[4].split()[1:]));u,v,t=vals[:3],vals[3:6],vals[6:]
            expected=[1,2,3,4,-1,2,7,10,-9] if identity else [-2,1,3,1,4,2,-10,7,-9]
            assert vals==expected,(vals,expected)
            assert sum(x*x for x in u)==14 and sum(x*x for x in v)==21
            assert sum(x*y for x,y in zip(u,v))==8
            assert [u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]]==t
            wrench=list(map(float,lines[5].split()[1:]));assert wrench==([2,-3,5]*2 if identity else [3,2,5]*2)
            return 51
        for tool in ['clang','fisics']:
            binary=out/('pilot_'+tool)
            run('link_'+tool,['clang',out/('wrapper_'+tool+'.o'),out/('core_structural_spatial_basis_'+tool+'.o'),out/('core_structural_model_'+tool+'.o'),out/('main_'+tool+'.o'),'-o',binary])
            abi.append(oracle(run('numerical_'+tool,[binary])))
            run('fail_nonfinite_'+tool,[binary,'nonfinite'],73)
        for wrapper,caller_tool in [('clang','fisics'),('fisics','clang')]:
            label=wrapper+'_'+caller_tool;binary=out/label
            run('link_mixed_'+label,['clang',out/('wrapper_'+wrapper+'.o'),out/('core_structural_spatial_basis_'+wrapper+'.o'),out/('core_structural_model_'+wrapper+'.o'),out/('main_'+caller_tool+'.o'),'-o',binary])
            abi.append(oracle(run('numerical_mixed_'+label,[binary])))
            run('fail_mixed_'+label,[binary,'nonfinite'],73)
        for label,replacement,exit_code in [('identity','{{1,2,3},{{1,0,0},{0,1,0},{0,0,1}}}',0),('reflection','{{1,2,3},{{1,0,0},{0,1,0},{0,0,-1}}}',73),('nonorthonormal','{{1,2,3},{{2,0,0},{0,1,0},{0,0,1}}}',73),('nonfinite_basis','{{NAN,2,3},{{1,0,0},{0,1,0},{0,0,1}}}',73)]:
            code=source.replace(basis,'static const CoreStructuralSpatialBasis member_a_basis='+replacement+';')
            altered=write(label+'.c','#include <math.h>\n'+code);obj=out/(label+'.o');binary=out/label
            run('compile_'+label,['clang',*inc,'-c',altered,'-o',obj])
            main_obj=out/'main_clang.o'
            if label=='identity':
                identity_main=write('identity_main.c',harness.replace('{{0,1,0},{-1,0,0},{0,0,1}}','{{1,0,0},{0,1,0},{0,0,1}}'));main_obj=out/'identity_main.o';run('compile_identity_main',['clang',*inc,'-c',identity_main,'-o',main_obj])
            run('link_'+label,['clang',obj,out/'core_structural_spatial_basis_clang.o',out/'core_structural_model_clang.o',main_obj,'-o',binary])
            text=run('run_'+label,[binary],exit_code)
            if not exit_code:oracle(text,True)
            else:assert not text,'failure wrapper published a successful value'
        mutants=[]
        for label,code in [('transpose',source.replace('&member_a_basis,true','&member_a_basis,false').replace('&member_a_basis,false,input','&member_a_basis,true,input',1)),('sign',source.replace('output[0],output[1],output[2]','-output[0],output[1],output[2]')),('origin_sign',source.replace('member_a_basis={{1,2,3}','member_a_basis={{-1,-2,-3}'))]:
            if label=='transpose':code=source.replace('&member_a_basis,true','&member_a_basis,TEMP').replace('&member_a_basis,false','&member_a_basis,true').replace('&member_a_basis,TEMP','&member_a_basis,false')
            altered=write('mutant_'+label+'.c',code);obj=out/('mutant_'+label+'.o');binary=out/('mutant_'+label)
            run('compile_mutant_'+label,['clang',*inc,'-c',altered,'-o',obj]);run('link_mutant_'+label,['clang',obj,out/'core_structural_spatial_basis_clang.o',out/'core_structural_model_clang.o',out/'main_clang.o','-o',binary]);text=run('run_mutant_'+label,[binary],0 if label=='origin_sign' else 11);rejected=False
            try:oracle(text)
            except AssertionError:rejected=True
            assert rejected;mutants.append(label)
        # A broken failure branch returning typed zero must fail qualification.
        bad=source.replace(basis,'static const CoreStructuralSpatialBasis member_a_basis={{1,2,3},{{1,0,0},{0,1,0},{0,0,-1}}};').replace('if(status!=CORE_STRUCTURAL_OK) _Exit(73);','if(status!=CORE_STRUCTURAL_OK) { RotationValue zero={0,0,0}; return zero; }')
        altered=write('mutant_failure_zero.c',bad);obj=out/'failure_zero.o';binary=out/'failure_zero'
        run('compile_failure_zero',['clang',*inc,'-c',altered,'-o',obj])
        run('link_failure_zero',['clang',obj,out/'core_structural_spatial_basis_clang.o',out/'core_structural_model_clang.o',out/'main_clang.o','-o',binary])
        text=run('run_failure_zero',[binary],11);assert text,'failure mutant did not exercise a false successful publication'
        rejected=False
        try:oracle(text)
        except AssertionError:rejected=True
        assert rejected;mutants.append('failure_zero')
        result.update(passed=True,wrappers=8,numeric_values_per_variant=abi,mutants_rejected=mutants,trust='declared implementation and closed numeric basis; separate source-interface and runtime evidence',full_phase_acceptance='fixed structural utility pilot, explicit body/basis trust; no inline assembly adoption' )
    finally:
        assert all(sha(owner/k)==v for k,v in pins.items()),'owner changed during qualification'
        write('result.json',result)
def main():
    p=argparse.ArgumentParser(description=__doc__)
    for n in ['compiler','probe','out','structural-root']:p.add_argument('--'+n,type=Path,required=True)
    a=p.parse_args();a.out.mkdir(parents=True,exist_ok=False);check(a.compiler.resolve(),a.probe.resolve(),a.out.resolve(),a.structural_root.resolve())
if __name__=='__main__':main()
