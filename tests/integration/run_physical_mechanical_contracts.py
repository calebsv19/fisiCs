#!/usr/bin/env python3
'''P6.1.3 four finite mechanical contracts and copied structural helper gate.'''
import argparse,copy,hashlib,json,math,os,re,shutil,subprocess,tempfile
from pathlib import Path
from solver_mechanical_profile import apply_mechanical_profile
ROOT=Path(__file__).resolve().parents[2]
FIX=ROOT/'tests/fixtures/solver_physics/structural_physical'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def oracle(text):
    rows=text.splitlines();assert rows[0]=='ABI 32 304 48 48 16 144 208 272',rows[0]
    # Independent Euler-Bernoulli matrix in x1,y1,r1,x2,y2,r2 order.
    E,A,I,L=120.,2.,3.,4.;a=E*A/L;b=12*E*I/L**3;c=6*E*I/L**2;d=4*E*I/L;e=2*E*I/L
    k=[[a,0,0,-a,0,0],[0,b,c,0,-b,c],[0,c,d,0,-c,e],[-a,0,0,a,0,0],[0,-b,-c,0,b,-c],[0,c,e,0,-c,d]]
    expected={('K',i,j):k[i][j] for i in range(6) for j in range(6)}
    for n in range(3):
        q=[1+n,-2+n*.25,.25+n*.125,.5-n,3-n*.5,-.375+n*.5]
        f=[sum(k[i][j]*q[j] for j in range(6)) for i in range(6)]
        eq=[3,5,13,7,11,17]
        for i in range(6):expected['F',n,i]=f[i];expected['R',n,i]=f[i]-eq[i]
    actual={}
    for row in rows[1:]:
        tag,i,j,v=row.split();key=tag,int(i),int(j);assert key not in actual;actual[key]=float(v)
    assert actual.keys()==expected.keys()
    for key,value in expected.items():assert math.isfinite(actual[key]) and actual[key]==value,(key,actual[key],value)
    return len(expected)
def check(compiler,out,probe=None,source_root=None):
    commands=[];reports={};result=dict(passed=False,commands=commands,compiler_sha256=sha(compiler),runner_sha256=sha(Path(__file__)),patch_sha256=sha(FIX/'candidate.patch'))
    env={**os.environ,'FISICS_MAX_PROCS':'0','PYTHONDONTWRITEBYTECODE':'1'};env.pop('DISABLE_CODEGEN',None);env.pop('FISICS_OVERLAY',None)
    def write(n,v):p=out/n;p.write_text(json.dumps(v,indent=2)+'\n');return p
    def run(n,args,expected=0):
        p=subprocess.run(list(map(str,args)),cwd=ROOT,env=env,capture_output=True,text=True,timeout=120);log=p.stdout+p.stderr;(out/(n+'.log')).write_text(log)
        commands.append(dict(id=n,command=list(map(str,args)),exit=p.returncode,expected=expected));assert p.returncode==expected,(n,p.returncode,log[-3500:]);assert expected or 'Error:' not in log,(n,log[-3500:]);return log
    candidate=out/'candidate';(candidate/'src').mkdir(parents=True)
    block=(FIX/'candidate.patch').read_text().split('+++ b/src/core_structural_frame_physical.h\n',1)[1].split('\n--- ',1)[0]
    original=''.join(line[1:]+'\n' for line in block.splitlines() if line.startswith('+'))
    header=candidate/'src/core_structural_frame_physical.h';header.write_text(original)
    legacy=out/'legacy';legacy.mkdir();(legacy/header.name).write_text(original)
    if source_root:
        pins=json.loads((FIX/'patch_inputs.json').read_text())
        for n,h in pins.items():assert (sha(source_root/n) if (source_root/n).exists() else None)==h,('owner source drift',n)
        result['source_pins']={str(source_root/n):h for n,h in pins.items()}
    result['profile']=apply_mechanical_profile(candidate,out);typed=header.read_text()
    policy=json.loads((out/'policy.json').read_text());catalog=json.loads((out/'catalog.json').read_text())
    roles={k:'linear' for k in policy['linear_kinds']}
    for k,dim,t in [('energy',[2,1,-2,0,0,0,0,0],'energy'),('ratio',[0]*8,'1'),('duration',[0,0,1,0,0,0,0,0],'time'),('velocity',[1,0,-1,0,0,0,0,0],'length/time'),('acceleration',[1,0,-2,0,0,0,0,0],'length/time^2')]:
        catalog['quantity_kinds'].append(dict(id='quantity.'+k,dimension_system='fisiCs.si_dim8.v1',dimension_vector=dim,dimension_text=t));policy['linear_kinds'].append('quantity.'+k);roles['quantity.'+k]='linear'
    def rule(op,a,b,k):return dict(id='rule.'+op+'.'+a+'.'+b,operation=op,operand_kinds=['quantity.'+a,'quantity.'+b],result_kind='quantity.'+k,reference_role='linear',origin_policy='none' if op in ['multiply','divide'] else 'same',provenance='inputs')
    policy['operation_rules'] += [rule('multiply','energy','ratio','energy'),rule('multiply','acceleration','duration','velocity'),rule('fmax','force','force','force')]
    write('catalog.json',catalog);model=write('policy.json',policy)
    v1=copy.deepcopy(policy);v1.update(schema_version=1);v1.pop('operation_rules');write('v1.json',v1)
    macros='''#include <fisics/extensions.h>
#define KTT FISICS_DIM(force/length) FISICS_QUANTITY_KIND("quantity.translational_stiffness")
#define LL FISICS_DIM(force/length) FISICS_QUANTITY_KIND("quantity.line_load")
#define TR FISICS_DIM(force) FISICS_QUANTITY_KIND("quantity.force_per_angle")
#define RT FISICS_DIM(force) FISICS_QUANTITY_KIND("quantity.moment_per_displacement")
#define KRR FISICS_DIM(force*length) FISICS_QUANTITY_KIND("quantity.rotational_stiffness")
#define FORCE FISICS_DIM(force) FISICS_QUANTITY_KIND("quantity.force")
#define TORQUE FISICS_DIM(force*length) FISICS_QUANTITY_KIND("quantity.torque")
#define ENERGY FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.energy")
#define D FISICS_DIM(length) FISICS_QUANTITY_KIND("quantity.displacement")
#define ANGLE FISICS_DIM(1) FISICS_QUANTITY_KIND("quantity.angle")
#define RATIO FISICS_DIM(1) FISICS_QUANTITY_KIND("quantity.ratio")
#define DT FISICS_DIM(time) FISICS_QUANTITY_KIND("quantity.duration")
#define V FISICS_DIM(length/time) FISICS_QUANTITY_KIND("quantity.velocity")
#define A FISICS_DIM(length/time^2) FISICS_QUANTITY_KIND("quantity.acceleration")
'''
    author=out/'authoring.h';author.write_text(macros)
    inc=['-I'+str(ROOT/'include'),'-I'+str(out),'-I'+str(candidate/'src')]
    both=['literal-assumptions','declared-boundaries']
    def report(n,body,expected=0,fn='check',permissions=(),pol=model,code=0):
        src=out/(n+'.c');src.write_text('#include "authoring.h"\n'+body+'\n');dest=out/(n+'.json')
        args=[compiler,*inc,'--overlay=physics-semantics','--physical-semantics-policy',pol,'--physical-semantics-function',fn,'--physical-semantics-strict','--emit-physical-semantics-json',dest,*['--physical-semantics-allow-'+p for p in permissions]]
        run(n,[*args,src],expected)
        if dest.exists():
            data=json.loads(dest.read_text());reports[n]=data;s=data['selection'];assert s['accepted']==(expected==0);assert s['required']==s['checked']+s['permitted_trust']+s['blocked'];assert s['total']==s['required']+s['ordinary_controls']==len(data['obligations'])
            if not expected:assert s['checked']>0 and not s['blocked'] and s['enumeration_complete']
        if code:
            diag=out/(n+'-diag.json');run(n+'-diagnostic',[compiler,*inc,'--overlay=physics-semantics','--physical-semantics-policy',pol,'--emit-diags-json',diag,src],1);assert any(d['code_id']==code for d in json.loads(diag.read_text())['diagnostics'])
        return dest
    try:
        tuples=[('KTT','D','FORCE'),('TR','ANGLE','FORCE'),('RT','D','TORQUE'),('KRR','ANGLE','TORQUE')]
        for n,(a,b,k) in enumerate(tuples):
            for rev in [False,True]:report('primitive_'+str(n)+str(rev),f'{k} double check(double a {a},double b {b}){{return '+('b*a' if rev else 'a*b')+';}')
        # Complete the promised linear/difference mechanical operand matrix.
        # Keep alternate catalogs separate so existing receipt freshness is intact.
        affine_catalog=copy.deepcopy(catalog)
        affine_catalog['quantity_kinds'].append(dict(id='quantity.position',dimension_system='fisiCs.si_dim8.v1',dimension_vector=[1,0,0,0,0,0,0,0],dimension_text='length'))
        write('affine-catalog.json',affine_catalog)
        affine=copy.deepcopy(policy);affine['catalog_paths']=['affine-catalog.json']
        affine['linear_kinds'].remove('quantity.displacement')
        affine['affine_pairs'].append(dict(absolute_kind='quantity.position',difference_kind='quantity.displacement'))
        affine['origins'].append(dict(id='origin.world',absolute_kind='quantity.position'))
        affine_model=write('affine-policy.json',affine)
        for a,k in [('KTT','FORCE'),('RT','TORQUE')]:
            for reverse in [False,True]:
                report('difference_'+a+str(reverse),f'{k} double check(double k {a},double d D){{return '+('d*k' if reverse else 'k*d')+';}',pol=affine_model)
        report('absolute_coordinate_refused','FORCE double check(double k KTT,double p FISICS_DIM(length) FISICS_QUANTITY_KIND("quantity.position") FISICS_PHYSICAL_ORIGIN("origin.world")){return k*p;}',1,permissions=both,pol=affine_model)
        ordered=copy.deepcopy(policy)
        ordered['operation_rules']=[rule for rule in ordered['operation_rules'] if rule['operand_kinds']!=['quantity.angle','quantity.force_per_angle']]
        ordered_model=write('one-order-policy.json',ordered)
        report('declared_order','FORCE double check(double k TR,double a ANGLE){return k*a;}',pol=ordered_model)
        report('undeclared_reverse','FORCE double check(double k TR,double a ANGLE){return a*k;}',1,permissions=both,pol=ordered_model)
        report('sum','FORCE double check(double k KTT,double d D,double c TR,double a ANGLE){return k*d+c*a;}')
        for n,body in [
            ('swapped_tr','FORCE double check(double k RT,double a ANGLE){return k*a;}'),
            ('swapped_rt','TORQUE double check(double k TR,double d D){return k*d;}'),
            ('line_load','FORCE double check(double k LL,double d D){return k*d;}'),
            ('collapsed_tr','FORCE double check(double k FORCE,double a ANGLE){return k*a;}'),
            ('collapsed_rt','TORQUE double check(double k FORCE,double d D){return k*d;}'),
            ('energy_product','ENERGY double check(double f FORCE,double d D){return f*d;}'),
            ('energy_as_stiffness','TORQUE double check(double k ENERGY,double a ANGLE){return k*a;}'),
            ('ratio_as_angle','FORCE double check(double k TR,double r RATIO){return k*r;}'),
            ('inverse','D double check(double f FORCE,double k KTT){return f/k;}'),
            ('bare','FORCE double check(double k TR){return k*0.5;}'),
            ('missing','FORCE double check(double k TR,double a){return k*a;}')]:report(n,body,1,permissions=both)
        report('wrong_result','ENERGY double check(double k KRR,double a ANGLE){return k*a;}',1,permissions=both,code=4316)
        report('v4_ratio','ENERGY double check(double e ENERGY,double r RATIO){return e*r;}')
        report('v4_time','V double check(double a A,double t DT){return a*t;}')
        report('v4_helper','#include <math.h>\nFORCE double check(double f FORCE){return fmax(f,f);}')
        report('literal','FORCE double check(double k TR){ANGLE double a=0.5;return k*a;}',1)
        report('literal_permitted','FORCE double check(double k TR){ANGLE double a=0.5;return k*a;}',permissions=['literal-assumptions'])
        body='TR double coeff(void);FORCE double check(double a ANGLE,double f FORCE){FORCE double clean=f+f;return clean+coeff()*a;}'
        report('declared',body,1);report('declared_permitted',body,permissions=['declared-boundaries'])
        body='TR double coeff(void);FORCE double check(double f FORCE){FORCE double clean=f+f;ANGLE double a=0.5;return clean+coeff()*a;}'
        report('both_missing_permission',body,1,permissions=['literal-assumptions']);report('both_permitted',body,permissions=both)
        for n,token in [('no_kind','FISICS_QUANTITY_KIND("quantity.force_per_angle")'),('no_dim','FISICS_DIM(force)')]:
            author.write_text(macros.replace(token,''));report(n,'FORCE double check(double k TR,double a ANGLE){return k*a;}',1,permissions=both);author.write_text(macros)
            assert reports[n]['selection']['total']==reports['primitive_1False']['selection']['total']
        # Qualify the actual copied helper bodies, not replacement implementations.
        report('actual_apply','#include "core_structural_frame_physical.h"',fn='cs_physical_apply',permissions=['literal-assumptions'])
        report('actual_apply_without_literals','#include "core_structural_frame_physical.h"',1,fn='cs_physical_apply')
        report('actual_recover','#include "core_structural_frame_physical.h"',fn='cs_physical_recover_end')
        for n,expected in [('actual_apply',(26,2,6)),('actual_recover',(28,0,6))]:
            selection=reports[n]['selection'];assert (selection['checked'],selection['permitted_trust'],selection['ordinary_controls'])==expected
        assert reports['actual_apply']['selection']['total']==reports['actual_apply_without_literals']['selection']['total']
        report('construction_refused','#include "core_structural_frame_physical.h"',1,fn='cs_physical_build',permissions=both)
        report('line_load_refused','#include "core_structural_frame_physical.h"',1,fn='cs_physical_equivalent_load',permissions=both)
        report('raw_refused','#include "core_structural_frame_physical.h"',1,fn='cs_physical_dense_restore_assumed_si',permissions=both)
        for n,text in [('swapped_fields',typed.replace('quantity.force_per_angle','temporary.swap').replace('quantity.moment_per_displacement','quantity.force_per_angle').replace('temporary.swap','quantity.moment_per_displacement')),('collapsed_fields',typed.replace('quantity.force_per_angle','quantity.force').replace('quantity.moment_per_displacement','quantity.force'))]:
            header.write_text(text);report(n,'#include "core_structural_frame_physical.h"',1,fn='cs_physical_apply',permissions=both);header.write_text(typed)
            assert reports[n]['selection']['total']==reports['actual_apply']['selection']['total']
        # V3 retains its old model: no mechanics by changing only the version.
        v3=copy.deepcopy(policy);v3['schema_version']=3;v3['operation_rules']=v3['operation_rules'][8:]
        report('v3_no_mechanics','FORCE double check(double k TR,double a ANGLE){return k*a;}',1,permissions=both,pol=write('v3.json',v3))
        report('v1_model','FORCE double check(double f FORCE){return f;}',pol=out/'v1.json')
        def invalid(n,mutate,code=4303):
            p=copy.deepcopy(policy);mutate(p);path=write(n+'-policy.json',p);diag=out/(n+'-diag.json')
            run(n,[compiler,*inc,'--overlay=physics-semantics','--physical-semantics-policy',path,'--emit-diags-json',diag,out/'primitive_0False.c'],1);assert any(d['code_id']==code for d in json.loads(diag.read_text())['diagnostics'])
        invalid('ambiguous',lambda p:p['operation_rules'].append(dict(p['operation_rules'][0],id='rule.other')))
        invalid('conflicting_result',lambda p:p['operation_rules'].append(dict(p['operation_rules'][0],id='rule.conflict',result_kind='quantity.torque')))
        invalid('duplicate_id',lambda p:p['operation_rules'][1].update(id=p['operation_rules'][0]['id']))
        invalid('relabel_rule',lambda p:p['operation_rules'][0].update(result_kind='quantity.torque'))
        invalid('collapse_rule',lambda p:p['operation_rules'][2].update(operand_kinds=['quantity.force','quantity.angle']))
        invalid('line_load_rule',lambda p:p['operation_rules'][0].update(operand_kinds=['quantity.line_load','quantity.displacement']))
        invalid('swap_rule',lambda p:p['operation_rules'][2].update(operand_kinds=['quantity.moment_per_displacement','quantity.angle']))
        invalid('origin_rule',lambda p:p['operation_rules'][0].update(origin_policy='preserve'))
        invalid('provenance_rule',lambda p:p['operation_rules'][0].update(provenance='checked'))
        invalid('inverse_rule',lambda p:p['operation_rules'][0].update(operation='divide'))
        invalid('v3_mechanics',lambda p:p.update(schema_version=3))
        invalid('unknown_version',lambda p:p.update(schema_version=5),4301)
        invalid('limit',lambda p:p.update(operation_rules=[dict(p['operation_rules'][0],id='rule.n'+str(i)) for i in range(4097)]),4304)
        if probe:result['native']=json.loads(run('native_allocation',[probe,out/'v1.json',model,out/'ambiguous-policy.json']))
        reverse=copy.deepcopy(policy);reverse['operation_rules'].reverse()
        report('permuted','FORCE double check(double k TR,double a ANGLE){return k*a;}',pol=write('permuted-policy.json',reverse));assert reports['permuted']['operation_model']==reports['primitive_1False']['operation_model']
        def number(n):return n.to_bytes(8,'little')
        def text(s):return number(len(s.encode()))+s.encode()
        def fnv(data,h=1469598103934665603):
            for b in data:h=((h^b)*1099511628211)&((1<<64)-1)
            return h
        m=reports['primitive_0False']['operation_model'];assert m['version']==3
        payload=text('fisiCs.physical_mechanics.v1')+number(len(m['rules']))
        order={'fabs':2,'fmax':4,'multiply':100,'divide':101};assert m['rules']==sorted(policy['operation_rules'],key=lambda r:(order[r['operation']],r['operand_kinds']))
        for v in m['rules']:
            args=v['operand_kinds'];payload+=text(v['id'])+text(v['operation'])+text(args[0])+text(args[1] if len(args)>1 else '')+text(v['result_kind'])+number(1)+text(v['origin_policy'])+text('inputs')
        oh=fnv(payload);assert m['fingerprint']==f'{oh:016x}'
        base=int(reports['v1_model']['policy']['fingerprint'],16);assert reports['primitive_0False']['policy']['fingerprint']==f"{fnv(text('fisiCs.physical_semantics_policy.v4')+number(oh),base):016x}"
        ra=report('tu_a','FORCE double shared(double k TR,double a ANGLE){return k*a;} FORCE double check(double k TR,double a ANGLE,double f FORCE){return f+k*a;}')
        rb=report('tu_b','FORCE double shared(double k TR,double a ANGLE); FORCE double check(double k TR,double a ANGLE,double f FORCE){FORCE double clean=f+f;return clean+shared(k,a);}',permissions=['declared-boundaries'])
        for n,paths in [('cross',[ra,rb]),('cross_reverse',[rb,ra])]:run(n,[compiler,'--physical-semantics-check',*paths])
        for n,mutate in [('tamper',lambda d:d['operation_model']['rules'][0].update(result_kind='quantity.energy')),('downgrade',lambda d:d.update(schema_version=3)),('model_version',lambda d:d['operation_model'].update(version=2)),('removed',lambda d:d.pop('operation_model'))]:
            d=json.loads(ra.read_text());mutate(d);run(n,[compiler,'--physical-semantics-check',write(n+'.json',d),rb],1)
        before=model.read_bytes();model.write_bytes(before+b'\n');run('stale',[compiler,'--physical-semantics-check',ra,rb],1);model.write_bytes(before)
        runtime=out/'runtime.c';runtime.write_text(r'''
#include "core_structural_frame_physical.h"
#include <stdio.h>
#include <stddef.h>
int main(void){CsPhysicalSection s={120,2,3,4};CsPhysicalStiffness k;cs_physical_build(&s,&k);
printf("ABI %zu %zu %zu %zu %zu %zu %zu %zu\n",sizeof(s),sizeof(k),sizeof(CsPhysicalCoordinates),sizeof(CsPhysicalLoads),offsetof(CsPhysicalStiffness,tt),offsetof(CsPhysicalStiffness,tr),offsetof(CsPhysicalStiffness,rt),offsetof(CsPhysicalStiffness,rr));
double matrix[6][6];cs_physical_dense_pack(&k,matrix);for(int i=0;i<6;i++)for(int j=0;j<6;j++)printf("K %d %d %.17g\n",i,j,matrix[i][j]);
for(int n=0;n<3;n++){double raw[6]={1+n,-2+n*.25,.25+n*.125,.5-n,3-n*.5,-.375+n*.5};double equiv[6]={3,5,13,7,11,17};CsPhysicalCoordinates q;CsPhysicalLoads f,r,eq;cs_physical_coordinates_restore_assumed_si(raw,&q);cs_physical_loads_restore_assumed_si(equiv,&eq);cs_physical_apply(&k,&q,&f);cs_physical_recover_end(&k,&q,&eq,&r);double v[6],w[6];cs_physical_loads_pack(&f,v);cs_physical_loads_pack(&r,w);for(int i=0;i<6;i++){printf("F %d %d %.17g\n",n,i,v[i]);printf("R %d %d %.17g\n",n,i,w[i]);}}return 0;}
''')
        outputs=[]
        for n,flags,includes in [('baseline',['clang','-std=c11','-pedantic-errors'],['-I'+str(legacy)]),('refined',['clang','-std=c11','-pedantic-errors'],inc),('physical',[compiler,'--overlay=physics-semantics','--physical-semantics-policy',model],inc)]:
            exe=out/n;run(n+'-build',[*flags,*includes,runtime,'-o',exe]);data=run(n+'-run',[exe]);result['oracle_values']=oracle(data);outputs.append(data);exe.unlink()
        assert outputs[0]==outputs[1]==outputs[2]
        header.write_text(typed.replace('k->tr[1][0] = coupling_tr;','k->tr[1][0] = -coupling_tr;'))
        exe=out/'numeric-mutant';run('numeric-mutant-build',['clang','-std=c11',*inc,runtime,'-o',exe]);mutant=run('numeric-mutant-run',[exe]);exe.unlink();header.write_text(typed)
        try:oracle(mutant)
        except AssertionError:result['numeric_mutant_detected']=True
        else:raise AssertionError('oracle failed to detect sign mutation')
        result.update(passed=True,cases=len(commands),coverage={n:d['selection'] for n,d in reports.items()})
        print('P6.1.3 mechanics: PASS',len(commands),'commands')
    finally:write('results.json',result)
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',type=Path,required=True);p.add_argument('--probe',type=Path);p.add_argument('--out',type=Path);p.add_argument('--source-root',type=Path);a=p.parse_args()
    if a.out:a.out.mkdir(parents=True,exist_ok=False);check(a.compiler.resolve(),a.out.resolve(),a.probe,a.source_root)
    else:
        with tempfile.TemporaryDirectory(prefix='fisics-physical-mechanics-') as t:check(a.compiler.resolve(),Path(t),a.probe,a.source_root)
if __name__=='__main__':main()
