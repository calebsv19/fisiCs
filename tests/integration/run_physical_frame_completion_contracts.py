#!/usr/bin/env python3
"""P6.3 point/origin, exact v6 receipts and cross-TU failure controls."""
import argparse,copy,hashlib,json,os,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];FIX=ROOT/'tests/fixtures/physical_semantics/frames'
REP='rep.record.double.r1.n3.polar'
def model():
    p=json.loads((FIX/'policy.json').read_text())
    p['frame_transforms'] += [dict(id='transform.'+src+'_to_'+dst+'.point',source_frame='frame.'+src,target_frame='frame.'+dst,operation='map_point',input_kind='quantity.position',output_kind='quantity.position',input_representation=REP,output_representation=REP,provenance='declared_implementation_and_basis') for src,dst in [('member_a','world'),('world','member_a')]]
    for op,kinds in [('subtract',['position','position','displacement']),('add',['position','displacement','position']),('subtract',['position','displacement','position']),('add',['displacement','position','position'])]:
        p['shape_helpers'].append(dict(id='helper.'+op+'.'+'.'.join(kinds),operation=op,operand_kinds=['quantity.'+k for k in kinds[:2]],result_kind='quantity.'+kinds[2],operand_representations=[REP,REP],result_representation=REP,preconditions='common_cartesian_basis_right_handed',provenance='declared_implementation_and_basis'))
    torque=copy.deepcopy(p['frame_transforms'][0]);torque.update(id='transform.torque',input_kind='quantity.torque',output_kind='quantity.torque',input_representation=REP.replace('polar','axial'),output_representation=REP.replace('polar','axial'));p['frame_transforms'].append(torque)
    displacement=copy.deepcopy(p['frame_transforms'][0]);displacement.update(id='transform.displacement',input_kind='quantity.displacement',output_kind='quantity.displacement');p['frame_transforms'].append(displacement)
    return p
def attr(k='position',frame='world',subject=None,extra=''):
    dim=dict(position='length',displacement='length',force='force',torque='energy')[k]
    a=f'[[fisics::dim({dim})]] [[fisics::quantity_kind("quantity.{k}")]] [[fisics::representation("{REP if k!="torque" else REP.replace("polar","axial")}")]] '
    if k=='position':a+=f'[[fisics::physical_origin("origin.{frame}")]] '
    if frame:a+=f'[[fisics::frame("frame.{frame}")]] '
    if subject:a+=f'[[fisics::point_id("point.{subject}")]] '
    return a+extra
PRE='typedef struct V {double x,y,z;} V;\n'
def check(compiler,probe,out,energy=False):
    commands=[];cases=[];env={**os.environ,'FISICS_MAX_PROCS':'0','PYTHONDONTWRITEBYTECODE':'1'};env.pop('FISICS_OVERLAY',None);env.pop('DISABLE_CODEGEN',None)
    def write(n,x):p=out/n;p.write_text(x if isinstance(x,str) else json.dumps(x,indent=2)+'\n');return p
    def run(n,args,code=0,environment=None):
        p=subprocess.run(list(map(str,args)),cwd=ROOT,env=environment or env,capture_output=True,text=True,timeout=120);write(n+'.log',p.stdout+p.stderr);commands.append(dict(id=n,command=list(map(str,args)),exit=p.returncode,expected=code));assert p.returncode==code,(n,p.returncode,(p.stdout+p.stderr)[-4000:]);return p.stdout
    pol=model()
    if energy:
        pol.update(schema_version=7,energy_forms=['energy.mixed','energy.thermal'],energy_transfers=[])
        pol['linear_kinds'].append('quantity.energy')
    write('catalog.json',json.loads((FIX/'catalog.json').read_text()));policy=write('policy.json',pol);flags=['--overlay=physics-semantics','--physical-semantics-policy',policy]
    def helper(op,kinds,left=None,right=None,result=None):
        return f'V combine(V a {left or attr(kinds[0])},V b {right or attr(kinds[1])}) {result or attr(kinds[2])} [[fisics::physical_operation("helper.{op}.{".".join(kinds)}")]];'
    def mapping(src='member_a',dst='world',extra=''):
        return f'V map_point(V a {attr(frame=src,extra=extra)}) {attr(frame=dst,extra=extra)} [[fisics::frame_transform("transform.{src}_to_{dst}.point")]];'
    def case(name,text,error=0,accepted=True,permissions=(0,1)):
        src=write(name+'.c',PRE+text+'\n');diag=out/(name+'.diagnostics.json');receipt=out/(name+'.json')
        run(name+'_source',[compiler,*flags,'--dump-sema','--emit-diags-json',diag,src],1 if error else 0)
        if error:assert any(d['code_id']==error for d in json.loads(diag.read_text())['diagnostics']),name
        native=run(name+'_native',[probe,policy,src,error,int(accepted),*permissions]);data=json.loads(next(l for l in native.splitlines() if l.startswith('{')))
        args=[compiler,*flags,'--physical-semantics-function','check','--physical-semantics-strict','--emit-physical-semantics-json',receipt]
        if permissions[0]:args+=['--physical-semantics-allow-literal-assumptions']
        if permissions[1]:args+=['--physical-semantics-allow-declared-boundaries']
        run(name+'_receipt',args+[src],1 if error or not accepted else 0)
        d=json.loads(receipt.read_text());assert len(d)==(18 if energy else 17) and d['schema_version']==(7 if energy else 6) and d['selection']['accepted']==(not error and accepted)
        assert 'frame_model' in d and all('physical_context' in e['fact'] and 'frames' not in e['fact'] for e in d['expressions'])
        cases.append(dict(id=name,error=error,accepted=not error and accepted,selection=d['selection']));return src,d
    try:
        pw=attr();pl=attr(frame='member_a');dw=attr('displacement')
        case('point_copy',f'V check(V a {pw}) {pw} {{a.x=a.x;return a;}}',permissions=(0,0))
        field_attrs=pw.replace('[[fisics::representation("'+REP+'")]]','')
        fields=''.join('double '+axis+' '+field_attrs+f' [[fisics::component_axis("axis.{axis}")]];' for axis in 'xyz')
        typed='typedef struct PointFields {'+fields+'} PointFields;'
        case('annotated_point_fields',typed+f'PointFields check(PointFields a {pw}) {pw} {{a.x=a.x;return a;}}',permissions=(0,0))
        case('wrong_point_field_origin',typed.replace('origin.world','origin.member_a',1)+f'PointFields check(PointFields a {pw}) {pw} {{return a;}}',4312,False,(1,1))
        ref='[[fisics::point_id("point.member_a_load")]]'
        h=mapping(extra=ref)
        _,mapped=case('point_map',h+f'V check(V a {attr(frame="member_a",extra=ref)}) {attr(extra=ref)} {{a.x=a.x;return map_point(a);}}')
        assert any(e['fact']['origin']=='origin.world' and e['fact']['physical_context']['point_subject']=='point.member_a_load' and e['trust_mask']&2 for e in mapped['expressions'])
        case('point_map_reverse',mapping('world','member_a')+f'V check(V a {pw}) {pl} {{a.x=a.x;return map_point(a);}}')
        case('point_map_wrong_source',h+f'V check(V a {attr(extra=ref)}) {attr(extra=ref)} {{return map_point(a);}}',4318,False)
        case('vector_as_point',mapping()+f'V check(V a {attr("displacement","member_a")}) {pw} {{return map_point(a);}}',4316,False)
        rotate=f'V rotate(V a {attr("displacement","member_a")}) {attr("displacement")} [[fisics::frame_transform("transform.displacement")]];'
        case('point_as_vector',rotate+f'V check(V a {pl}) {attr("displacement")} {{return rotate(a);}}',4316,False)
        for name in ['point_as_vector','vector_as_point']:
            assert not any(d['code_id']==4112 for d in json.loads((out/(name+'.diagnostics.json')).read_text())['diagnostics']), 'same-Dim8 semantic refusal must not rely on a dimensional mismatch'
        removed_source=(out/'point_map.c').read_text()[len(PRE):].replace('[[fisics::frame_transform("transform.member_a_to_world.point")]]','')
        _,unmapped=case('transform_metadata_removed',removed_source,4317,False,(1,1))
        assert unmapped['selection']['total']==mapped['selection']['total']
        # Record affine helper results use actual point subjects, not destination annotations.
        for op,kinds in [('subtract',['position','position','displacement']),('add',['position','displacement','position']),('subtract',['position','displacement','position']),('add',['displacement','position','position'])]:
            head=helper(op,kinds);case('affine_'+op+'_'+kinds[0]+'_'+kinds[1],head+f'V check(V a {attr(kinds[0])},V b {attr(kinds[1])}) {attr(kinds[2])} {{a.x=a.x;return combine(a,b);}}')
        head=helper('subtract',['position','position','displacement'])
        between='[[fisics::between_points("point.world_zero","point.member_a_load")]]'
        _,sub=case('named_point_subtract',head+f'V check(V a {attr(subject="member_a_load")},V b {attr(subject="world_zero")}) {attr("displacement",extra=between)} {{a.x=a.x;return combine(a,b);}}')
        assert any(e['fact']['physical_context']['displacement_from']=='point.world_zero' and e['fact']['physical_context']['displacement_to']=='point.member_a_load' for e in sub['expressions'])
        case('unknown_endpoints',head+f'V check(V a {pw},V b {pw}) {attr("displacement",extra=between)} {{return combine(a,b);}}',4319,False,(1,1))
        case('point_mixed_frame',head+f'V check(V a {pw},V b {pl}) {dw} {{return combine(a,b);}}',4318,False,(1,1))
        shift=helper('add',['position','displacement','position'])
        case('shift_clears_subject',shift+f'V check(V a {attr(subject="member_a_load")},V b {dw}) {attr(subject="member_a_load")} {{return combine(a,b);}}',4319,False,(1,1))
        case('wrong_origin',f'V check(V a {pw.replace("origin.world","origin.member_a")}) {pw} {{return a;}}',4312,False,(1,1))
        case('point_scalar_laundering',f'V check(V a {pl}) {pw} {{double x=a.x,y=a.y,z=a.z;V b {pw}={{x,y,z}};return b;}}',4319,False,(1,1))
        # Same-point wrench components retain separate physical contracts.
        moment='[[fisics::moment_reference("point.world_zero")]]'
        force_at='[[fisics::applied_at("point.world_zero")]]'
        torque_local=attr('torque','member_a',extra=moment);torque_world=attr('torque',extra=moment)
        force_local=attr('force','member_a',extra=force_at);force_world=attr('force',extra=force_at)
        wr=f'V rf(V a {force_local}) {force_world} [[fisics::frame_transform("transform.member_a_to_world.force")]];'
        # Add the explicit axial transform to this case policy, never infer it.
        wr+=f'V rt(V a {torque_local}) {torque_world} [[fisics::frame_transform("transform.torque")]];'
        case('same_point_wrench',wr+f'V check(V f {force_local},V t {torque_local}) {torque_world} {{f.x=f.x;V out {force_world}=rf(f);return rt(t);}}')
        case('moment_relabel',wr+f'V check(V t {torque_local}) {attr("torque",extra=moment.replace("point.world_zero","point.member_a_zero"))} {{return rt(t);}}',4317,False,(1,1))
        case('missing_moment',wr+f'V check(V t {attr("torque","member_a")}) {torque_world} {{return rt(t);}}',4319,False,(1,1))
        case('packed_wrench',f'void check(double a[6] {attr("force")}){{a[0]=a[0];}}',4312,False,(1,1))
        case('float_point','typedef struct F {float x,y,z;} F;'+f'F check(F a {pw.replace("record.double","record.float")}) {pw.replace("record.double","record.float")} {{return a;}}',4312,False,(1,1))
        case('missing_permission',mapping()+f'V check(V a {pl}) {pw} {{a.x=a.x;return map_point(a);}}',accepted=False,permissions=(1,0))
        case('trust_only_scope',f'V source(void) {pw}; V check(void) {pw} {{return source();}}',accepted=False,permissions=(1,1))
        invalid=copy.deepcopy(pol);bad_helper=copy.deepcopy(invalid['shape_helpers'][1]);bad_helper.update(id='helper.invalid_point_plus_point',operand_kinds=['quantity.position','quantity.position']);invalid['shape_helpers'].append(bad_helper);badpol=write('bad_point_plus_point_policy.json',invalid)
        run('point_plus_point_rejected',[compiler,'--overlay=physics-semantics','--physical-semantics-policy',badpol,out/'point_copy.c'],1)
        _,comparison=case('frame_invariant_control',f'_Bool check(V a {attr("force")},V b {attr("force")}) {{a.x=a.x;return a.x==b.x;}}',permissions=(0,0))
        assert any(e['fact']['physical_context']['facets']['frame_identity']=='not_applicable' and e['fact']['physical_context']['facets']['geometry']=='not_applicable' for e in comparison['expressions'])
        # Fresh shared signatures include direction and all context facets.
        shared=mapping()+f'V check(V a {pl}) {pw} {{a.x=a.x;return map_point(a);}}'
        _,one=case('tu_a',shared);_,two=case('tu_b',shared)
        def cross(name,a,b,code=0):return run(name,[compiler,'--physical-semantics-check',a,b],code)
        cross('shared_fresh',out/'tu_a.json',out/'tu_b.json')
        for name,mut in [
            ('schema_mix',lambda d:d.update(schema_version=5)),
            ('frame_fingerprint',lambda d:d['frame_model'].update(fingerprint='0'*16)),
            ('transform_fingerprint',lambda d:d['frame_model']['frame_transforms'][0].update(fingerprint='0'*16)),
            ('unknown_frame',lambda d:d['declarations'][0]['slots'][0]['fact']['physical_context'].update(frame='frame.unknown')),
            ('facet_claim',lambda d:d['declarations'][0]['slots'][0]['fact']['physical_context']['facets'].update(geometry='proved')),
            ('extra_context',lambda d:d['declarations'][0]['slots'][0]['fact']['physical_context'].update(matrix=[])),
            ('direction',lambda d:d['declarations'][1].update(frame_transform='transform.world_to_member_a.point')),
            ('counter',lambda d:d['selection'].update(checked=999)),
            ('missing_model',lambda d:d.pop('frame_model'))]:
            bad=copy.deepcopy(two);mut(bad);cross('tamper_'+name,out/'tu_a.json',write('bad_'+name+'.json',bad),1)
        # Shared ordinary same-frame signatures cannot disagree about instances.
        bad=copy.deepcopy(two);bad['declarations'][0]['slots'][0]['fact']['physical_context']['frame']='frame.member_a';cross('shared_frame_mismatch',out/'tu_a.json',write('bad_shared.json',bad),1)
        original=(out/'tu_b.c').read_text();(out/'tu_b.c').write_text(original+'\n');cross('stale_source',out/'tu_a.json',out/'tu_b.json',1);(out/'tu_b.c').write_text(original)
        original=policy.read_text();policy.write_text(original+'\n');cross('stale_policy',out/'tu_a.json',out/'tu_b.json',1);policy.write_text(original)
        preserved=write('preserve.json','preserve');args=[compiler,*flags,'--physical-semantics-function','check','--emit-physical-semantics-json',preserved,out/'point_copy.c']
        run('atomic_write_failure',args,1,{**env,'FISICS_TEST_FAIL_PHYSICAL_RECEIPT_WRITE':'1'});assert preserved.read_text()=='preserve'
        run('alias_source',[compiler,*flags,'--physical-semantics-function','check','--emit-physical-semantics-json',out/'point_copy.c',out/'point_copy.c'],1)
        # Metadata removal retains the AST obligation denominator.
        original=PRE+f'V check(V a {attr("force")}) {attr("force")} {{a.x=a.x;return a;}}'
        _,present=case('metadata_present',original[len(PRE):],permissions=(0,0))
        _,removed=case('metadata_removed',original[len(PRE):].replace('[[fisics::frame("frame.world")]]',''),4319,False,(1,1))
        assert present['selection']['total']==removed['selection']['total']
        assert any(e['fact']['representation']==REP and e['fact']['physical_context']['facets']['frame_identity']=='unknown' for e in removed['expressions'])
        run('point_allocation_sweep',[probe,policy,out/'named_point_subtract.c','sweep'])
        result=dict(passed=True,compiler_sha256=hashlib.sha256(compiler.read_bytes()).hexdigest(),cases=cases,commands=commands)
    except BaseException:
        write('result.json',dict(passed=False,cases=cases,commands=commands));raise
    write('result.json',result);print(f'frame completion PASS ({len(cases)} source/native/receipt cases; {len(commands)} commands)')
def main():
    p=argparse.ArgumentParser(description=__doc__)
    for n in ['compiler','probe']:p.add_argument('--'+n,type=Path,required=True)
    p.add_argument('--out',type=Path);p.add_argument('--energy',action='store_true',help='Qualify frame contracts under the additive v7 energy model');a=p.parse_args()
    if a.out:a.out.mkdir(parents=True,exist_ok=False);check(a.compiler.resolve(),a.probe.resolve(),a.out.resolve(),a.energy)
    else:
        with tempfile.TemporaryDirectory(prefix='fisics-frame-completion-') as t:check(a.compiler.resolve(),a.probe.resolve(),Path(t),a.energy)
if __name__=='__main__':main()
