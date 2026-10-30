#!/usr/bin/env python3
"""Copied core_math and structural axes representation qualification; no adoption."""
import argparse,hashlib,json,math,os,re,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];FIX=ROOT/'tests/fixtures/physical_semantics/representations'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def extract(text,name):
    match=re.search(r'^.*\b'+re.escape(name)+r'\([^\n]*\)\s*\{',text,re.M);assert match,name
    start=match.start();pos=text.index('{',start);depth=1;end=pos+1
    while depth:
        depth += (text[end]=='{')-(text[end]=='}');end+=1
    return text[start:end]+'\n'
def attr(k,rep=None):
    dim=dict(force='force',displacement='length',energy='energy',torque='energy',ratio='1')[k]
    return f'FISICS_DIM({dim}) FISICS_QUANTITY_KIND("quantity.{k}")'+(f' FISICS_REPRESENTATION("{rep}")' if rep else '')
def oracle(text):
    lines=text.splitlines();assert lines[0]=='ABI 8 12 8 72 0',lines[0];seen=0
    for line in lines[1:]:
        words=line.split();dim,n=int(words[1]),int(words[2]);actual=list(map(float,words[3:]));a,b=CASES[n];a=a[:dim];b=b[:dim]
        expected=[*[x+y for x,y in zip(a,b)],*[x-y for x,y in zip(a,b)],*[x*.5 for x in a],sum(x*y for x,y in zip(a,b))]
        if dim==3:expected += [a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]]
        assert len(actual)==len(expected)
        for x,y in zip(actual,expected):assert math.isfinite(x) and x==y,(dim,n,actual,expected)
        seen+=len(expected)
    assert len(lines)==1+2*len(CASES);return seen
CASES=[([1,0,0],[0,1,0]),([0,1,0],[1,0,0]),([1,2,3],[1,2,3]),([0,0,0],[3,-2,1]),([2,-4,6],[-3,2,1]),([-1,0,2],[0,3,-4])]
def check(compiler,out,math_root,structural_root):
    sources={'math_header':math_root/'include/core_math.h','math_source':math_root/'src/core_math.c','structural_header':structural_root/'include/core_structural_spatial.h'}
    pins=json.loads((FIX/'source_pins.json').read_text());assert {k:sha(p) for k,p in sources.items()}==pins,'owner source drift: review and repin explicitly'
    snapshot=out/'snapshot';snapshot.mkdir();[(snapshot/(k+p.suffix)).write_bytes(p.read_bytes()) for k,p in sources.items()]
    result=dict(passed=False,compiler_sha256=sha(compiler),sources={k:dict(path=str(p),sha256=pins[k]) for k,p in sources.items()},commands=[])
    env={**os.environ,'FISICS_MAX_PROCS':'0','PYTHONDONTWRITEBYTECODE':'1'};env.pop('FISICS_OVERLAY',None);env.pop('DISABLE_CODEGEN',None)
    def write(n,s):p=out/n;p.write_text(s if isinstance(s,str) else json.dumps(s,indent=2)+'\n');return p
    def run(n,args,code=0):
        p=subprocess.run(list(map(str,args)),cwd=ROOT,env=env,capture_output=True,text=True,timeout=120);log=p.stdout+p.stderr;write(n+'.log',log);result['commands'].append(dict(id=n,command=list(map(str,args)),exit=p.returncode,expected=code));assert p.returncode==code,(n,p.returncode,log[-4000:]);assert code or 'Error:' not in log,(n,log[-4000:]);return p.stdout
    model=json.loads((FIX/'policy.json').read_text());write('catalog.json',json.loads((FIX/'catalog.json').read_text()));policy=write('policy.json',model)
    inc=['-I'+str(ROOT/'include'),'-I'+str(out)];physical=['--overlay=physics-semantics','--physical-semantics-policy',policy]
    header=sources['math_header'].read_text();original=sources['math_source'].read_text();records=[];bodies=[];prototypes=[]
    for dim in [2,3]:
        start=header.index('typedef struct CoreMathVec'+str(dim)+'f');end=header.index(';',header.index('}',start))+1;records.append(header[start:end])
        rep=f'rep.record.float.r1.n{dim}.polar'
        for suffix in ['add','sub','scale','dot']+(['cross'] if dim==3 else []):
            op={'sub':'subtract'}.get(suffix,suffix);name=f'core_math_vec{dim}_{suffix}';body=extract(original,name);signature,tail=body.split('{',1)
            kinds={'add':['force']*3,'sub':['force']*3,'scale':['force','ratio','force'],'dot':['force','displacement','energy'],'cross':['displacement','force','torque']}[suffix]
            annotations=[attr(k,None if suffix=='scale' and i==1 or suffix=='dot' and i==2 else f'rep.record.float.r1.n{dim}.'+('axial' if suffix=='cross' and i==2 else 'polar')) for i,k in enumerate(kinds)]
            new=signature.rstrip();params=new[new.index('(')+1:new.rindex(')')].split(',');assert len(params)==2
            new=new[:new.index('(')+1]+','.join(p.strip()+' '+a for p,a in zip(params,annotations))+')'
            new=annotations[2]+f' FISICS_PHYSICAL_OPERATION("helper.{op}.float.n{dim}") '+new
            bodies.append(new+'{'+tail);prototypes.append(new+';')
            # Numerical body bytes are identical to actual owner source.
            assert bodies[-1].split('{',1)[1]==body.split('{',1)[1]
    matrix=attr('ratio','rep.record.double.r2.n3.tensor')
    # Copy only the actual homogeneous member into a storage adapter. The
    # original basis also contains origin_m and is deliberately not bound whole.
    sh=sources['structural_header'].read_text();assert 'double origin_m[3], axes[3][3];' in sh
    matrix_record='typedef struct PilotAxes { double axes[3][3]; } PilotAxes;'
    head=write('pilot.h','#include <fisics/extensions.h>\n'+'\n'.join(records+[matrix_record]+prototypes)+'\n'+matrix+' PilotAxes matrix_copy(PilotAxes a '+matrix+');\n')
    impl=write('pilot.c','#include "pilot.h"\n'+'\n'.join(bodies)+'\n'+matrix+' PilotAxes matrix_copy(PilotAxes a '+matrix+'){PilotAxes b '+matrix+'=a;return b;}\n'+attr('energy')+' float acceptance(CoreMathVec3f a '+attr('force','rep.record.float.r1.n3.polar')+',CoreMathVec3f b '+attr('displacement','rep.record.float.r1.n3.polar')+'){CoreMathVec3f clean '+attr('force','rep.record.float.r1.n3.polar')+'=a;return core_math_vec3_dot(clean,b);}\n')
    harness='#include <stdio.h>\n#include <stddef.h>\n#include "pilot.h"\nint main(void){printf("ABI %zu %zu %zu %zu %zu\\n",sizeof(CoreMathVec2f),sizeof(CoreMathVec3f),offsetof(CoreMathVec3f,z),sizeof(PilotAxes),offsetof(PilotAxes,axes));\n'
    for dim in [2,3]:
        for n,(a,b) in enumerate(CASES):
            T=f'CoreMathVec{dim}f';v=lambda x:'{'+','.join(map(str,x[:dim]))+'}'
            harness+='{'+T+' a='+v(a)+',b='+v(b)+f';{T} sum=core_math_vec{dim}_add(a,b),sub=core_math_vec{dim}_sub(a,b),scale=core_math_vec{dim}_scale(a,.5f);float dot=core_math_vec{dim}_dot(a,b);'
            if dim==3:harness+=f'{T} cross=core_math_vec3_cross(a,b);'
            vals=[o+'.'+axis for o in ['sum','sub','scale'] for axis in 'xyz'[:dim]]+['dot']+(['cross.'+s for s in 'xyz'] if dim==3 else [])
            harness+='printf("CASE '+str(dim)+' '+str(n)+' '+' '.join(['%.9g']*len(vals))+'\\n",'+','.join(vals)+');}\n'
    harness+='PilotAxes m={{{1,2,3},{4,5,6},{7,8,9}}};PilotAxes c=matrix_copy(m);for(int i=0;i<3;i++)for(int j=0;j<3;j++)if(c.axes[i][j]!=m.axes[i][j])return 2;return 0;}\n';main=write('main.c',harness)
    reports={}
    try:
        for fn,permissions in [('matrix_copy',[]),('acceptance',['declared-boundaries'])]:
            dest=out/(fn+'.json');run('strict_'+fn,[compiler,*inc,*physical,'--physical-semantics-function',fn,'--physical-semantics-strict',*['--physical-semantics-allow-'+p for p in permissions],'--emit-physical-semantics-json',dest,impl]);d=json.loads(dest.read_text());assert d['selection']['accepted'];reports[fn]=d['selection']
        numerical=[]
        for tool in ['clang','physical','units','base']:
            for role,src in [('impl',impl),('main',main)]:
                flags=[];selected=src
                if tool=='clang':exe='clang';flags=['-std=c11','-pedantic-errors','-Werror']
                else:
                    exe=compiler
                    if tool=='physical':flags=physical
                    else:
                        pp=run('prepare_'+role+'_'+tool,['clang','-std=c11','-fno-blocks','-D__FISICS__=1',*inc,'-E','-P',src])
                        names='quantity_kind|physical_origin|representation|physical_operation'+('|dim|unit' if tool=='base' else '')
                        selected=write(role+'-'+tool+'.c',re.sub(r'\[\[\s*fisics::(?:'+names+r')\s*\(.*?\)\s*\]\]',lambda m:''.join('\n' if c=='\n' else ' ' for c in m[0]),pp,flags=re.S))
                        flags=['--overlay=physics-units'] if tool=='units' else []
                run('compile_'+tool+'_'+role,[exe,*inc,*flags,'-c',selected,'-o',out/(role+'-'+tool+'.o')])
        for left,right in [('clang','clang'),('physical','physical'),('clang','physical'),('physical','clang'),('units','units'),('base','base')]:
            n=left+'_'+right;binary=out/n;run('link_'+n,['clang',out/('impl-'+left+'.o'),out/('main-'+right+'.o'),'-o',binary]);text=run('run_'+n,[binary]);numerical.append(oracle(text))
        original_typed=impl.read_text()
        for label,old,new in [('cross_sign','(a.y * b.z) - (a.z * b.y)','(a.z * b.y) - (a.y * b.z)'),('dot_term','(a.x * b.x) + (a.y * b.y) + (a.z * b.z)','(a.x * b.x) + (a.y * b.y) - (a.z * b.z)')]:
            assert old in original_typed;mut=write('mutant_'+label+'.c',original_typed.replace(old,new));obj=out/(label+'.o');run('compile_'+label,[compiler,*inc,*physical,'-c',mut,'-o',obj]);binary=out/label;run('link_'+label,['clang',obj,out/'main-clang.o','-o',binary]);text=run('run_'+label,[binary]);rejected=False
            try:oracle(text)
            except AssertionError:rejected=True
            assert rejected,'independent numeric oracle failed to detect orientation/dot mutant'
        manifest=dict(physical_policy=str(policy),dependencies=list(map(str,[policy,out/"catalog.json",head,impl,main,ROOT/"include/fisics/extensions.h"])),translation_units=[dict(id='vector_helpers',source=str(impl),args=inc)],strict_functions=[dict(id=fn,source=str(impl),function=fn,args=inc,physical=True,permissions=[] if fn=='matrix_copy' else ['declared-boundaries'],expected_checked=reports[fn]['checked'],expected_permitted_trust=reports[fn]['permitted_trust']) for fn in reports]);write('performance.json',manifest)
        result.update(passed=True,reports=reports,numerical_values_per_variant=numerical,abi_variants=6,numerical_bodies_unchanged=True,rank2_scope='copied axes member storage adapter; no basis/frame validation',mutants_rejected=['cross_sign','dot_term'])
    finally:
        assert {k:sha(p) for k,p in sources.items()}==pins,'owner drift'
        write('result.json',result)
def main():
    p=argparse.ArgumentParser(description=__doc__)
    for n in ['compiler','out','math-root','structural-root']:p.add_argument('--'+n,type=Path,required=True)
    a=p.parse_args();a.out.mkdir(parents=True,exist_ok=False);check(a.compiler.resolve(),a.out.resolve(),a.math_root.resolve(),a.structural_root.resolve())
if __name__=='__main__':main()
