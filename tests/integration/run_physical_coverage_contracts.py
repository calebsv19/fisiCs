#!/usr/bin/env python3
"""P5.5 selected physical strict/export and fresh cross-TU receipt contracts."""
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
PREFIX='''#include <fisics/extensions.h>
#ifndef NO_KIND
#define Q(x) FISICS_QUANTITY_KIND(x)
#define O(x) FISICS_PHYSICAL_ORIGIN(x)
#else
#define Q(x)
#define O(x)
#endif
#ifndef NO_DIM
#define D(x) FISICS_DIM(x)
#else
#define D(x)
#endif
#define E D(energy) Q("quantity.energy")
#define T D(energy) Q("quantity.torque")
#define A D(1) Q("quantity.angle")
#define R D(1) Q("quantity.ratio")
#define P D(length) Q("quantity.position") O("origin.world")
#define LP D(length) Q("quantity.position") O("origin.local")
#define DT D(length) Q("quantity.displacement")
'''


def check(compiler,out):
    fixture=out/'policy';shutil.copytree(ROOT/'tests/fixtures/physical_semantics',fixture)
    policy=fixture/'policy.json'
    commands=[];reports={}
    env={**os.environ,'FISICS_MAX_PROCS':'0'};env.pop('FISICS_OVERLAY',None)
    base=[str(compiler),'-I'+str(ROOT/'include'),'--overlay=physics-semantics','--physical-semantics-policy',str(policy)]
    def execute(name,cmd,expected=0,overrides=None):
        p=subprocess.run(list(map(str,cmd)),cwd=ROOT,env={**env,**(overrides or {})},text=True,capture_output=True,timeout=90)
        (out/(name+'.log')).write_text(p.stdout+p.stderr)
        assert p.returncode==expected,(name,p.returncode,expected,(p.stdout+p.stderr)[-6000:])
        commands.append(dict(id=name,command=list(map(str,cmd)),exit=p.returncode,environment=overrides or {}));return p
    def run(name,body,expected=0,strict=True,permissions=(),defines=(),extra=(),overrides=None,require_report=True):
        source=out/(name+'.c');source.write_text(PREFIX+body+'\n');dest=out/(name+'.json')
        cmd=[*base,'--physical-semantics-function','check','--emit-physical-semantics-json',str(dest)]
        if strict:cmd+=['--physical-semantics-strict']
        cmd+=['--physical-semantics-allow-'+p for p in permissions]
        execute(name,cmd+['-D'+d for d in defines]+list(extra)+[str(source)],expected,overrides)
        if require_report:
            d=json.loads(dest.read_text());reports[name]=d;s=d['selection']
            assert d['schema']=='fisiCs.physical_semantics' and d['schema_version']==1
            assert s['total']==len(d['obligations'])==s['required']+s['ordinary_controls']
            assert s['required']==s['checked']+s['permitted_trust']+s['blocked']
            assert s['ordinary_controls']==sum(not row['strict_required'] for row in d['obligations'])
            assert s['checked']==sum(row['checked'] for row in d['obligations'])
            assert all(not row['trust_mask'] for row in d['obligations'] if row['checked'])
            if strict and expected==0:assert s['accepted'] and s['checked']>0 and s['blocked']==0 and s['enumeration_complete']
            if strict and expected:assert not s['accepted'] or not d['analysis_valid']
            assert d['declaration_bindings'] and all('fact' in b and 'site' in b for b in d['declaration_bindings'])
            assert d['dependencies']['complete']
            ids={e['id'] for e in d['expressions']}
            assert all(dep in ids for e in d['expressions'] for dep in e['dependencies'])
            assert all(row['expression'] is None or row['expression'] in ids for row in d['obligations'])
        return source,dest
    both=('literal-assumptions','declared-boundaries')
    simple='E double check(double a E,double b E){return a+b;}'
    run('known_linear',simple)
    run('known_affine','DT double check(double a P,double b P){return a-b;}')
    run('known_shift','P double check(double a P,double b DT){return b+a;}')
    run('cast','E double check(double a E){return (double)a;}')
    run('field','struct S{double a E;double b E;};E double check(struct S *s){return s->a+s->b;}')
    run('array','E double check(double a[2] E){return a[0]+a[1];}')
    loop='E double check(double a E,double b E){double x E=a;for(int i=0;i<2;i++){x=x+b;}return x;}'
    run('loop',loop)
    run('loop_missing_kind',loop,1,defines=['NO_KIND'],permissions=both)
    run('loop_missing_dim',loop,1,defines=['NO_DIM'],permissions=both)
    for other in ('loop_missing_kind','loop_missing_dim'):
        a=reports['loop']['selection'];b=reports[other]['selection']
        assert (a['total'],a['ordinary_controls'])==(b['total'],b['ordinary_controls'])
    counter='E double check(double a E){for(int i D(1) Q("quantity.angle")=0;i<2;i++){}return a+a;}'
    run('tagged_counter',counter,1)
    run('erased_counter',counter.replace('D(1) Q("quantity.angle")',''))
    assert reports['tagged_counter']['selection']['ordinary_controls']==reports['erased_counter']['selection']['ordinary_controls']==3
    assert not reports['tagged_counter']['selection']['ordinary_controls_valid']
    assert reports['erased_counter']['selection']['ordinary_controls_valid']
    boolean='D(1) Q("quantity.angle") _Bool check(double a E){double x E=a+a;return 1;}'
    run('tagged_status',boolean,1)
    run('erased_status',boolean.replace('D(1) Q("quantity.angle") ',''))
    assert reports['tagged_status']['selection']['ordinary_controls']==reports['erased_status']['selection']['ordinary_controls']==1
    logical='_Bool check(double a A,double b A,double c A){return (a<b)&&(b<c);}'
    # Logical-expression returns are conservative required rows; comparison
    # dependencies still prevent unknown operands from being waived.
    run('logical_metadata',logical)
    run('logical_erased',logical,1,defines=['NO_KIND','NO_DIM'])
    assert reports['logical_metadata']['selection']['ordinary_controls']==reports['logical_erased']['selection']['ordinary_controls']
    run('missing_input','E double check(double a){return a;}',1,permissions=both)
    run('missing_output','double check(double a E){return a;}',1,permissions=both)
    run('generic_product','E double check(double a E,double r A){return a*r;}',1,permissions=both)
    run('raw_alias','E double check(double a E){double *p=&a;return *p;}',1,permissions=both)
    run('raw_record','struct S{double e E;};E double check(void *p){return ((struct S*)p)->e;}',1,permissions=both)
    run('unprofiled','double check(double a D(1) Q("quantity.unprofiled")){return a+a;}',1,permissions=both)
    run('point_sign','P double check(double a P){return -a;}',1,permissions=both)
    run('literal_blocked','E double check(double a E){double x E=2;return x+a;}',1)
    run('literal_permitted','E double check(double a E){double x E=2;return x+a;}',permissions=['literal-assumptions'])
    run('literal_tree','E double check(double a E){double x E=2+3;return x+a;}',permissions=['literal-assumptions'])
    run('literal_return_only','E double check(void){return 2;}',1,permissions=both)
    run('empty','void check(void){}',1)
    run('boolean_status','_Bool check(double a E,double b E){double x E=a+b;return 1;}')
    call='extern E double ext(double a E);E double check(double a E){return ext(a)+a;}'
    run('declared_blocked',call,1)
    run('declared_permitted',call,permissions=['declared-boundaries'])
    mixed='extern E double ext(double a E);E double check(double a E,double b E){return (a=2)+ext(b);}'
    run('mixed_literal_only',mixed,1,permissions=['literal-assumptions'])
    run('mixed_declared_only',mixed,1,permissions=['declared-boundaries'])
    run('mixed_both',mixed,permissions=both)
    assert any(row['trust_mask']==3 for row in reports['mixed_both']['obligations'])
    interface='struct S{double e E;};void helper(struct S *s){s->e=s->e;} E double check(struct S *s,double a E){helper(s);return a+a;}'
    run('interface_blocked',interface,1)
    run('interface_permitted',interface,permissions=['declared-boundaries'])
    run('interface_missing_field',interface.replace('double e E','double e D(energy)'),1,permissions=both)
    run('interface_missing_kind',interface,1,defines=['NO_KIND'],permissions=both)
    run('interface_raw','struct S{double e E;};void helper(struct S *s){s->e=s->e;} E double check(void *raw,double a E){struct S *s=(struct S*)raw;helper(s);return a+a;}',1,permissions=both)
    run('comparison','_Bool check(double a E,double b E){return a<b;}')
    run('unknown_comparison','_Bool check(double a E,double b){return a<b;}',1,permissions=both)
    run('conditional','E double check(_Bool c,double a E,double b E){return c?a:b;}')
    run('mismatch','E double check(double a T){return a;}',1,permissions=both)
    run('wrong_origin','DT double check(double a P,double b LP){return a-b;}',1,permissions=both)
    run('missing_comparison_kind','_Bool check(double a A,double b D(1)){return a<b;}',1,permissions=both)
    run('report_unknown','E double check(double a){return a;}',strict=False)
    assert not reports['report_unknown']['selection']['accepted']
    # Deterministic publication at the same source identity/options.
    source=out/'known_linear.c';dest=out/'repeat.json'
    execute('repeat',[*base,'--physical-semantics-function','check','--physical-semantics-strict','--emit-physical-semantics-json',dest,source])
    assert dest.read_bytes()==(out/'known_linear.json').read_bytes()
    # Capture guard-only and empty headers; they must not be inferred from token ranges.
    empty=out/'empty.h';empty.write_text('');guard=out/'guard.h';guard.write_text('#ifndef GUARD_ONLY\n#define GUARD_ONLY\n#endif\n')
    source,dest=run('header_closure','#include "empty.h"\n#include "guard.h"\n'+simple)
    paths={p['path'] for p in reports['header_closure']['dependencies']['files']}
    assert str(empty) in paths and str(guard) in paths
    # A blocked publication retains the last good bytes and removes its temp.
    sentinel=out/'sentinel.json';sentinel.write_text('old good receipt\n')
    common=[*base,'--physical-semantics-function','check','--physical-semantics-strict','--emit-physical-semantics-json']
    for flag in ('FISICS_TEST_FAIL_PHYSICAL_RECEIPT_WRITE','FISICS_TEST_FAIL_PHYSICAL_COVERAGE_ALLOC','FISICS_TEST_FAIL_PHYSICAL_CLOSURE'):
        execute(flag,[*common,sentinel,out/'known_linear.c'],1,{flag:'1'})
        assert sentinel.read_text()=='old good receipt\n'
        assert not list(out.glob('sentinel.json.tmp.*'))
    for name,args in [
        ('no_function',['--emit-physical-semantics-json',sentinel]),
        ('no_report',['--physical-semantics-function','check','--physical-semantics-strict']),
        ('duplicate',['--physical-semantics-function','check','--physical-semantics-function','check','--emit-physical-semantics-json',sentinel]),
        ('trust_without_strict',['--physical-semantics-function','check','--emit-physical-semantics-json',sentinel,'--physical-semantics-allow-declared-boundaries']),
        ('object_output',['--physical-semantics-function','check','--emit-physical-semantics-json',sentinel,'-o',out/'forbidden.o']),
        ('multi_source',['--physical-semantics-function','check','--emit-physical-semantics-json',sentinel,out/'known_affine.c'])]:
        execute(name,[*base,*args,out/'known_linear.c'],1);assert sentinel.read_text()=='old good receipt\n'
    assert not (out/'forbidden.o').exists()
    run('missing_function','E double other(double a E){return a;}',1,require_report=False)
    # Never overwrite source, policy or a loaded header, including symlink/hardlink aliases.
    for label,path in [('source',out/'known_linear.c'),('policy',policy),('header',ROOT/'include/fisics/extensions.h')]:
        before=path.read_bytes();execute('alias_'+label,[*common,path,out/'known_linear.c'],1);assert path.read_bytes()==before
    alias=out/'source-alias.json';alias.symlink_to(out/'known_linear.c')
    execute('alias_symlink',[*common,alias,out/'known_linear.c'],1);alias.unlink()
    alias.hardlink_to(out/'known_linear.c');execute('alias_hardlink',[*common,alias,out/'known_linear.c'],1);alias.unlink()
    # Export matching contracts from two independent TUs, then compare in both orders.
    _,ra=run('tu_a','E double shared(double x E){return x;} '+simple)
    _,rb=run('tu_b','E double shared(double x E); '+simple)
    def cross(name,a,b,expected=0):
        execute(name+'_ab',[compiler,'--physical-semantics-check',a,b],expected)
        execute(name+'_ba',[compiler,'--physical-semantics-check',b,a],expected)
    cross('matching',ra,rb)
    for tag,decl in [('kind','T double shared(double x T);'),('missing','double shared(double x D(energy));'),('arity','E double shared(double x E,double y E);'),('variable','double shared E;')]:
        _,other=run('tu_'+tag,decl+' '+simple);cross(tag,ra,other,1)
    _,pa=run('point_a','P double shared(double x P); '+simple)
    _,pb=run('point_b','LP double shared(double x LP); '+simple)
    cross('point_origin',pa,pb,1)
    _,ua=run('unit_a','P double shared(double x P FISICS_UNIT(meter)); '+simple)
    _,ub=run('unit_b','P double shared(double x P FISICS_UNIT(centimeter)); '+simple)
    cross('concrete_unit',ua,ub,1)
    _,uc=run('unit_c','P double shared(double x P FISICS_UNIT(meter)); '+simple)
    cross('matching_units',ua,uc)
    _,array=run('tu_array','E double shared(double x[2] E); '+simple)
    cross('carrier_shape',ra,array,1)
    _,array_b=run('tu_array_b','E double shared(double x[2] E); '+simple)
    cross('matching_arrays',array,array_b)
    _,static=run('tu_static','static T double shared(double x T); '+simple)
    cross('static_isolation',ra,static)
    _,ca=run('callback_a','E double shared(void (*cb)(double x E)); '+simple)
    _,cb=run('callback_b','E double shared(void (*cb)(double x T)); '+simple)
    cross('unsupported_callback',ca,cb,1)
    _,cra=run('record_a','struct V{double v E;}; E double shared(struct V *p); '+simple)
    _,crb=run('record_b','struct V{double v T;}; E double shared(struct V *p); '+simple)
    cross('unsupported_record',cra,crb,1)
    # Policy redefinition with the same IDs and exact dimensional vectors.
    variant=out/'variant';shutil.copytree(fixture,variant)
    pd=json.loads((variant/'policy.json').read_text());pd['linear_kinds'].remove('quantity.energy');(variant/'policy.json').write_text(json.dumps(pd))
    variant_json=out/'tu_variant.json'
    execute('variant_export',[str(x) if x!=str(policy) else str(variant/'policy.json') for x in common if x!='--physical-semantics-strict']+[variant_json,out/'tu_b.c'])
    cross('policy_role',ra,variant_json,1)
    # Freshness must include every actual header and all catalogs, with byte-level changes.
    for label,path in [('source',out/'tu_a.c'),('header',ROOT/'include/fisics/extensions.h'),('catalog',fixture/'catalog.json'),('policy',policy)]:
        if label=='header':
            # Use a caller-owned empty header instead of editing compiler source.
            a=out/'header_closure.json';b=rb;path=empty
        else:a=ra;b=rb
        before=path.read_bytes()
        try:
            path.write_bytes(before+b'\n');cross('stale_'+label,a,b,1)
        finally:path.write_bytes(before)
    cross('restored',ra,rb)
    # Strict schema, explicit incompleteness, and invalid-analysis guards.
    for tag,edit in [('version',lambda d:d.update(schema_version=99)),('extra',lambda d:d.update(extra=True)),('incomplete',lambda d:d['dependencies'].update(complete=False)),('invalid_analysis',lambda d:d.update(analysis_valid=False)),('missing_catalogs',lambda d:d['policy'].update(catalogs=[])),('missing_options',lambda d:d.update(options={})),('bad_definition',lambda d:d['declarations'][0]['slots'][0]['fact'].update(definition_fingerprint='0000000000000000'))]:
        d=copy.deepcopy(reports['tu_a']);edit(d);bad=out/(tag+'.json');bad.write_text(json.dumps(d));cross(tag,bad,rb,1)
    execute('duplicate_input',[compiler,'--physical-semantics-check',ra,ra],1)
    execute('no_receipts',[compiler,'--physical-semantics-check'],1)
    execute('checker_mixed_mode',[compiler,'--physical-semantics-check',ra,rb,'--overlay=physics-semantics'],1)
    p=execute('explain_incomplete',[compiler,'--explain','4319']);assert 'Selected physical obligations' in p.stdout+p.stderr
    result=dict(passed=True,compiler_sha256=hashlib.sha256(compiler.read_bytes()).hexdigest(),commands=commands,cases=len(commands),scope='P5.5 selected physical obligations, complete read-set receipts and cross-TU declaration consistency; no body adoption, runtime physics, hermetic replay or graph-v1 consumption')
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n')
    print('P5.5 physical coverage/receipt contracts: PASS',len(commands),'commands')


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',required=True,type=Path);p.add_argument('--out',type=Path);a=p.parse_args()
    if a.out:a.out.mkdir(parents=True,exist_ok=False);check(a.compiler.resolve(),a.out.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix='fisics-physical-coverage-') as d:check(a.compiler.resolve(),Path(d))

if __name__=='__main__':main()
