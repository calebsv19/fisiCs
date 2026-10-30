"""P7.6 ABI projections and contained actual Structural Sim consumer checks."""
import hashlib, json, math, os, re, subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def abi(compiler,source,base,out,run,includes):
    out.mkdir();header='#include "core_structural_frame_physical.h"\n#include "core_structural.h"\n'
    attr='CS_PHYSICAL_DIM(force*length) CS_PHYSICAL_KIND("quantity.torque")'
    declaration='double '+attr+' solver_moment(double x '+attr+')'
    callee=out/'callee.c';caller=out/'caller.c'
    callee.write_text(header+declaration+'{return x;}\nvoid relay(const CsPhysicalLoads *a,CsPhysicalLoads *b){for(unsigned i=0;i<4;++i)b->force[i]=a->force[i];for(unsigned i=0;i<2;++i)b->moment[i]=a->moment[i];}\n')
    caller.write_text(header+declaration+';\nvoid relay(const CsPhysicalLoads*,CsPhysicalLoads*);\n#include <stdio.h>\n#include <stddef.h>\nint main(void){CsPhysicalLoads a={{1,2,3,4},{5,6}},b={{0},{0}};relay(&a,&b);b.moment[1]=solver_moment(b.moment[0]);printf("%zu %zu %zu %zu %zu %zu %zu %.1f %.1f %.1f\\n",sizeof(CoreStructuralModel),sizeof(CoreStructuralFrameResult),sizeof(CoreStructuralMemberResult),offsetof(CoreStructuralFrameResult,members),offsetof(CoreStructuralMemberResult,end_forces),sizeof(b),offsetof(CsPhysicalLoads,moment),b.force[3],b.moment[0],b.moment[1]);return 0;}\n')
    semantic=['--overlay=physics-semantics','--physical-semantics-policy',source/'physics/frame_policy.json','-DCORE_STRUCTURAL_PHYSICAL_SEMANTICS=1']
    paths={};reports=[]
    for p in [callee,caller]:
        expanded=run(p.stem+'-preprocess',['clang','-std=c11','-fno-blocks','-E','-P','-D__FISICS__=1','-DCORE_STRUCTURAL_PHYSICAL_SEMANTICS=1',*includes,p])
        for mode,names in [('base','dim|unit|quantity_kind|physical_origin|physical_operation'),('units','quantity_kind|physical_origin|physical_operation')]:
            plain=re.sub(r'\[\[\s*fisics::(?:'+names+r')\s*\(.*?\)\s*\]\]',lambda m:''.join('\n' if c=='\n' else ' ' for c in m[0]),expanded,flags=re.S)
            path=out/(p.stem+'-'+mode+'.c');path.write_text(plain);paths[p.stem,mode]=path
        dest=out/(p.stem+'.json')
        run(p.stem+'-receipt',[compiler,*semantic,*includes,'--physical-semantics-function','solver_moment' if p==callee else 'main','--emit-physical-semantics-json',dest,p]);reports.append(dest)
    # Record/pointer composition remains outside the scalar receipt checker.
    # Runtime ABI tests below qualify it; do not weaken this static refusal.
    run('cross-tu-record-refusal',[compiler,'--physical-semantics-check',*reports],code=1)
    scalar=[]
    for name,body,fn in [('scalar-callee',declaration+'{return x;}','solver_moment'),('scalar-caller',declaration+';\ndouble '+attr+' abi_checked_moment(double x '+attr+'){return solver_moment(x);}','abi_checked_moment')]:
        src=out/(name+'.c');src.write_text('#include "core_structural_frame_physical.h"\n'+body+'\n');dest=out/(name+'.json')
        run(name+'-receipt',[compiler,*semantic,*includes,'--physical-semantics-function',fn,'--emit-physical-semantics-json',dest,src]);scalar.append(dest)
    run('cross-tu-forward',[compiler,'--physical-semantics-check',*scalar]);run('cross-tu-reverse',[compiler,'--physical-semantics-check',*reversed(scalar)])
    expected='256376 115888 312 19584 48 48 32 4.0 5.0 5.0\n'
    tools={'clang':['clang','-std=c11','-pedantic-errors'],'physical':[compiler,*semantic],'base':[compiler],'units':[compiler,'--overlay=physics-units']}
    variants=[]
    for left,right in [('clang','clang'),('physical','physical'),('physical','clang'),('clang','physical'),('base','base'),('units','units')]:
        label=left+'-'+right;objects=[]
        for p,mode in [(callee,left),(caller,right)]:
            obj=out/(label+'-'+p.stem+'.o');run(label+'-'+p.stem,[*tools[mode],*includes,'-c',paths.get((p.stem,mode),p),'-o',obj]);objects.append(obj)
        exe=out/label;run(label+'-link',['clang',*objects,'-o',exe]);actual=run(label+'-run',[exe]);assert actual==expected,(label,actual,expected);variants.append(dict(name=label,output=actual))
    result=dict(passed=True,scope='actual public layouts, physical-carrier pointer relay and scalar torque ABI; not arbitrary aggregate ABI',variants=variants,expected=expected,source_sha256={str(p):sha(p) for p in [callee,caller]})
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n');return result

def equivalent(a,b,path='root'):
    # JSON has one number type: printers may spell numerical zero as 0 while
    # another toolchain emits a tiny finite floating residual. Keep the same
    # abs/relative numerical tolerance; keys, booleans and text remain exact.
    if isinstance(a,(int,float)) and not isinstance(a,bool) and isinstance(b,(int,float)) and not isinstance(b,bool):
        assert math.isfinite(a) and math.isfinite(b) and abs(a-b)<=1e-8+1e-8*abs(a),(path,a,b)
        return
    assert type(a) is type(b),(path,type(a),type(b))
    if isinstance(a,dict):
        assert a.keys()==b.keys(),path
        for k in a:equivalent(a[k],b[k],path+'.'+k)
    elif isinstance(a,list):
        assert len(a)==len(b),path
        for i,(x,y) in enumerate(zip(a,b)):equivalent(x,y,path+str(i))
    elif isinstance(a,float):assert math.isfinite(a) and math.isfinite(b) and abs(a-b)<=1e-8+1e-8*abs(a),(path,a,b)
    else:assert a==b,(path,a,b)

def consumer(app,shared,numerical,out,run):
    out.mkdir(exist_ok=True);source=shared/'core/core_structural';version=(source/'VERSION').read_text().strip();variants={};identities={};libraries={}
    for name in ['native','candidate-clang','candidate-fisics']:
        dest=out/name
        cmd=['make',str(dest/'bin/structural_sim'),'identity','-j2','BUILD_DIR='+str(dest),'SHARED_ROOT='+str(shared),'CC=/usr/bin/clang']
        if name!='native':
            archive=numerical/name/'structural.a';libraries[name]=sha(archive)
            # Supported Make variable override: all numerical structural/base
            # definitions come from the separately hash-bound qualified archive.
            cmd+=['CORE_SOURCES='+str(shared/'core/core_units/src/core_units.c'),'LDFLAGS=-lm '+str(archive)]
        run(name+'-consumer-build',cmd,cwd=app,timeout=300)
        identity=json.loads((dest/'build_identity.json').read_text());binary=dest/'bin/structural_sim';assert identity['binary_sha256']==sha(binary)
        inputs=identity['inputs'];assert all(sha(Path(p))==v for p,v in inputs.items())
        # This specifically verifies the maintained private metadata headers were
        # included in the consumer's normal build identity, not just Git HEAD.
        for n in ['physical','boundaries','energy']:
            p=source/f'src/core_structural_frame_{n}.h';assert inputs[str(p)]==sha(p)
        if name=='native':
            for n in ['element','loads','result','recovery']:
                p=source/f'src/core_structural_frame_{n}.c';assert inputs[str(p)]==sha(p)
        else:
            assert not any('/core_structural/src/' in p and p.endswith('.c') for p in inputs)
        identities[name]=dict(build_identity_sha256=sha(dest/'build_identity.json'),archive_sha256=libraries.get(name),binary_sha256=sha(binary),input_count=len(inputs))
        run(name+'-cli-contract',['python3','-B',app/'tests/test_cli.py','--binary',binary],cwd=app,timeout=180)
        values={}
        for example in ['cantilever_nodal','elastic_gravity_frame','uniform_supported_span']:
            value=json.loads(run(name+'-'+example,[binary,'solve-static',app/'examples'/(example+'.json')],cwd=app))
            assert value['schema']=='structural_sim_frame_result_v1' and value['status']=='ok' and value['core_version']==version
            d=value['diagnostics'];assert set(['strain_energy_j','external_work_j','algebraic_strain_energy_j','algebraic_external_work_j','member_strain_energy_j','connection_strain_energy_j'])<=d.keys()
            assert abs(2*d['strain_energy_j']-d['external_work_j'])<=1e-12+1e-8*max(abs(2*d['strain_energy_j']),abs(d['external_work_j']))
            values[example]=value
        variants[name]=values
    for name in ['candidate-clang','candidate-fisics']:equivalent(variants['native'],variants[name])
    assert all(sha(numerical/n/'structural.a')==h for n,h in libraries.items())
    result=dict(passed=True,app_version=(app/'VERSION').read_text().strip(),core_version=version,identities=identities,variants=variants,scope='frozen current Main Edit CLI/codec/static result consumer; native Clang plus actual qualified Clang/fisiCs archives; no GUI/install adoption')
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n');return result
