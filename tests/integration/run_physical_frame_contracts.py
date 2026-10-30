#!/usr/bin/env python3
"""P6.3.2 declaration/model qualification. No frame expression acceptance claim."""
import argparse,copy,hashlib,json,os,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
FIX=ROOT/'tests/fixtures/physical_semantics/frames'
def check(compiler,probe,out):
    commands=[];env={**os.environ,'FISICS_MAX_PROCS':'0','PYTHONDONTWRITEBYTECODE':'1'}
    env.pop('FISICS_OVERLAY',None);env.pop('DISABLE_CODEGEN',None)
    def write(name,value):
        p=out/name;p.write_text(value if isinstance(value,str) else json.dumps(value,indent=2)+'\n');return p
    def run(name,args,expected=0,diagnostic=None):
        result=subprocess.run(list(map(str,args)),cwd=ROOT,env=env,capture_output=True,text=True,timeout=120)
        log=result.stdout+result.stderr;write(name+'.log',log)
        commands.append(dict(id=name,command=list(map(str,args)),exit=result.returncode,expected=expected))
        assert result.returncode==expected,(name,result.returncode,log[-3500:])
        if diagnostic:assert str(diagnostic) in log,(name,log[-3500:])
        return log
    policy=json.loads((FIX/'policy.json').read_text());write('catalog.json',json.loads((FIX/'catalog.json').read_text()));base=write('policy.json',policy)
    tables=['frame_contexts','reference_points','frames','frame_transforms']
    def model(name,mut,mode='fail',code=4303):
        p=copy.deepcopy(policy);mut(p);path=write(name+'.json',p)
        run(name,[probe,base,path,mode,*([str(code)] if mode=='fail' else [])])
    try:
        run('native_qualification',[probe,base,base,'qualify'])
        model('canonical_reorder',lambda p:[p[k].reverse() for k in tables+['representations','linear_kinds','origins']],mode='same')
        for table in tables:
            model('duplicate_'+table,lambda p,t=table:p[t].append(copy.deepcopy(p[t][0])))
            for field in policy[table][0]:
                model('missing_'+table+'_'+field,lambda p,t=table,f=field:p[t][0].pop(f))
            model('extra_'+table,lambda p,t=table:p[t][0].update(extra='unsupported'))
            model('invalid_id_'+table,lambda p,t=table:p[t][0].update(id='Bad name'))
            model('wrong_table_'+table,lambda p,t=table:p.update({t:{}}))
            model('unknown_root_'+table,lambda p,t=table:p.pop(t),code=4301)
        for table,field in [('reference_points','context_id'),('frames','context_id'),('frames','origin_point'),('frames','affine_origin'),('frame_transforms','source_frame'),('frame_transforms','target_frame'),('frame_transforms','input_kind'),('frame_transforms','output_kind'),('frame_transforms','input_representation'),('frame_transforms','output_representation')]:
            model('unknown_'+table+'_'+field,lambda p,t=table,f=field:p[t][0].update({f:'unknown.identity'}))
        mutations={
          'frame_4d':lambda p:p['frames'][0].update(dimension=4),
          'reflection':lambda p:p['frames'][0].update(convention='cartesian_left_handed'),
          'bad_provenance':lambda p:p['frame_transforms'][0].update(provenance='inferred'),
          'mixed_kind':lambda p:p['frame_transforms'][0].update(output_kind='quantity.torque'),
          'wrong_parity':lambda p:p['frame_transforms'][0].update(input_representation='rep.record.double.r1.n3.axial',output_representation='rep.record.double.r1.n3.axial'),
          'float_transform':lambda p:p['frame_transforms'][0].update(input_representation='rep.record.float.r1.n3.polar',output_representation='rep.record.float.r1.n3.polar'),
          'array_transform':lambda p:p['frame_transforms'][0].update(input_representation='rep.array.double.r1.n3.polar',output_representation='rep.array.double.r1.n3.polar'),
          'scale_transform':lambda p:p['frame_transforms'][0].update(operation='scale'),
          'missing_point_origins':lambda p:(p['frames'][0].update(affine_origin=None),p['frame_transforms'][0].update(operation='map_point',input_kind='quantity.position',output_kind='quantity.position')),
          'nonposition_affine':lambda p:(p['origins'][0].update(absolute_kind='quantity.force')),
          'two_dimensional_transform':lambda p:[f.update(dimension=2) for f in p['frames']],
          'mixed_observer_context':lambda p:(p['frame_contexts'].append(dict(id='context.other',observer_id='observer.other',state_id='state.static_a')),p['reference_points'][0].update(context_id='context.other'),p['frames'][0].update(context_id='context.other')),
          'point_context_mismatch':lambda p:(p['frame_contexts'].append(dict(id='context.other',observer_id='observer.other',state_id='state.static_a')),p['reference_points'][0].update(context_id='context.other')),
        }
        for name,mut in mutations.items():model(name,mut)
        for name,mut in {
          'observer_changes_hash':lambda p:p['frame_contexts'][0].update(observer_id='observer.changed'),
          'state_changes_hash':lambda p:p['frame_contexts'][0].update(state_id='state.changed'),
          'direction_changes_hash':lambda p:p['frame_transforms'][0].update(source_frame='frame.world',target_frame='frame.member_a'),
          'identity_explicit':lambda p:p['frame_transforms'][0].update(source_frame='frame.world'),
          'point_model_only':lambda p:p['frame_transforms'][0].update(operation='map_point',input_kind='quantity.position',output_kind='quantity.position'),
          'torque_model':lambda p:p['frame_transforms'][0].update(input_kind='quantity.torque',output_kind='quantity.torque',input_representation='rep.record.double.r1.n3.axial',output_representation='rep.record.double.r1.n3.axial'),
          'displacement_model':lambda p:p['frame_transforms'][0].update(input_kind='quantity.displacement',output_kind='quantity.displacement'),
        }.items():model(name,mut,mode='changed')
        caps=dict(frame_contexts=64,reference_points=256,frames=256,frame_transforms=512)
        def fill(p,table,n):
            first=copy.deepcopy(p[table][0]);original=copy.deepcopy(p[table]);p[table]=original+[dict(first,id=first['id']+'.entry'+str(i)) for i in range(len(original),n)]
            # Keep the first authored ID for existing references.
            p[table][0]['id']=first['id']
            if table=='frames':p['frame_transforms'][0].update(source_frame=first['id'],target_frame=first['id'])
        for table,cap in caps.items():
            model('limit_'+table,lambda p,t=table,n=cap:fill(p,t,n+1),code=4304)
            model('ceiling_'+table,lambda p,t=table,n=cap:fill(p,t,n),mode='changed')
        big=copy.deepcopy(policy)
        for table in tables:fill(big,table,3)
        # Reconnect after frame names changed.
        big['frame_transforms']=[dict(big['frame_transforms'][0],id='transform.test'+str(i)) for i in range(3)]
        path=write('lowered_limits.json',big);run('lowered_limits',[probe,base,path,'lower'])
        attrs='FISICS_DIM(force) FISICS_QUANTITY_KIND("quantity.force")'
        frame='FISICS_FRAME("frame.world")';axis='FISICS_COMPONENT_AXIS("axis.x")';rep='FISICS_REPRESENTATION("rep.record.double.r1.n3.polar")'
        pre='#include <fisics/extensions.h>\ntypedef struct V {double x,y,z;} V;\n'
        def decl(name,body,expected=0,pol=base,diag=None,overlay=True):
            src=write(name+'.c',pre+body+'\n')
            diag_path=out/(name+'.diagnostics.json');args=[compiler,'-I'+str(ROOT/'include'),'--dump-sema','--emit-diags-json',diag_path]
            if overlay:args+=['--overlay=physics-semantics','--physical-semantics-policy',pol]
            log=run(name,[*args,src],expected)
            if diag:
                diagnostics=json.loads(diag_path.read_text())['diagnostics'];assert any(d['code_id']==diag and d['severity_name']=='error' for d in diagnostics),(name,diagnostics)
            return log
        scalar='double f '+attrs+' '+frame+' '+axis+';'
        decl('scalar_decl',scalar)
        decl('record_decl','V f '+attrs+' '+frame+' '+rep+';')
        decl('declarator_isolation','double f '+attrs+' '+frame+' '+axis+', ordinary;')
        decl('disabled_overlay',scalar,1,diag=4310,overlay=False)
        decl('v5_refuses_frames',scalar,1,pol=ROOT/'tests/fixtures/physical_semantics/representations/policy.json',diag=4312)
        for name,body in {
          'unknown_frame':scalar.replace('frame.world','frame.unknown'),
          'duplicate_frame':scalar.replace(frame,frame+' '+frame),
          'unknown_axis':scalar.replace('axis.x','axis.w'),
          'missing_frame':scalar.replace(frame,''),
          'missing_axis':scalar.replace(axis,''),
          'missing_quantity':scalar.replace(attrs,''),
          'record_axis':'V f '+attrs+' '+frame+' '+axis+' '+rep+';',
          'wrong_role':scalar.replace('quantity.force','quantity.ratio').replace('DIM(force)','DIM(1)'),
          'wrong_reference_role':scalar.replace(';',' FISICS_MOMENT_REFERENCE("point.world_zero");'),
          'unknown_point':scalar.replace(';',' FISICS_APPLIED_AT("point.missing");'),
          'bad_between':scalar.replace(';',' FISICS_BETWEEN_POINTS("point.world_zero","point.member_a_load");'),
          'transform_variable':'V f '+attrs+' '+rep+' '+frame+' FISICS_FRAME_TRANSFORM("transform.member_a_to_world.force");',
          'unsupported_typedef':'typedef double T '+attrs+' '+frame+' '+axis+';',
          'redeclare_frame':scalar+' extern double f '+attrs+' '+axis+' FISICS_FRAME("frame.member_a");',
          'malformed_between':'double r FISICS_DIM(length) FISICS_QUANTITY_KIND("quantity.displacement") '+frame+' '+axis+' [[fisics::between_points("point.world_zero")]];',
          'combined':'double f '+attrs+' '+axis+' [[fisics::frame("frame.world"),fisics::applied_at("point.world_zero")]];',
        }.items():decl(name,body,1,diag=4315 if name=='redeclare_frame' else 4312)
        decl('applied_at',scalar.replace(';',' FISICS_APPLIED_AT("point.world_zero");'))
        decl('between_points','double r FISICS_DIM(length) FISICS_QUANTITY_KIND("quantity.displacement") '+frame+' '+axis+' FISICS_BETWEEN_POINTS("point.world_zero","point.member_a_load");')
        decl('moment_reference','double m FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.torque") '+frame+' '+axis+' FISICS_MOMENT_REFERENCE("point.world_zero");')
        decl('point_subject','double p FISICS_DIM(length) FISICS_QUANTITY_KIND("quantity.position") FISICS_PHYSICAL_ORIGIN("origin.world") '+frame+' '+axis+' FISICS_POINT_ID("point.member_a_load");')
        helper='V rotate(V input '+attrs+' '+rep+' FISICS_FRAME("frame.member_a")) '+attrs+' '+rep+' '+frame+' FISICS_FRAME_TRANSFORM("transform.member_a_to_world.force");'
        decl('transform_signature',helper)
        decl('reference_preserving_helper',helper.replace(')) '+attrs,') FISICS_APPLIED_AT("point.world_zero")) '+attrs).replace(';',' FISICS_APPLIED_AT("point.world_zero");'))
        decl('reference_changing_helper',helper.replace(')) '+attrs,') FISICS_APPLIED_AT("point.world_zero")) '+attrs).replace(';',' FISICS_APPLIED_AT("point.member_a_load");'),1,diag=4312)
        fields=''.join('double '+a+' '+attrs+' '+frame+' FISICS_COMPONENT_AXIS("axis.'+a+'");' for a in 'xyz')
        decl('component_agreement','struct W {'+fields+'}; struct W v '+attrs+' '+frame+' '+rep+';')
        decl('component_axis_conflict','struct W {'+fields.replace('axis.y','axis.x')+'}; struct W v '+attrs+' '+frame+' '+rep+';',1,diag=4312)
        decl('component_frame_conflict','struct W {'+fields.replace('frame.world','frame.member_a')+'}; struct W v '+attrs+' '+frame+' '+rep+';',1,diag=4312)
        decl('component_without_carrier_frame','struct W {'+fields+'}; struct W v '+attrs+' '+rep+';',1,diag=4312)
        local=copy.deepcopy(policy);local['frame_contexts'].append(dict(id='context.other',observer_id='observer.other',state_id='state.other'));local['reference_points'].append(dict(id='point.other',context_id='context.other'));other=write('other_context.json',local)
        decl('cross_context_reference',scalar.replace(';',' FISICS_APPLIED_AT("point.other");'),1,pol=other,diag=4312)
        local=copy.deepcopy(policy);local['frames'].append(dict(id='frame.planar',context_id=local['frame_contexts'][0]['id'],dimension=2,convention='cartesian_orthonormal_right_handed',origin_point='point.world_zero',affine_origin=None));planar=write('planar.json',local)
        decl('planar_components',scalar.replace('frame.world','frame.planar'),pol=planar)
        decl('planar_z_axis',scalar.replace('frame.world','frame.planar').replace('axis.x','axis.z'),1,pol=planar,diag=4312)
        decl('partial_redeclaration_refinement',scalar+' extern double f '+attrs+' '+frame+' '+axis+' FISICS_APPLIED_AT("point.world_zero");')

        for name,body in {
          'reverse_helper':helper.replace('FISICS_FRAME("frame.member_a")',frame),
          'pointer_helper':helper.replace('V input','V *input'),
          'extra_parameter':helper.replace(')) '+attrs,'), int extra) '+attrs),
          'variadic_helper':helper.replace(')) '+attrs,'), ...) '+attrs),
          'unknown_transform':helper.replace('transform.member_a_to_world.force','transform.unknown'),
        }.items():decl(name,body,1,diag=4312)
        # All new macros erase for strict ordinary C, including two-string form.
        portable=write('portable.c',pre+scalar+'\n'+helper+'\n'+'double r FISICS_BETWEEN_POINTS("point.world_zero","point.member_a_load");\n')
        run('clang_macro_erasure',['clang','-std=c11','-pedantic-errors','-Wall','-Wextra','-Werror','-I'+str(ROOT/'include'),'-fsyntax-only',portable])
        src=write('gate.c',pre+'void check(void){'+scalar+'}\n');dest=out/'refuse_receipt.json';dest.write_text('preserve-existing-output')
        run('v6_empty_strict_rejected',[compiler,'-I'+str(ROOT/'include'),'--overlay=physics-semantics','--physical-semantics-policy',base,'--physical-semantics-function','check','--physical-semantics-strict','--physical-semantics-allow-literal-assumptions','--physical-semantics-allow-declared-boundaries','--emit-physical-semantics-json',dest,src],1,4319)
        assert json.loads(dest.read_text())['selection']['accepted'] is False
        result=dict(passed=True,compiler_sha256=hashlib.sha256(compiler.read_bytes()).hexdigest(),commands=commands,scope='model/declaration gate; expression qualification belongs to the owning flow/completion gates')
    except BaseException:
        write('result.json',dict(passed=False,commands=commands));raise
    write('result.json',result);print(f'physical frame model/declaration contracts: PASS ({len(commands)} commands)')
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',type=Path,required=True);p.add_argument('--probe',type=Path,required=True);p.add_argument('--out',type=Path)
    a=p.parse_args()
    if a.out:a.out.mkdir(parents=True,exist_ok=False);check(a.compiler.resolve(),a.probe.resolve(),a.out.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix='fisics-frame-contract-') as d:check(a.compiler.resolve(),a.probe.resolve(),Path(d))
if __name__=='__main__':main()
