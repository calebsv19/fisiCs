#!/usr/bin/env python3
"""P6.1.1 explicit operation model, standard-helper identity and provenance gate."""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]
FIX=ROOT/'tests/fixtures/physical_semantics'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def check(compiler,out,probe=None):
    env={**os.environ,'FISICS_MAX_PROCS':'0','PYTHONDONTWRITEBYTECODE':'1'}
    env.pop('FISICS_OVERLAY',None);env.pop('DISABLE_CODEGEN',None)
    commands=[];reports={};result=dict(passed=False,compiler_sha256=sha(compiler),runner_sha256=sha(Path(__file__)),commands=commands)
    def write(name,data):
        path=out/name;path.write_text(json.dumps(data,indent=2)+'\n');return path
    catalog=json.loads((FIX/'catalog.json').read_text());policy=json.loads((FIX/'policy.json').read_text())
    # Same Dim8, distinct force/rotation and moment/translation coupling meanings.
    for name in ['force_per_angle','moment_per_displacement']:
        catalog['quantity_kinds'].append(dict(id='quantity.'+name,dimension_system='fisiCs.si_dim8.v1',dimension_vector=[1,1,-2,0,0,0,0,0],dimension_text='force'))
        policy['linear_kinds'].append('quantity.'+name)
    for name,dim,dimtext in [('translational_stiffness',[0,1,-2,0,0,0,0,0],'force/length'),('rotational_stiffness',[2,1,-2,0,0,0,0,0],'force*length'),('stress',[-1,1,-2,0,0,0,0,0],'pressure')]:
        catalog['quantity_kinds'].append(dict(id='quantity.'+name,dimension_system='fisiCs.si_dim8.v1',dimension_vector=dim,dimension_text=dimtext))
        policy['linear_kinds'].append('quantity.'+name)
    write('catalog.json',catalog);v1=write('v1.json',policy)
    policy['schema_version']=2;policy['operation_rules']=[]
    roles={k:'linear' for k in policy['linear_kinds']}
    for pair in policy['affine_pairs']:roles[pair['absolute_kind']]='absolute';roles[pair['difference_kind']]='difference'
    for kind,role in roles.items():
        for op in ['fabs','fmax']:
            if op=='fabs' and role=='absolute':continue
            policy['operation_rules'].append(dict(id='rule.'+op+'.'+kind,operation=op,operand_kinds=[kind]*(1 if op=='fabs' else 2),result_kind=kind,reference_role=role,origin_policy='preserve' if op=='fabs' else 'same',provenance='inputs'))
    v2=write('v2.json',policy)
    text=(FIX/'authoring.h').read_text().replace('FISICS_DIM(', 'D(').replace('FISICS_QUANTITY_KIND(', 'Q(')
    header='''#include <fisics/extensions.h>
#ifdef NO_KIND
#define Q(v)
#else
#define Q(v) FISICS_QUANTITY_KIND(v)
#endif
#ifdef NO_DIM
#define D(v)
#else
#define D(v) FISICS_DIM(v)
#endif
'''+text+'''
#define TD D(temperature) Q("quantity.temperature.difference")
#define TR D(force) Q("quantity.force_per_angle")
#define RT D(force) Q("quantity.moment_per_displacement")
#define KTT D(force/length) Q("quantity.translational_stiffness")
#define KRR D(force*length) Q("quantity.rotational_stiffness")
#define STRESS D(pressure) Q("quantity.stress")
'''
    (out/'authoring.h').write_text(header)
    include=['-I'+str(ROOT/'include'),'-I'+str(out)]
    def execute(name,args,expected=0,overrides=None):
        cmd=list(map(str,args));p=subprocess.run(cmd,cwd=ROOT,env={**env,**(overrides or {})},capture_output=True,text=True,timeout=120)
        log=p.stdout+p.stderr;(out/(name+'.log')).write_text(log)
        commands.append(dict(id=name,command=cmd,exit=p.returncode,expected=expected,environment=overrides or {}))
        assert p.returncode==expected,(name,p.returncode,log[-5000:])
        assert expected or 'Error:' not in log,(name,'backend diagnostic at successful exit')
        return log
    def source(name,body):
        path=out/(name+'.c');path.write_text('#include "authoring.h"\n'+body+'\n');return path
    def report(name,body,expected=0,code=0,selected='check',permissions=(),defines=(),model=v2,strict=True):
        src=source(name,body);dest=out/(name+'.json');diag=out/(name+'-diags.json')
        args=[compiler,*include,'--overlay=physics-semantics','--physical-semantics-policy',model,'--physical-semantics-function',selected,'--emit-physical-semantics-json',dest]
        if strict:args+=['--physical-semantics-strict']
        args+=['--physical-semantics-allow-'+v for v in permissions]
        execute(name,[*args,*['-D'+v for v in defines],src],expected)
        if code:
            execute(name+'-diagnostic',[compiler,*include,'--overlay=physics-semantics','--physical-semantics-policy',model,'--emit-diags-json',diag,src],1)
            assert any(d['code_id']==code for d in json.loads(diag.read_text())['diagnostics']),(name,code)
        if dest.exists():
            d=json.loads(dest.read_text());reports[name]=d;s=d['selection']
            assert s['total']==len(d['obligations'])==s['required']+s['ordinary_controls']
            assert s['required']==s['checked']+s['permitted_trust']+s['blocked']
            if strict:assert s['accepted']==(expected==0)
            if strict and not expected:assert s['checked']>0 and not s['blocked'] and s['enumeration_complete']
        return src,dest
    both=['literal-assumptions','declared-boundaries']
    try:
        body='#include <math.h>\nENERGY double check(double a ENERGY){return fabs(a);}'
        report('fabs',body)
        report('fmax','#include <math.h>\nENERGY double check(double a ENERGY,double b ENERGY){return fmax(a,b);}')
        assert not reports['fabs']['selection']['permitted_trust']
        assert any(e['operation_rule']=='rule.fabs.quantity.energy' for e in reports['fabs']['expressions'])
        assert reports['fabs']['schema_version']==2 and reports['fabs']['operation_model']['version']==1
        report('delta_fabs','#include <math.h>\nDELTA double check(double a DELTA){return fabs(a);}')
        report('temperature_delta','#include <math.h>\nTD double check(double a TD,double b TD){return fmax(a,b);}')
        report('point_fmax','#include <math.h>\nPOSITION double check(double a POSITION,double b POSITION){return fmax(a,b);}')
        assert any(e['fact']['origin']=='origin.world' for e in reports['point_fmax']['expressions'] if e['operation_rule'])
        report('point_fabs','#include <math.h>\nPOSITION double check(double a POSITION){return fabs(a);}',1,permissions=both)
        report('kind_mismatch','#include <math.h>\nENERGY double check(double a ENERGY,double b TORQUE){return fmax(a,b);}',1,4317,permissions=both)
        report('angle_ratio','#include <math.h>\nANGLE double check(double a ANGLE,double b RATIO){return fmax(a,b);}',1,4317,permissions=both)
        report('origin_mismatch','#include <math.h>\nPOSITION double check(double a POSITION,double b LOCAL_POSITION){return fmax(a,b);}',1,4318,permissions=both)
        report('coupling_mismatch','#include <math.h>\nTR double check(double a TR,double b RT){return fmax(a,b);}',1,4317,permissions=both)
        report('coupling_preserved','#include <math.h>\nRT double check(double a RT){return fabs(a);}')
        report('rotational_stiffness_not_torque','#include <math.h>\nKRR double check(double a KRR,double b TORQUE){return fmax(a,b);}',1,4317,permissions=both)
        report('translational_stiffness','#include <math.h>\nKTT double check(double a KTT){return fabs(a);}')
        report('normal_envelope','#include <math.h>\nSTRESS double check(double tension STRESS,double compression STRESS,double stress STRESS){double positive STRESS=fmax(tension,stress);double negative STRESS=fmax(compression,-stress);return fmax(positive,negative);}')
        for op in ['sqrt','hypot']:
            expr='sqrt(a*a)' if op=='sqrt' else 'hypot(a,a)'
            report('deferred_'+op,'#include <math.h>\nDELTA double check(double a DELTA){return '+expr+';}',1,permissions=both)
        report('product_deferred','ENERGY double check(double a RT,double b DELTA){return a*b;}',1,permissions=both)
        report('bare_literal','#include <math.h>\nANGLE double check(double a ANGLE){return fmax(a,0);}',1,permissions=both)
        for name in ['kind','dim']:
            report('removed_'+name,body,1,permissions=both,defines=['NO_'+name.upper()])
            assert reports['removed_'+name]['selection']['total']==reports['fabs']['selection']['total']
            assert reports['removed_'+name]['selection']['ordinary_controls']==reports['fabs']['selection']['ordinary_controls']
        report('v1_unchanged',body,1,model=v1,permissions=both)
        assert reports['v1_unchanged']['schema_version']==1 and 'operation_model' not in reports['v1_unchanged']
        assert all('operation_rule' not in e for e in reports['v1_unchanged']['expressions'])
        for fn,ctype in [('fabsf','float'),('fabsl','long double'),('fmaxf','float'),('fmaxl','long double')]:
            expr=fn+'(a)' if fn.startswith('fabs') else fn+'(a,a)'
            report(fn,'#include <math.h>\nENERGY '+ctype+' check('+ctype+' a ENERGY){return '+expr+';}')
        generic=source('generic_fabs','#include <tgmath.h>\nENERGY double check(double a ENERGY){return fabs(a);}')
        log=execute('generic_fabs_direct',[compiler,*include,'--overlay=physics-semantics','--physical-semantics-policy',v2,'--dump-sema',generic],overrides={'DISABLE_CODEGEN':'1'})
        assert 'identity-checked standard helper; admitted input-derived physical rule' in log
        report('generic_fabs_closure','#include <tgmath.h>\nENERGY double check(double a ENERGY){return fabs(a);}',1)
        assert 'unable to capture analyzed physical source dependencies' in (out/'generic_fabs_closure.log').read_text()
        assert not (out/'generic_fabs_closure.json').exists()

        report('builtin_fabs','ENERGY double check(double a ENERGY){return __builtin_fabs(a);}')
        report('user_name','double fabs(double x){return x;} ENERGY double check(double a ENERGY){return fabs(a);}',1,permissions=both)
        user='ENERGY double fabs(double x ENERGY){return x;} ENERGY double check(double a ENERGY){return fabs(a);}'
        report('user_declared_blocked',user,1)
        report('user_declared_permitted',user,permissions=['declared-boundaries'])
        assert not any(e['operation_rule'] for e in reports['user_declared_permitted']['expressions'])
        report('pointer_alias','#include <math.h>\nENERGY double check(double a ENERGY){double(*fn)(double)=fabs;return fn(a);}',1,permissions=both)
        report('shadowed_local','#include <math.h>\ndouble raw(double x){return x;} ENERGY double check(double a ENERGY){double(*fabs)(double)=raw;return fabs(a);}',1,permissions=both)
        report('ordinary_comparison','#include <math.h>\n_Bool check(double a ENERGY,double b ENERGY){return fmax(a,b)>a;}')
        report('literal_blocked','#include <math.h>\nENERGY double check(void){double e ENERGY=2;return fabs(e);}',1)
        report('literal_permitted','#include <math.h>\nENERGY double check(void){double e ENERGY=2;return fabs(e);}',permissions=['literal-assumptions'])
        combined='#include <math.h>\nextern ENERGY double get(void); ENERGY double check(double e ENERGY){return fabs(fmax(get(),(e=2)));}'
        report('trust_only_refused',combined,1,permissions=both)
        assert reports['trust_only_refused']['selection']['checked']==0 and reports['trust_only_refused']['selection']['blocked']==0
        combined=combined.replace('{return fabs','{double checked_copy ENERGY=e;return fabs')
        for name,permissions,expected in [('none',[],1),('literal',['literal-assumptions'],1),('declared',['declared-boundaries'],1),('both',both,0)]:
            report('combined_'+name,combined,expected,permissions=permissions)
        assert any(e['operation_rule'] and e['trust_mask']==3 for e in reports['combined_both']['expressions'])
        assert not any(o['checked'] and o['trust_mask'] for o in reports['combined_both']['obligations'])
        report('wrong_dim','#include <math.h>\nENERGY double check(double a ENERGY,double b DELTA){return fmax(a,b);}',1,permissions=both)
        report('wrong_unit','#include <math.h>\ndouble check(double a D(length) Q("quantity.displacement") FISICS_UNIT(meter),double b D(length) Q("quantity.displacement") FISICS_UNIT(millimeter)){return fmax(a,b);}',1,permissions=both)
        # Policy/schema/reference conflicts are rejected before source analysis.
        variants=[]
        def bad(name,change,code=4303):
            value=copy.deepcopy(policy);change(value);path=write(name+'-policy.json',value)
            diag=out/(name+'-diags.json');execute(name,[compiler,*include,'--overlay=physics-semantics','--physical-semantics-policy',path,'--emit-diags-json',diag,out/'fabs.c'],1)
            assert any(d['code_id']==code for d in json.loads(diag.read_text())['diagnostics'])
            variants.append(name)
        bad('old_schema_new_fields',lambda p:p.update(schema_version=1),4301)
        bad('v2_missing_rules',lambda p:p.pop('operation_rules'),4301)
        bad('bad_version',lambda p:p.update(schema_version=5),4301)
        bad('rules_type',lambda p:p.update(operation_rules=None))
        bad('rule_extra',lambda p:p['operation_rules'][0].update(override=True))
        bad('rule_missing',lambda p:p['operation_rules'][0].pop('provenance'))
        bad('rule_unknown_kind',lambda p:p['operation_rules'][0].update(result_kind='quantity.unknown'))
        bad('rule_bad_id',lambda p:p['operation_rules'][0].update(id='bad id'))
        bad('rule_duplicate_id',lambda p:p['operation_rules'][1].update(id=p['operation_rules'][0]['id']))
        bad('rule_ambiguous',lambda p:p['operation_rules'].append(dict(p['operation_rules'][0],id='rule.other')))
        bad('rule_product',lambda p:p['operation_rules'][0].update(operation='multiply'))
        for op in ['sqrt','hypot','dot','cross']:bad('rule_reserved_'+op,lambda p,op=op:p['operation_rules'][0].update(operation=op))
        bad('rule_bad_arity',lambda p:p['operation_rules'][0].update(operand_kinds=[]))
        bad('rule_relabel',lambda p:p['operation_rules'][0].update(result_kind='quantity.torque'))
        bad('rule_bad_dimension',lambda p:p['operation_rules'][0].update(result_kind='quantity.displacement'))
        bad('rule_unknown_operand',lambda p:p['operation_rules'][0].update(operand_kinds=['quantity.unknown']))
        bad('rule_absolute_fabs',lambda p:p['operation_rules'][0].update(operand_kinds=['quantity.position'],result_kind='quantity.position',reference_role='absolute'))
        bad('rule_role',lambda p:p['operation_rules'][0].update(reference_role='absolute'))
        bad('rule_origin',lambda p:p['operation_rules'][0].update(origin_policy='discard'))
        bad('rule_provenance',lambda p:p['operation_rules'][0].update(provenance='checked'))
        value=copy.deepcopy(policy);value['operation_rules']=[dict(value['operation_rules'][0],id='rule.limit.'+str(i)) for i in range(4097)]
        path=out/'limit-policy.json';path.write_text(json.dumps(value,separators=(',',':')))
        diag=out/'limit-diags.json';execute('rule_limit',[compiler,*include,'--overlay=physics-semantics','--physical-semantics-policy',path,'--emit-diags-json',diag,out/'fabs.c'],1)
        assert any(d['code_id']==4304 for d in json.loads(diag.read_text())['diagnostics'])
        # Empty v2 is valid but opts into no helpers. Rule order is irrelevant.
        empty=copy.deepcopy(policy);empty['operation_rules']=[]
        report('empty_rules',body,1,model=write('empty-policy.json',empty),permissions=both)
        reverse=copy.deepcopy(policy);reverse['operation_rules'].reverse()
        report('permuted',body,model=write('permuted-policy.json',reverse))
        assert reports['permuted']['policy']['fingerprint']==reports['fabs']['policy']['fingerprint']
        assert reports['permuted']['operation_model']==reports['fabs']['operation_model']
        # Independent byte encoding of the canonical helper model and v2 policy.
        model=reports['fabs']['operation_model']
        assert model['rules']==sorted(policy['operation_rules'],key=lambda r:(r['operation'],r['operand_kinds'][0]))
        def number(n):return int(n).to_bytes(8,'little')
        def text(v):return number(len(v.encode()))+v.encode()
        def hash_bytes(data,h=1469598103934665603):
            for b in data:h=((h^b)*1099511628211)&((1<<64)-1)
            return h
        payload=text('fisiCs.physical_helpers.v1')+number(len(model['rules']))
        for rule in model['rules']:
            payload+=text(rule['id'])+text(rule['operation'])+text(rule['result_kind'])
            payload+=number({'linear':1,'absolute':2,'difference':3}[rule['reference_role']])
            payload+=text(rule['origin_policy'])+text('inputs')
        op_hash=hash_bytes(payload);assert model['fingerprint']==f'{op_hash:016x}'
        v1_hash=int(reports['v1_unchanged']['policy']['fingerprint'],16)
        assert reports['fabs']['policy']['fingerprint']==f"{hash_bytes(text('fisiCs.physical_semantics_policy.v2')+number(op_hash),v1_hash):016x}"
        # Dedicated cross-TU checker verifies v2 operations independently of body acceptance.
        _,ra=report('tu_a','#include <math.h>\nENERGY double shared(double a ENERGY){return fabs(a);}',selected='shared')
        _,rb=report('tu_b','ENERGY double shared(double a ENERGY); ENERGY double check(double a ENERGY){return shared(a)+a;}',permissions=['declared-boundaries'])
        for name,paths in [('ab',[ra,rb]),('ba',[rb,ra])]:execute('cross_'+name,[compiler,'--physical-semantics-check',*paths])
        _,nested=report('tu_math_b','#include <math.h>\nENERGY double shared(double a ENERGY); ENERGY double check(double a ENERGY){return fmax(a,a);}')
        text=execute('shared_math_header_refused',[compiler,'--physical-semantics-check',ra,nested],1)
        assert 'unsupported shared physical signature surface' in text
        def tamper(name,change):
            value=json.loads(ra.read_text());change(value);path=write(name+'.json',value)
            execute(name,[compiler,'--physical-semantics-check',path,rb],1)
        tamper('receipt_rule_fingerprint',lambda d:d['operation_model'].update(fingerprint='0000000000000000'))
        tamper('receipt_rule_identity',lambda d:d['operation_model']['rules'][0].update(id='rule.forged'))
        tamper('receipt_rule_role',lambda d:d['operation_model']['rules'][0].update(reference_role='absolute'))
        tamper('receipt_rule_provenance',lambda d:d['operation_model']['rules'][0].update(provenance='checked'))
        tamper('receipt_rule_operand',lambda d:d['operation_model']['rules'][0].update(operand_kinds=['quantity.torque']))
        tamper('receipt_model_version',lambda d:d['operation_model'].update(version=2))
        tamper('receipt_rule_removed',lambda d:d['operation_model']['rules'].pop())
        tamper('receipt_downgrade',lambda d:(d.update(schema_version=1),d.pop('operation_model')))
        tamper('receipt_bad_version',lambda d:d.update(schema_version=3))
        changed=copy.deepcopy(policy);changed['operation_rules'][0]['id']='rule.changed'
        _,rc=report('changed_model',body,model=write('changed-policy.json',changed))
        for name,paths in [('ab',[rb,rc]),('ba',[rc,rb])]:execute('different_model_'+name,[compiler,'--physical-semantics-check',*paths],1)
        before=v2.read_bytes();v2.write_bytes(before+b'\n');execute('stale_policy',[compiler,'--physical-semantics-check',ra,rb],1);v2.write_bytes(before)
        if probe:
            output=execute('native_allocation',[probe,v1,v2,out/'rule_ambiguous-policy.json']);result['native']=json.loads(output)
        # Exact numerical/layout control, including signed zero/NaN choice in fmax.
        runtime=source('runtime','''#include <math.h>
#include <stdio.h>
#include <stddef.h>
struct Payload{double e ENERGY;double t TORQUE;};
ENERGY double choose(double a ENERGY,double b ENERGY){return fmax(fabs(a),b);}
int main(void){struct Payload p={-3,7};printf("%zu %zu %.1f %.1f %d %d\\n",sizeof(p),offsetof(struct Payload,t),choose(p.e,2),fmax(NAN,4),signbit(fabs(-0.0))!=0,fmax(-0.0,0.0)==0.0);return 0;}
''')
        expected='16 8 3.0 4.0 0 1\n'
        for name,flags in [('clang',['clang','-std=c99','-pedantic-errors']),('physical',[compiler,'--overlay=physics-semantics','--physical-semantics-policy',v2])]:
            exe=out/name;execute(name+'-build',[*flags,*include,runtime,'-lm','-o',exe]);assert execute(name+'-run',[exe])==expected;exe.unlink()
        result.update(passed=True,cases=len(commands),policy_failure_cases=len(variants)+1,coverage={name:d['selection'] for name,d in reports.items()})
        print('P6.1.1 operations: PASS',len(commands),'commands')
    finally:(out/'results.json').write_text(json.dumps(result,indent=2)+'\n')
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',type=Path,required=True);p.add_argument('--probe',type=Path);p.add_argument('--out',type=Path)
    a=p.parse_args()
    if a.out:a.out.mkdir(parents=True,exist_ok=False);check(a.compiler.resolve(),a.out.resolve(),a.probe.resolve() if a.probe else None)
    else:
        with tempfile.TemporaryDirectory(prefix='fisics-physical-operations-') as t:check(a.compiler.resolve(),Path(t),a.probe.resolve() if a.probe else None)
if __name__=='__main__':main()
