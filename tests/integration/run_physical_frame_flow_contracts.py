#!/usr/bin/env python3
"""P6.3.3 static frame/axis propagation, enforcement and laundering controls."""
import argparse,copy,hashlib,json,os,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
FIX=ROOT/'tests/fixtures/physical_semantics'
def check(compiler,probe,out):
    commands=[];cases=[];env={**os.environ,'FISICS_MAX_PROCS':'0','PYTHONDONTWRITEBYTECODE':'1'};env.pop('FISICS_OVERLAY',None);env.pop('DISABLE_CODEGEN',None)
    def write(name,x):
        p=out/name;p.write_text(x if isinstance(x,str) else json.dumps(x,indent=2)+'\n');return p
    def run(name,args,expected=0):
        x=subprocess.run(list(map(str,args)),cwd=ROOT,env=env,capture_output=True,text=True,timeout=120);log=x.stdout+x.stderr;write(name+'.log',log)
        commands.append(dict(id=name,command=list(map(str,args)),exit=x.returncode,expected=expected));assert x.returncode==expected,(name,x.returncode,log[-5500:]);return x.stdout
    model=json.loads((FIX/'frames/policy.json').read_text());write('catalog.json',json.loads((FIX/'frames/catalog.json').read_text()))
    model['linear_kinds'].append('quantity.energy');model['shape_helpers']=json.loads((FIX/'representations/policy.json').read_text())['shape_helpers']
    model['frames'].append(dict(id='frame.member_b',context_id=model['frame_contexts'][0]['id'],dimension=3,convention='cartesian_orthonormal_right_handed',origin_point='point.member_a_zero',affine_origin=None))
    model['frames'].append(dict(model['frames'][0],id='frame.planar',dimension=2,affine_origin=None))
    for kind in ['force','displacement','torque','energy']:
        role='difference' if kind=='displacement' else 'linear'
        for operation,left,right in [('multiply',kind,'ratio'),('multiply','ratio',kind),('divide',kind,'ratio')]:
            model['operation_rules'].append(dict(id=f'rule.{operation}.{left}.{right}',operation=operation,operand_kinds=['quantity.'+left,'quantity.'+right],result_kind='quantity.'+kind,reference_role=role,origin_policy='none',provenance='inputs'))
        for operation in ['fabs','fmax']:
            model['operation_rules'].append(dict(id=f'rule.{operation}.{kind}',operation=operation,operand_kinds=['quantity.'+kind]*(1 if operation=='fabs' else 2),result_kind='quantity.'+kind,reference_role=role,origin_policy='preserve' if operation=='fabs' else 'same',provenance='inputs'))
    policy=write('policy.json',model)
    def attr(kind='force',frame='world',axis=None,rep=None,extra=''):
        dim={'force':'force','displacement':'length','torque':'energy','energy':'energy','ratio':'1'}[kind]
        a=f'[[fisics::dim({dim})]] [[fisics::quantity_kind("quantity.{kind}")]]'
        if rep:a+=f' [[fisics::representation("{rep}")]]'
        if frame:a+=f' [[fisics::frame("frame.{frame}")]]'
        if axis:a+=f' [[fisics::component_axis("axis.{axis}")]]'
        return a+' '+extra
    def rep(prec='double',extent=3,storage='record',parity='polar',rank=1):return f'rep.{storage}.{prec}.r{rank}.n{extent}.{parity if rank==1 else "tensor"}'
    pre=''.join(f'typedef struct V{n}{prec[0]} {{{prec} '+','.join('xyz'[:n])+f';}} V{n}{prec[0]};\n' for prec in ['float','double'] for n in [2,3])
    w=attr(rep=rep());l=attr(frame='member_a',rep=rep());other=attr(frame='member_b',rep=rep());sx=attr(axis='x');sy=attr(axis='y');ratio=attr('ratio',None)
    def case(name,body,error=0,accepted=True,permissions=(0,0),inspect=None):
        src=write(name+'.c',pre+body+'\n');diag=out/(name+'.diagnostics.json')
        run(name+'_cli',[compiler,'--overlay=physics-semantics','--physical-semantics-policy',policy,'--dump-sema','--emit-diags-json',diag,src],1 if error else 0)
        if error:assert any(d['code_id']==error for d in json.loads(diag.read_text())['diagnostics']),(name,json.loads(diag.read_text()))
        native=run(name+'_native',[probe,policy,src,error,int(accepted),*permissions]);data=json.loads(next(x for x in native.splitlines() if x.startswith('{')))
        if inspect:inspect(data)
        cases.append(dict(id=name,source=str(src),error=error,accepted=accepted,coverage={k:v for k,v in data.items() if k!='facts'}));return src,data
    def has(data,**wanted):assert any(all(f[k]==v for k,v in wanted.items()) for f in data['facts']),(wanted,data)
    try:
        for prec in ['float','double']:
            for n in [2,3]:
                aa=attr(frame='planar' if n==2 else 'world',rep=rep(prec,n));ty=f'V{n}{prec[0]}'
                case(f'record_copy_{prec}_{n}',f'{ty} check({ty} a {aa}) {aa} {{ {ty} b {aa}=a; b.x=a.x; return b; }}')
                case(f'array_components_{prec}_{n}',f'{prec} a[{n}] {attr(frame="planar" if n==2 else "world",rep=rep(prec,n,"array"))}; void check(void){{a[0]=a[0];}}')
        case('axis_propagation',f'double check(V3d a {w}) {sx} {{double q {sx}=a.x; return q;}}',inspect=lambda d:has(d,name='x',frame='frame.world',axis=1))
        case('mixed_frame_copy',f'V3d check(V3d a {l}) {w} {{return a;}}',4317,False,permissions=(1,1))
        case('wrong_member_instance',f'V3d check(V3d a {l}) {other} {{return a;}}',4317,False,permissions=(1,1))
        case('wrong_axis',f'double check(V3d a {w}) {sy} {{return a.x;}}',4317,False)
        case('component_write',f'void check(V3d a {w},V3d b {l}){{a.x=b.x;}}',4317,False)
        case('axis_write',f'void check(V3d a {w}){{a.y=a.x;}}',4317,False)
        case('mixed_component_construction',f'V3d check(V3d a {w},V3d b {l}) {w} {{V3d v {w}={{a.x,b.y,a.z}};return v;}}',4317,False,(1,1))
        case('swapped_component_construction',f'V3d check(V3d a {w}) {w} {{V3d v {w}={{a.y,a.x,a.z}};return v;}}',4317,False,(1,1))
        case('component_construction',f'V3d check(V3d a {w}) {w} {{V3d v {w}={{a.x,a.y,a.z}};return v;}}')
        case('array_construction',f'void check(V3d a {w}){{double v[3] {attr(rep=rep(storage="array"))}={{a.x,a.y,a.z}};v[0]=a.x;}}')
        case('scalar_erasure',f'void check(V3d a {w}){{double tmp=a.x;}}',4319,False,(1,1))
        case('kind_only_erasure',f'void check(V3d a {w}){{double tmp {attr(frame=None)}=a.x;}}',4319,False,(1,1))
        case('scalar_launder_rebuild',f'V3d check(V3d a {l}) {w} {{double x=a.x,y=a.y,z=a.z;V3d b {w}={{x,y,z}};return b;}}',4319,False,(1,1))
        case('unframed_scalar_to_component',f'double check(double a {attr(frame=None)}) {sx} {{return a;}}',4319,False,(1,1))
        case('missing_vector_frame',f'V3d check(V3d a {attr(rep=rep(),frame=None)}) {w} {{return a;}}',4319,False,(1,1))
        case('missing_quantity',f'void check(V3d a){{V3d b {w}=a;}}',4319,False,(1,1))
        case('unknown_dynamic_axis',f'double check(V3d a {w},int i) {sx} {{double v[3] {attr(rep=rep(storage="array"))}={{a.x,a.y,a.z}};return v[i];}}',4319,False,(1,1))
        case('unknown_pointer_alias',f'double check(V3d a {w}) {sx} {{V3d *p=&a;return p->x;}}',4319,False,(1,1))
        case('raw_cast',f'double check(void *p) {sx} {{return ((V3d*)p)->x;}}',4319,False,(1,1))
        case('scalar_cast_preserves',f'double check(V3d a {w}) {sx} {{return (double)a.x;}}')
        case('unary_preserves',f'double check(V3d a {w}) {sx} {{return -a.x;}}')
        case('component_add',f'double check(V3d a {w},V3d b {w}) {sx} {{return a.x+b.x;}}')
        case('component_axis_add',f'double check(V3d a {w}) {sx} {{return a.x+a.y;}}',4317,False)
        case('component_frame_add',f'double check(V3d a {w},V3d b {l}) {sx} {{return a.x+b.x;}}',4317,False)
        case('comparison_frame',f'int check(V3d a {w},V3d b {l}){{return a.x==b.x;}}',4317,False)
        case('conditional_match',f'double check(V3d a {w},V3d b {w},int choose) {sx} {{return choose?a.x:b.x;}}')
        case('conditional_mismatch',f'double check(V3d a {w},V3d b {l},int choose) {sx} {{return choose?a.x:b.x;}}',4317,False)
        for op in ['a.x*r','r*a.x','a.x/r']:
            case('ratio_'+str(len(cases)),f'double check(V3d a {w},double r {ratio}) {sx} {{return {op};}}')
        case('compound_ratio',f'double check(V3d a {w},double r {ratio}) {sx} {{double x {sx}=a.x;x*=r;x/=r;return x;}}')
        case('bare_vector_no_obligation',f'void check(V3d a {attr(frame=None,rep=rep())},V3d b {w}){{b.x=b.x;a;}}',accepted=True,permissions=(1,1))
        case('discarded_call_missing_frame',f'V3d source(void) {attr(frame=None,rep=rep())}; void check(V3d a {w}){{a.x=a.x;source();}}',accepted=False,permissions=(1,1))
        case('both_vectors_unframed',f'V3d check(V3d a {attr(frame=None,rep=rep())}) {attr(frame=None,rep=rep())} {{return a;}}',4319,False,(1,1))
        case('array_axis_swap',f'void check(V3d a {w}){{double v[3] {attr(rep=rep(storage="array"))}={{a.y,a.x,a.z}};}}',4317,False,(1,1))
        case('rank2_copy_preserved','typedef struct M {double axes[3][3];} M;'+f'M check(M a {attr(frame=None,rep=rep(rank=2,parity="tensor"))}) {attr(frame=None,rep=rep(rank=2,parity="tensor"))} {{M b {attr(frame=None,rep=rep(rank=2,parity="tensor"))}=a;return b;}}')
        case('ordinary_scalar',f'double check(double a {attr(frame=None)},double b {attr(frame=None)}) {attr(frame=None)} {{return a+b;}}')
        case('ordinary_controls',f'double check(V3d a {w}) {sx} {{for(int i=0;i<2;i++){{a.x=a.x;}}return a.x;}}')
        # Ordinary resolved by-value interfaces retain implementation trust.
        function=f'V3d pass(V3d a {w}) {w};'
        case('call_frame_match',function+f'V3d check(V3d a {w}) {w} {{a.x=a.x;return pass(a);}}',permissions=(0,1))
        case('call_ordinary_control',f'V3d pass_n(V3d a {w},int n) {w}; V3d check(V3d a {w},int n) {w} {{a.x=a.x;return pass_n(a,n);}}',accepted=False,permissions=(0,1),inspect=lambda d:has(d,kind='quantity.force',frame='frame.world',state=3))
        case('callback_unknown',function+f'V3d check(V3d a {w}) {w} {{V3d (*p)(V3d) = pass;return p(a);}}',4319,False,(1,1))
        transform=f'V3d rotate(V3d a {l}) {w} [[fisics::frame_transform("transform.member_a_to_world.force")]];'
        case('transform_execution_admitted',transform+f'V3d check(V3d a {l}) {w} {{a.x=a.x;return rotate(a);}}',0,True,(1,1))
        case('call_trust_required',function+f'V3d check(V3d a {w}) {w} {{a.x=a.x;return pass(a);}}',accepted=False)
        case('opaque_reframe',f'V3d relabel(V3d a {l}) {w}; V3d check(V3d a {l}) {w} {{return relabel(a);}}',4317,False,(1,1))
        case('opaque_reaxis',f'double relabel(double a {sx}) {sy}; double check(double a {sx}) {sy} {{return relabel(a);}}',4317,False,(1,1))
        case('call_frame_mismatch',function+f'V3d check(V3d a {l}) {l} {{return pass(a);}}',4317,False,(1,1))
        case('unannotated_scalar_helper','double erase(double x);'+f'void check(V3d a {w}){{double x=erase(a.x);}}',4319,False,(1,1))
        case('literal_construction',f'V3d check(V3d a {w}) {w} {{V3d b {w}={{1,2,3}};a.x=a.x;return b;}}',permissions=(1,0))
        case('literal_permission_required',f'V3d check(V3d a {w}) {w} {{V3d b {w}={{1,2,3}};a.x=a.x;return b;}}',accepted=False)
        # Shape contracts are selected by resolved C helper identity.
        shape_helpers={h['operation']:h['id'] for h in model['shape_helpers'] if h['operand_representations'][0]==rep()}
        def helper(name,op,left,right,result,scalar=False):
            return f'{"double" if scalar else "V3d"} {name}(V3d a {left},{"double" if op=="scale" else "V3d"} b {right}) {result} [[fisics::physical_operation("{shape_helpers[op]}")]];'
        for op in ['add','subtract']:
            head=helper('combine',op,w,w,w)
            case('shape_'+op,head+f'V3d check(V3d a {w},V3d b {w}) {w} {{a.x=a.x;return combine(a,b);}}',permissions=(0,1))
            case('shape_'+op+'_wrong_frame',head+f'V3d check(V3d a {w},V3d b {l}) {w} {{return combine(a,b);}}',4317,False,(1,1))
        head=helper('scale','scale',w,ratio,w)
        case('shape_scale',head+f'V3d check(V3d a {w},double r {ratio}) {w} {{a.x=a.x;return scale(a,r);}}',permissions=(0,1))
        dw=attr('displacement',rep=rep());energy=attr('energy',None)
        head=helper('work','dot',w,dw,energy,True)
        case('dot_invariant',head+f'double check(V3d a {w},V3d d {dw}) {energy} {{a.x=a.x;return work(a,d);}}',permissions=(0,1),inspect=lambda d:has(d,kind='quantity.energy',frame='',flags=3))
        case('component_product_not_dot',f'double check(V3d a {w},V3d d {dw}) {energy} {{return a.x*d.x;}}',4319,False,(1,1))
        case('dot_wrong_frame',head+f'double check(V3d a {l},V3d d {dw}) {energy} {{return work(a,d);}}',4317,False,(1,1))
        at='[[fisics::applied_at("point.member_a_load")]]';between='[[fisics::between_points("point.world_zero","point.member_a_load")]]';moment='[[fisics::moment_reference("point.world_zero")]]'
        force_at=attr(rep=rep(),extra=at);lever=attr('displacement',rep=rep(),extra=between);torque=attr('torque',rep=rep(parity='axial'),extra=moment)
        head=helper('moment','cross',lever,force_at,torque)
        case('cross_moment',head+f'V3d check(V3d r {lever},V3d f {force_at}) {torque} {{r.x=r.x;return moment(r,f);}}',permissions=(0,1),inspect=lambda d:has(d,kind='quantity.torque',frame='frame.world',moment=3))
        case('cross_missing_reference',head+f'V3d check(V3d r {dw},V3d f {force_at}) {torque} {{return moment(r,f);}}',4319,False,(1,1))
        # Same frame force result cannot acquire an invented application point.
        force_other=attr(rep=rep(),extra='[[fisics::applied_at("point.world_zero")]]')
        head=helper('add','add',force_at,force_other,w)
        case('force_resultant_free',head+f'V3d check(V3d a {force_at},V3d b {force_other}) {w} {{a.x=a.x;return add(a,b);}}',permissions=(0,1),inspect=lambda d:has(d,kind='quantity.force',frame='frame.world',flags=1,applied_at=0))
        case('resultant_cannot_relabel',head+f'V3d check(V3d a {force_at},V3d b {force_other}) {force_at} {{return add(a,b);}}',4319,False,(1,1))
        other_moment=attr('torque',rep=rep(parity='axial'),extra='[[fisics::moment_reference("point.member_a_zero")]]')
        case('opaque_moment_rebase',f'V3d relabel(V3d a {torque}) {other_moment}; V3d check(V3d a {torque}) {other_moment} {{return relabel(a);}}',4317,False,(1,1))
        scalar_lever=attr('displacement',axis='x',extra=between)
        case('scale_clears_endpoints',f'double check(double x {scalar_lever},double r {ratio}) {attr("displacement",axis="x")} {{return x*r;}}',inspect=lambda d:has(d,name='*',frame='frame.world',axis=1,flags=1,**{'from':0,'to':0}))
        case('scaled_endpoints_unknown',f'double check(double x {scalar_lever},double r {ratio}) {scalar_lever} {{return x*r;}}',4319,False,(1,1))
        # Required unknowns remain failures under all trust permissions.
        for lit,declared in [(0,0),(0,1),(1,0),(1,1)]:
            case(f'unknown_permissions_{lit}_{declared}',f'void check(V3d a {w}){{double x=a.x;}}',4319,False,(lit,declared))
        aa=f'V3d check(V3d a {w}) {w} {{V3d b {w}=a;b.x=a.x;return b;}}'
        _,present=case('metadata_present',aa)
        _,removed=case('metadata_removed',aa.replace('[[fisics::frame("frame.world")]]',''),4319,False,(1,1))
        assert present['required']+present['controls']==removed['required']+removed['controls']
        # The standard-helper identity path must also enforce component contexts.
        for name,expression,error in [('fabs_component','fabs(a.x)',0),('fmax_component','fmax(a.x,a.x)',0),('fmax_wrong_axis','fmax(a.x,a.y)',4317),('fmax_wrong_frame','fmax(a.x,b.x)',4317)]:
            source=write(name+'.c','#include <math.h>\n'+pre+f'double check(V3d a {w},V3d b {l}) {sx} {{return {expression};}}\n');diag=out/(name+'.diagnostics.json')
            run(name,[compiler,'--overlay=physics-semantics','--physical-semantics-policy',policy,'--dump-sema','--emit-diags-json',diag,source],1 if error else 0)
            if error:assert any(d['code_id']==error for d in json.loads(diag.read_text())['diagnostics'])
        source=write('allocation.c' ,pre+f'V3d check(V3d a {w},double r {ratio}) {w} {{double x {sx}=a.x*r;V3d b {w}={{x,a.y,a.z}};return b;}}\n')
        run('allocation_sweep',[probe,policy,source,'sweep'])
        # V6 receipt publication preserves checked context and strict acceptance.
        dest=write('gated_receipt.json','preserve');src=Path(cases[0]['source'])
        run('receipt_published',[compiler,'--overlay=physics-semantics','--physical-semantics-policy',policy,'--physical-semantics-function','check','--physical-semantics-strict','--emit-physical-semantics-json',dest,src],0);assert json.loads(dest.read_text())['selection']['accepted']
        result=dict(passed=True,compiler_sha256=hashlib.sha256(compiler.read_bytes()).hexdigest(),cases=cases,commands=commands,scope='static frame propagation, declared directed rotation and exact v6 receipts')
    except BaseException:
        write('result.json',dict(passed=False,cases=cases,commands=commands));raise
    write('result.json',result);print(f'physical frame propagation: PASS ({len(cases)} cases, {len(commands)} commands)')
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',type=Path,required=True);p.add_argument('--probe',type=Path,required=True);p.add_argument('--out',type=Path);a=p.parse_args()
    if a.out:a.out.mkdir(parents=True,exist_ok=False);check(a.compiler.resolve(),a.probe.resolve(),a.out.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix='fisics-frame-flow-') as d:check(a.compiler.resolve(),a.probe.resolve(),Path(d))
if __name__=='__main__':main()
