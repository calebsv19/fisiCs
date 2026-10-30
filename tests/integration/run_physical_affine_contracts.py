#!/usr/bin/env python3
"""Finite point/difference arithmetic contract; no frame or affine unit conversion."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
FIXTURE = ROOT / 'tests/fixtures/physical_semantics'


def check(compiler, out):
    # Extra same-Dim8 identities test explicit pairing, not kind spelling/dimensions.
    catalog = json.loads((FIXTURE / 'catalog.json').read_text())
    for name in ('other.point', 'other.delta', 'linear.length', 'unprofiled.length'):
        catalog['quantity_kinds'].append(dict(id=name, dimension_system='fisiCs.si_dim8.v1',
            dimension_vector=[1,0,0,0,0,0,0,0], dimension_text='length'))
    (out / 'catalog.json').write_text(json.dumps(catalog))
    policy = json.loads((FIXTURE / 'policy.json').read_text())
    policy['linear_kinds'].append('linear.length')
    policy['affine_pairs'].append(dict(absolute_kind='other.point', difference_kind='other.delta'))
    policy['origins'] += [dict(id='other.origin', absolute_kind='other.point'),
                         dict(id='thermal.reference', absolute_kind='quantity.temperature.absolute')]
    (out / 'policy.json').write_text(json.dumps(policy))
    prefix = '''#include "authoring.h"
#define TD FISICS_DIM(temperature) FISICS_QUANTITY_KIND("quantity.temperature.difference")
#define TP FISICS_DIM(temperature) FISICS_QUANTITY_KIND("quantity.temperature.absolute") FISICS_PHYSICAL_ORIGIN("thermal.reference")
#define OP FISICS_DIM(length) FISICS_QUANTITY_KIND("other.point") FISICS_PHYSICAL_ORIGIN("other.origin")
#define OD FISICS_DIM(length) FISICS_QUANTITY_KIND("other.delta")
#define LL FISICS_DIM(length) FISICS_QUANTITY_KIND("linear.length")
#define UL FISICS_DIM(length) FISICS_QUANTITY_KIND("unprofiled.length")
'''
    rows = []
    env = {**os.environ, 'FISICS_MAX_PROCS':'0', 'DISABLE_CODEGEN':'1'}
    env.pop('FISICS_OVERLAY', None)
    include = ['-I'+str(ROOT/'include'), '-I'+str(FIXTURE)]
    physical = ['--overlay=physics-semantics', '--physical-semantics-policy', str(out/'policy.json')]

    def run(name, source, code=0, fact=None, contains=()):
        path = out / (name+'.c'); path.write_text(prefix+source+'\n')
        diag = out / (name+'.json')
        command = [str(compiler), *include, *physical, '--dump-sema', '--emit-diags-json', str(diag), str(path)]
        p = subprocess.run(command, cwd=ROOT, env=env, text=True, capture_output=True, timeout=60)
        text = p.stdout+p.stderr
        (out/(name+'.log')).write_text(text)
        assert p.returncode == (1 if code else 0), (name,p.returncode,text[-5000:])
        if code:
            ds = json.loads(diag.read_text())['diagnostics']
            assert any(d['code_id']==code and d['severity_name']=='error' for d in ds), (name,ds)
        if fact:
            kind, origin, reason = fact
            assert re.search(r'domain=0 slot=0 kind='+re.escape(kind)+r' origin='+re.escape(origin)+
                             r' state=known reason='+re.escape(reason), text), (name,fact,text[-5000:])
        for value in contains:
            assert value in text, (name,value,text[-5000:])
        rows.append(dict(id=name, command=command, exit=p.returncode, diagnostic=code, fact=fact, contains=contains))

    for label, A, D, B, ak, dk, origin in [
        ('position','POSITION','DELTA','LOCAL_POSITION','quantity.position','quantity.displacement','origin.world'),
        ('thermal','TEMPERATURE','TD','TP','quantity.temperature.absolute','quantity.temperature.difference','origin.kelvin.zero'),
        # Arbitrary catalog names prove that the mechanism does not hard-code the two domains.
        ('custom','OP','OD',None,'other.point','other.delta','other.origin')]:
        for op, expr, result, reason in [
            ('aa_add','a+b',None,None),
            ('aa_sub','a-b',D,'same-origin point subtraction yields paired difference'),
            ('ad_add','a+d',A,'point shifted by paired difference; origin preserved'),
            ('da_add','d+a',A,'paired difference plus point; origin preserved'),
            ('ad_sub','a-d',A,'point shifted by paired difference; origin preserved'),
            ('da_sub','d-a',None,None),
            ('dd_add','d+e',D,'same-kind linear operation'),
            ('dd_sub','d-e',D,'same-kind linear operation')]:
            source=f'{result or "double"} '+('double ' if result else '')+f'f(double a {A},double b {A},double d {D},double e {D}){{return {expr};}}'
            run(label+'_'+op,source,0 if result else 4317,
                (ak if result==A else dk, origin if result==A else '<none>',reason) if result else None)
        for op in ('+=','-='):
            run(label+'_compound_'+('add' if op=='+=' else 'sub'),f'void f(double a {A},double d {D}){{a{op}d;}}')
            run(label+'_delta_compound_'+('add' if op=='+=' else 'sub'),f'void f(double a {A},double d {D}){{d{op}a;}}',4316 if op=='+=' else 4317)
        run(label+'_point_sub_assign',f'void f(double a {A},double b {A}){{a-=b;}}',4316)
        run(label+'_point_add_assign',f'void f(double a {A},double b {A}){{a+=b;}}',4317)
        run(label+'_sub_wrong_result',f'{A} double f(double a {A},double b {A}){{return a-b;}}',4316)
        run(label+'_shift_wrong_result',f'{D} double f(double a {A},double d {D}){{return a+d;}}',4316)
        run(label+'_roundtrip',f'{D} double f(double a {A},double b {A},double d {D}){{return (d+a)-(b-d);}}')
        run(label+'_raw',f'{A} double f(double a {A},double raw){{return a+raw;}}',contains=['missing operand kind or unsupported carrier'])
        run(label+'_literal',f'{A} double f(double a {A}){{return a+1;}}',4101,contains=['missing operand kind or unsupported carrier'])
        run(label+'_unary_point',f'{A} double f(double a {A}){{return -a;}}',contains=['point/unprofiled sign operation deferred'])
        run(label+'_cast',f'{D} double f(double a {A},double b {A}){{return (double)a-(double)b;}}')
        run(label+'_trusted_rhs',f'extern {D} double get(void);{A} double f(double a {A}){{return a+get();}}',contains=['state=declared-trust reason=point shifted by paired difference; origin preserved'])
        run(label+'_trusted_point_rhs',f'extern {A} double get(void);{A} double f(double d {D}){{return d+get();}}',contains=['state=declared-trust reason=paired difference plus point; origin preserved'])
        for cmp in ('==','!=','<','<=','>','>='):
            run(label+'_compare_'+str(len(rows)),f'int f(double a {A},double b {A}){{return a{cmp}b;}}')
            run(label+'_mixed_compare_'+str(len(rows)),f'int f(double a {A},double d {D}){{return a{cmp}d;}}',4317)
        run(label+'_merge_delta',f'{A} double f(double a {A},double d {D}){{return 1?a:d;}}',4316)
        if B:
            for expr in ('a-b','a<b','1?a:b','(double)a-(double)b'):
                run(label+'_origin_'+str(len(rows)),f'double f(double a {A},double b {B}){{return {expr};}}',4318)
            for expr in ('b+d','d+b','b-d'):
                run(label+'_preserve_rhs_origin_'+str(len(rows)),f'{A} double f(double b {B},double d {D}){{return {expr};}}',4318)
            # Point subtraction clears origin: a world/local difference is freely assignable as D.
            run(label+'_clear_origin',f'{D} double f(double a {A},double b {A},double c {B},double e {B}){{return (a-b)+(c-e);}}')
    for other in ('OP','OD','LL','UL'):
        for expr in ('a+b','b+a','a-b','b-a'):
            run('unrelated_'+str(len(rows)),f'double f(double a POSITION,double b {other}){{return {expr};}}',4317)
    run('unrelated_differences','double f(double a DELTA,double b OD){return a+b;}',4317)
    run('field_array','struct S{double p POSITION;double d DELTA;};DELTA double f(struct S *s,double a[2] POSITION){return (s->p+s->d)-a[0];}')
    run('dim_conflict','double f(double a POSITION,double b ENERGY){return a+b;}',4101)
    run('generic_product','double f(double a POSITION,double d DELTA){return a*d;}',contains=['unsupported physical operation; P6 rule required'])

    # All admitted arithmetic rows execute with exactly representable inputs.
    # Expected values are independent constants, not inferred compiler facts.
    runtime = prefix+'''#include <stdio.h>
int main(void){
 double a POSITION=12,b POSITION=7,d DELTA=3,e DELTA=2;
 double t TEMPERATURE=300,u TEMPERATURE=280,v TD=5,w TD=2;
 double x DELTA=a-b;double y POSITION=a+d;double z POSITION=d+a;double q POSITION=a-d;
 double r DELTA=d+e;double s DELTA=d-e;
 double h TD=t-u;double i TEMPERATURE=t+v;double j TEMPERATURE=v+t;double k TEMPERATURE=t-v;
 double l TD=v+w;double m TD=v-w;
 a+=d;a-=e;t+=v;t-=w;
 printf("%.0f %.0f %.0f %.0f %.0f %.0f %.0f %.0f %.0f %.0f %.0f %.0f %.0f %.0f\\n",x,y,z,q,r,s,h,i,j,k,l,m,a,t);return 0;}
'''
    source = out/'runtime.c'; source.write_text(runtime)
    erased = out/'runtime-erased.c'
    # Keep definitions, erase annotation macro uses from the runtime body only.
    erased.write_text(prefix+re.sub(r'\b(?:POSITION|DELTA|TEMPERATURE|TD)\b','',runtime[len(prefix):]))
    expected = '5 15 15 9 5 1 20 305 305 295 7 3 13 303\n'
    for label,cmd,path in [('clang',['clang','-std=c99','-pedantic-errors'],source),('physical',[str(compiler),*physical],source),('ordinary',[str(compiler)],erased)]:
        exe = out/('runtime-'+label)
        for name,command in [('build',cmd+include+[str(path),'-o',str(exe)]),('run',[str(exe)])]:
            p = subprocess.run(command,cwd=ROOT,env={k:v for k,v in env.items() if k!='DISABLE_CODEGEN'},text=True,capture_output=True,timeout=60)
            (out/(label+'-'+name+'.log')).write_text(p.stdout+p.stderr)
            assert p.returncode==0,(label,name,p.stdout,p.stderr)
            if name=='run': assert p.stdout==expected,(label,p.stdout,expected)
            rows.append(dict(id=label+'-'+name,command=command,exit=p.returncode))
        exe.unlink()
    result = dict(passed=True,cases=len(rows),compiler_sha256=hashlib.sha256(compiler.read_bytes()).hexdigest(),commands=rows,
                  scope='finite scalar affine reference algebra and numerical parity; no physical strict, frame/shape or unit offset conversion')
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n')
    print('P5.4 affine contracts: PASS',len(rows),'cases')


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--compiler',type=Path,required=True);p.add_argument('--out',type=Path)
    a=p.parse_args()
    if a.out:
        a.out.mkdir(parents=True,exist_ok=False);check(a.compiler.resolve(),a.out.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix='fisics-affine-') as t: check(a.compiler.resolve(),Path(t))

if __name__=='__main__': main()
