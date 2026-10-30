#!/usr/bin/env python3
"""P7.4 directional C contracts, explicit static refusals and numerical adapter proof."""
import argparse, hashlib, json, math, os, shutil, subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
FIX=ROOT/'tests/fixtures/solver_physics/structural_boundaries/probe.c'

def validate(output):
    rows={};releases={};basis=[];lines=[]
    for row in output.splitlines():
        f=row.split();name=f[0];v=[float(x) for x in f[1:]]
        if name=='B':basis.append(v)
        elif name=='LINE':lines.append(v)
        elif name=='RELEASE':releases[int(v[0])]=v[1:]
        else:assert name not in rows;rows[name]=v
    def equal(actual,expected):
        assert len(actual)==len(expected),(len(actual),len(expected))
        for a,b in zip(actual,expected):assert math.isfinite(a) and abs(a-b)<=1e-10+1e-10*abs(b),(a,b)
    qs=[[1,2,3,4,5,6],[2,-1,3,5,-4,6],[-1,-2,3,-4,-5,6],[2.2,.4,3,6.4,-.2,6]]
    fs=[[10,20,30,40,50,60],[-20,10,30,-50,40,60],[-10,-20,30,-40,-50,60],[-10,20,30,-16,62,60]]
    assert len(basis)==len(lines)==4
    for i,(q,f) in enumerate(zip(qs,fs)):
        work=sum(x*y for x,y in zip([1,2,3,4,5,6],f))
        equal(basis[i],[i,*q,*f,work,work])
    for a,b in zip(lines,[[10,20],[20,-10],[-10,-20],[22,4]]):equal(a,b)
    k=[0.0]*36
    for i,v in [(0,1.64),(1,-.48),(6,-.48),(7,1.36),(14,3),(21,4.64),(22,-.48),(27,-.48),(28,4.36),(35,6)]:k[i]=v
    equal(rows['K'],k);equal(rows['RIGID'],[0]*6);equal(rows['TRANSLATION'],[0]*6)
    a=48;b=2.88;L=5
    original=[[a,0,0,-a,0,0],[0,12*b,6*b*L,0,-12*b,6*b*L],[0,6*b*L,4*b*L*L,0,-6*b*L,2*b*L*L],[-a,0,0,a,0,0],[0,-12*b,-6*b*L,0,12*b,-6*b*L],[0,6*b*L,2*b*L*L,0,-6*b*L,4*b*L*L]]
    cond_a=[[a,0,0,-a,0,0],[0,3*b,0,0,-3*b,3*b*L],[0]*6,[-a,0,0,a,0,0],[0,-3*b,0,0,3*b,-3*b*L],[0,3*b*L,0,0,-3*b*L,3*b*L*L]]
    cond_b=[[a,0,0,-a,0,0],[0,3*b,3*b*L,0,-3*b,0],[0,3*b*L,3*b*L*L,0,-3*b*L,0],[-a,0,0,a,0,0],[0,-3*b,-3*b*L,0,3*b,0],[0]*6]
    cond_both=[[a,0,0,-a,0,0],[0]*6,[0]*6,[-a,0,0,a,0,0],[0]*6,[0]*6]
    expected_loads=[[0,30,25,0,30,-25],[0,22.5,0,0,37.5,-37.5],[0,37.5,37.5,0,22.5,0],[0,30,0,0,30,0]]
    expected_rotations=[[0,0],[25/288,0],[0,-25/288],[25/144,-25/144]]
    assert set(releases)==set(range(4))
    for i,m in enumerate([original,cond_a,cond_b,cond_both]):equal(releases[i],[v for row in m for v in row]+expected_rotations[i]+expected_loads[i])
    for name,v in [('REDUCED',[19,2,3,5]),('RHS',[-26,20,30,62]),('RESTORED',[-26,20,30,-26,62,0]),('SOLVED',[1,2]),('REFUSALS',[4])]:equal(rows[name],v)
    return sum(len(v) for v in rows.values())+sum(map(len,basis))+sum(map(len,lines))+sum(map(len,releases.values()))

def check(compiler,source,base,numerical,out):
    commands=[];refusals=[];result={'passed':False,'commands':commands,'refusals':refusals,'compiler_sha256':hashlib.sha256(compiler.read_bytes()).hexdigest(),'runner_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}
    env={**os.environ,'FISICS_MAX_PROCS':'0','PYTHONDONTWRITEBYTECODE':'1'};env.pop('FISICS_OVERLAY',None)
    model=source/'physics/frame_policy.json';header=source/'src/core_structural_frame_boundaries.h';text=header.read_text()
    paths=[header,model,source/'physics/frame_catalog.json',source/'src/core_structural_frame_physical.h',*[source/f'src/core_structural_frame_{p}.c' for p in ['element','loads','recovery','assemble','linear','result']]]
    pins={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}
    numerical_receipt=json.loads((numerical/'results.json').read_text())
    assert numerical_receipt['passed'] and numerical_receipt['compiler_sha256']==result['compiler_sha256']
    for p in paths:
        if p.suffix in ['.c','.h']:
            assert numerical_receipt['candidate_sha256'][str(p.relative_to(source))]==pins[str(p)],('numerical source identity mismatch',p)
    libraries=[numerical/x/'structural.a' for x in ['candidate-clang','candidate-fisics']]
    library_pins={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in libraries}
    result['numerical_receipt_sha256']=hashlib.sha256((numerical/'results.json').read_bytes()).hexdigest()
    result['numerical_library_sha256']=library_pins
    mapping={'element':['cs_frame_local_matrix_pack_assumed_si','cs_frame_local_release_condense','cs_frame_local_to_world_stiffness'],
             'loads':['cs_frame_local_release_load_assumed_si','cs_frame_local_to_world_loads','cs_frame_scatter_world_load_assumed_si'],
             'recovery':['cs_frame_world_coordinates_restore_assumed_si','cs_frame_world_to_local_coordinates','cs_frame_local_coordinates_pack_assumed_order'],
             'assemble':['cs_frame_world_matrix_restore_assumed_si','cs_frame_scatter_world_stiffness_assumed_si'],
             'result':['cs_frame_restore_world_solution_assumed_si']}
    for part,names in mapping.items():
        code=(source/f'src/core_structural_frame_{part}.c').read_text()
        for name in names:assert code.count(name+'(')==1,('maintained call mapping changed',part,name)
    result['maintained_call_sites']=mapping
    def run(label,cmd,code=0):
        p=subprocess.run(list(map(str,cmd)),capture_output=True,text=True,env=env,timeout=180)
        (out/(label+'.log')).write_text(p.stdout+p.stderr);commands.append(dict(label=label,command=list(map(str,cmd)),exit=p.returncode,expected=code))
        assert p.returncode==code,(label,p.returncode,(p.stdout+p.stderr)[-1600:])
        assert code or 'Error:' not in p.stdout+p.stderr,label
        return p.stdout
    includes=['-I',ROOT/'include','-I',source/'include','-I',source/'src','-I',base/'include']
    semantic=['--overlay=physics-semantics','--physical-semantics-policy',model,'-DCORE_STRUCTURAL_PHYSICAL_SEMANTICS=1']
    wrapper=out/'scope.c';local=out/'core_structural_frame_boundaries.h'
    def refuse(label,fn,src=None,modified=None):
        local.write_text(text if modified is None else modified);wrapper.write_text('#include "core_structural_frame_boundaries.h"\n')
        report=out/(label+'.json')
        run(label,[compiler,*semantic,*includes,'--physical-semantics-strict','--physical-semantics-allow-literal-assumptions','--physical-semantics-allow-declared-boundaries','--physical-semantics-function',fn,'--emit-physical-semantics-json',report,src or wrapper],1)
        d=json.loads(report.read_text());s=d['selection'];assert not s['accepted']
        refusals.append({'function':fn,'selection':s,'frame_assessment':'not inferred; runtime member identity/basis and mixed storage remain declared numerical boundaries'})
    try:
        scopes=['cs_frame_world_coordinates_restore_assumed_si','cs_frame_world_to_local_coordinates','cs_frame_local_coordinates_pack_assumed_order','cs_frame_world_to_local_line_load','cs_frame_local_to_world_loads','cs_frame_local_matrix_pack_assumed_si','cs_frame_world_matrix_restore_assumed_si','cs_frame_local_release_condense','cs_frame_local_to_world_stiffness','cs_frame_released_rotations_assumed_si','cs_frame_local_release_load_assumed_si','cs_frame_scatter_world_stiffness_assumed_si','cs_frame_scatter_world_load_assumed_si','cs_frame_restore_world_solution_assumed_si']
        for fn in scopes:refuse(fn,fn)
        refuse('raw-scaled-solve','cs_frame_dense_scaled_solve_assumed_si',source/'src/core_structural_frame_linear.c')
        refuse('raw-global-assembly','cs_frame_assemble',source/'src/core_structural_frame_assemble.c')
        refuse('raw-joint-assembly','cs_frame_connections',source/'src/core_structural_frame_connections.c')
        # Directional C record contracts reject local/world and force/coordinate confusion.
        invalid=[('wrong-direction','CsFrameLocalCoordinates q; CsFrameLocalCoordinates out; cs_frame_world_to_local_coordinates(&b,&q,&out);'),('wrong-matrix-direction','CsFrameWorldMatrix k; CsFrameWorldMatrix out; cs_frame_local_to_world_stiffness(&b,&k,&out);'),('force-coordinate-swap','CsFrameWorldLoads q; CsFrameLocalCoordinates out; cs_frame_world_to_local_coordinates(&b,&q,&out);')]
        for label,body in invalid:
            wrapper.write_text('#include "core_structural_frame_boundaries.h"\nvoid check(void){CsFrameMemberBasis b={1,0};'+body+'}\n')
            run(label+'-clang',['clang','-std=c11','-pedantic-errors','-Werror=incompatible-pointer-types',*includes,'-fsyntax-only',wrapper],1)
            run(label+'-fisics',[compiler,*semantic,*includes,'--physical-semantics-strict','--physical-semantics-allow-literal-assumptions','--physical-semantics-allow-declared-boundaries','--physical-semantics-function','check','--emit-physical-semantics-json',out/(label+'.json'),wrapper],1)
        # Values and references from exact simple orientations/closed-form releases.
        local.write_text(text)
        probe=out/'probe.c';shutil.copy2(FIX,probe)
        values={}
        variants=[('clang','clang',['-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O0','-ffp-contract=off'],'candidate-clang'),('units',compiler,['--overlay=physics-units'],'candidate-fisics'),('semantic',compiler,semantic,'candidate-fisics')]
        for label,tool,args,library in variants:
            obj=out/(label+'.o');exe=out/label
            run(label+'-compile',[tool,*args,*includes,'-c',probe,'-o',obj]);run(label+'-link',['clang',obj,numerical/library/'structural.a','-lm','-o',exe])
            output=run(label+'-run',[exe]);values[label]=validate(output)
        result['numeric_values_per_variant']=values
        # Numeric adapter bugs remain ordinary legal C. Static refusal must not
        # be advertised as proof of these bodies; the independent oracle detects them.
        mutations=[('rotation-sign','-s*world->value.translation[i]+c*world->value.translation[i+1]','s*world->value.translation[i]+c*world->value.translation[i+1]','cs_frame_world_to_local_coordinates'),('transpose','t[k][i]*local->value[k][l]*t[l][j]','t[i][k]*local->value[k][l]*t[j][l]','cs_frame_local_to_world_stiffness'),('dof-swap','a,a+1,a+2,b,b+1,b+2','a,a+2,a+1,b,b+1,b+2','cs_frame_world_coordinates_restore_assumed_si'),('rotation-force-swap','ordered[2]=local->value.rotation[0]','ordered[2]=local->value.translation[1]','cs_frame_local_coordinates_pack_assumed_order'),('basis-erasure','double c=basis->cosine,s=basis->sine;','double c=1,s=0;','cs_frame_local_to_world_stiffness'),('load-moment-swap','world->value.moment[end]=local->value.moment[end];','world->value.moment[end]=local->value.force[i];','cs_frame_local_to_world_loads'),('release-end-swap','size_t r=release_a ? 2u : 5u;','size_t r=release_a ? 5u : 2u;','cs_frame_local_release_condense'),('restore-map','solved_rhs[index]','solved_rhs[i]','cs_frame_restore_world_solution_assumed_si')]
        for label,old,new,fn in mutations:
            assert text.count(old)==1,label;changed=text.replace(old,new)
            refuse(label+'-static-refusal',fn,modified=changed)
            run(label+'-compile',['clang','-std=c11','-pedantic-errors',*includes,'-c',probe,'-o',out/(label+'.o')]);run(label+'-link',['clang',out/(label+'.o'),numerical/'candidate-clang/structural.a','-lm','-o',out/label])
            observed=run(label+'-run',[out/label]);caught=False
            try:validate(observed)
            except AssertionError:caught=True
            assert caught,label
        # A forced cast can bypass nominal C direction, not establish frame identity.
        refuse('context-erasure','cs_frame_world_to_local_coordinates',modified=text.replace('CsFrameWorldCoordinates *world','CsFrameLocalCoordinates *world'))
        local.write_text(text)
        assert all(hashlib.sha256(Path(p).read_bytes()).hexdigest()==h for p,h in pins.items()),'selected source/model drift'
        assert all(hashlib.sha256(Path(p).read_bytes()).hexdigest()==h for p,h in library_pins.items()),'numerical library drift'
        result.update(source_sha256=pins,passed=True,frame_registry_extended=False,static_frame_identity='unknown',numeric_mutants_rejected=len(mutations),ordinary_controls=['cs_frame_world_dofs_assumed_order','geometric_rank index/pivot controls'],scope='declared runtime 2D/member/raw boundaries; no general frame/point or matrix inference')
        print(f'Structural frame boundaries: PASS; {len(refusals)} explicit refusals, {len(commands)} commands, {values["clang"]} numeric values per variant')
    finally:(out/'results.json').write_text(json.dumps(result,indent=2)+'\n')

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for n in ['compiler','source-root','base-root','numerical-output','out']:p.add_argument('--'+n,type=Path,required=True)
    a=p.parse_args();a.out.mkdir(parents=True,exist_ok=False)
    check(a.compiler.resolve(),a.source_root.resolve(),a.base_root.resolve(),a.numerical_output.resolve(),a.out.resolve())
if __name__=='__main__':main()
