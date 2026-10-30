#!/usr/bin/env python3
"""Fixed-direction rotation static contracts; numeric geometry remains trust."""
import argparse,copy,hashlib,json,os,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
FIX=ROOT/'tests/fixtures/physical_semantics/frames'
def check(compiler,probe,out):
    commands=[];cases=[]
    env={**os.environ,'FISICS_MAX_PROCS':'0','PYTHONDONTWRITEBYTECODE':'1'}
    env.pop('FISICS_OVERLAY',None);env.pop('DISABLE_CODEGEN',None)
    def write(n,s):
        p=out/n;p.write_text(s if isinstance(s,str) else json.dumps(s,indent=2)+'\n');return p
    def run(n,args,code=0):
        p=subprocess.run(list(map(str,args)),cwd=ROOT,env=env,capture_output=True,text=True,timeout=120)
        write(n+'.log',p.stdout+p.stderr);commands.append(dict(id=n,command=list(map(str,args)),exit=p.returncode,expected=code))
        assert p.returncode==code,(n,p.returncode,(p.stdout+p.stderr)[-5000:]);return p.stdout
    model=json.loads((FIX/'policy.json').read_text());write('catalog.json',json.loads((FIX/'catalog.json').read_text()))
    model['frames'].append(dict(model['frames'][0],id='frame.member_b',affine_origin=None))
    model['frame_transforms']=[]
    for kind in ['force','displacement','torque']:
        for source,target in [('member_a','world'),('world','member_a'),('world','world')]:
            model['frame_transforms'].append(dict(id=f'transform.{source}_to_{target}.{kind}',source_frame='frame.'+source,target_frame='frame.'+target,operation='rotate_vector',input_kind='quantity.'+kind,output_kind='quantity.'+kind,input_representation='rep.record.double.r1.n3.'+('axial' if kind=='torque' else 'polar'),output_representation='rep.record.double.r1.n3.'+('axial' if kind=='torque' else 'polar'),provenance='declared_implementation_and_basis'))
    policy=write('policy.json',model)
    def attr(k='force',frame='world',extra='',representation=None):
        dim=dict(force='force',displacement='length',torque='energy')[k]
        rep=representation or 'rep.record.double.r1.n3.'+('axial' if k=='torque' else 'polar')
        return f'[[fisics::dim({dim})]] [[fisics::quantity_kind("quantity.{k}")]] [[fisics::representation("{rep}")]] '+(f'[[fisics::frame("frame.{frame}")]] ' if frame else '')+extra
    pre='typedef struct V {double x,y,z;} V;\ntypedef struct F {float x,y,z;} F;\n'
    def decl(k='force',source='member_a',target='world',extra='',name='rotate'):
        return f'V {name}(V a {attr(k,source,extra)}) {attr(k,target,extra)} [[fisics::frame_transform("transform.{source}_to_{target}.{k}")]];'
    def case(name,body,error=0,accepted=True,permissions=(0,1),inspect=None):
        body=body.replace('{return','{a.x=a.x;return') if 'V check(V a' in body else body
        source=write(name+'.c',pre+body+'\n');diag=out/(name+'.diagnostics.json')
        run(name+'_cli',[compiler,'--overlay=physics-semantics','--physical-semantics-policy',policy,'--dump-sema','--emit-diags-json',diag,source],1 if error else 0)
        if error:assert any(d['code_id']==error for d in json.loads(diag.read_text())['diagnostics']),name
        data=json.loads(next(x for x in run(name+'_native',[probe,policy,source,error,int(accepted),*permissions]).splitlines() if x.startswith('{')))
        if inspect:inspect(data)
        cases.append(dict(id=name,error=error,accepted=accepted,coverage={k:v for k,v in data.items() if k!='facts'}));return source
    def has(d,**wanted):assert any(all(f[k]==v for k,v in wanted.items()) for f in d['facts']),(wanted,d)
    try:
        for k in ['force','displacement','torque']:
            ref='[[fisics::moment_reference("point.world_zero")]]' if k=='torque' else ''
            for source,target in [('member_a','world'),('world','member_a'),('world','world')]:
                h=decl(k,source,target,ref)
                case(k+'_'+source+'_to_'+target,h+f'V check(V a {attr(k,source,ref)}) {attr(k,target,ref)} {{return rotate(a);}}',inspect=lambda d,k=k,target=target,source=source:has(d,kind='quantity.'+k,frame='frame.'+target,state=3,flags=1,rule=f'transform.{source}_to_{target}.{k}'))
        h=decl();a=attr(frame='member_a');b=attr()
        case('trust_permission_required',h+f'V check(V a {a}) {b} {{return rotate(a);}}',accepted=False,permissions=(1,0))
        case('wrong_source',h+f'V check(V a {b}) {b} {{return rotate(a);}}',4317,False,(1,1))
        case('wrong_instance',h+f'V check(V a {attr(frame="member_b")}) {b} {{return rotate(a);}}',4317,False,(1,1))
        case('wrong_target',h+f'V check(V a {a}) {a} {{return rotate(a);}}',4317,False,(1,1))
        case('wrong_kind',h+f'V check(V a {attr("displacement","member_a")}) {b} {{return rotate(a);}}',4316,False,(1,1))
        case('wrong_representation',h+f'V check(V a {attr("torque","member_a")}) {b} {{return rotate(a);}}',4316,False,(1,1))
        for lit,trust in [(0,0),(0,1),(1,0),(1,1)]:
            case(f'unknown_input_{lit}_{trust}',h+f'V check(V a {attr(frame=None)}) {b} {{return rotate(a);}}',4319,False,(lit,trust))
        case('unannotated_name',f'V rotate(V a {a}) {b}; V check(V a {a}) {b} {{return rotate(a);}}',4317,False,(1,1))
        case('callback',h+f'V check(V a {a}) {b} {{V (*p)(V)=rotate;return p(a);}}',4319,False,(1,1))
        case('raw_pointer',h+f'V check(void *p) {b} {{return rotate(*((V*)p));}}',4319,False,(1,1))
        for label,new in [('runtime_direction',h.replace('V a '+a,'V a '+a+',int to_world')),('mutable_basis',h.replace('V a '+a,'V a '+a+',double *basis')),('pointer_result',h.replace('V rotate','V *rotate')),('pointer_input',h.replace('V a','V *a')),('wrong_declared_source',h.replace('V a '+a,'V a '+b)),('unknown_identity',h.replace('transform.member_a_to_world.force','transform.unknown'))]:
            case(label,new+f'void check(V a {a}){{a.x=a.x;}}',4312,True,(1,1))
        case('float_signature',h.replace('V rotate(V a','F rotate(F a')+f'void check(V a {a}){{a.x=a.x;}}',4312,True,(1,1))
        case('direction_mismatch',h.replace('transform.member_a_to_world.force','transform.world_to_member_a.force')+f'void check(V a {a}){{a.x=a.x;}}',4312,True,(1,1))
        # Required reference checks cannot be waived; rotation keeps optional references too.
        for k,ref in [('force','[[fisics::applied_at("point.member_a_load")]]'),('displacement','[[fisics::between_points("point.world_zero","point.member_a_load")]]'),('torque','[[fisics::moment_reference("point.world_zero")]]')]:
            hh=decl(k,extra=ref)
            case('reference_'+k,hh+f'V check(V a {attr(k,"member_a",ref)}) {attr(k,"world",ref)} {{return rotate(a);}}')
            case('missing_reference_'+k,hh+f'V check(V a {attr(k,"member_a")}) {attr(k,"world",ref)} {{return rotate(a);}}',4319,False,(1,1))
            case('optional_reference_'+k,decl(k)+f'V check(V a {attr(k,"member_a",ref)}) {attr(k,"world",ref)} {{return rotate(a);}}')
        m='[[fisics::moment_reference("point.world_zero")]]';other='[[fisics::moment_reference("point.member_a_zero")]]'
        case('moment_rebase',decl('torque',extra=m).replace(attr('torque','world',m),attr('torque','world',other))+f'void check(V a {a}){{a.x=a.x;}}',4312,True,(1,1))
        # Transform dependencies carry trust through nested operations and inverse calls.
        case('round_trip',h+decl(source='world',target='member_a',name='inverse')+f'V check(V a {a}) {a} {{return inverse(rotate(a));}}')
        alloc=case('allocation_path',h+f'V check(V a {a}) {b} {{V q {b}=rotate(a);q.x=q.x;return q;}}')
        run('allocation_sweep',[probe,policy,alloc,'sweep'])
        result=dict(passed=True,compiler_sha256=hashlib.sha256(compiler.read_bytes()).hexdigest(),cases=cases,commands=commands,scope='fixed-direction 3D double static rotation; implementation and basis remain declared trust; full v6 receipts deferred')
    except BaseException:
        write('result.json',dict(passed=False,cases=cases,commands=commands));raise
    write('result.json',result);print(f'physical rotation contracts: PASS ({len(cases)} cases, {len(commands)} commands)')
def main():
    p=argparse.ArgumentParser(description=__doc__)
    for n in ['compiler','probe']:p.add_argument('--'+n,type=Path,required=True)
    p.add_argument('--out',type=Path);a=p.parse_args()
    if a.out:a.out.mkdir(parents=True,exist_ok=False);check(a.compiler.resolve(),a.probe.resolve(),a.out.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix='fisics-rotations-') as d:check(a.compiler.resolve(),a.probe.resolve(),Path(d))
if __name__=='__main__':main()
