#!/usr/bin/env python3
"""P5.3 source/boundary contract, explicitly excluding physical strict/export."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]
FIXTURE=ROOT/'tests/fixtures/physical_semantics'
PREFIX='#include "authoring.h"\n'

def check(compiler,out):
    rows=[]
    env={**os.environ,'FISICS_MAX_PROCS':'0','DISABLE_CODEGEN':'1'}
    env.pop('FISICS_OVERLAY',None)
    def run(name,source,code=0,contains=(),extra=(),overlay=True):
        path=out/(name+'.c');path.write_text(PREFIX+source+'\n');diag=out/(name+'.json')
        command=[str(compiler),'-I'+str(ROOT/'include'),'-I'+str(FIXTURE),'--dump-sema','--emit-diags-json',str(diag)]
        if overlay:command+=['--overlay=physics-semantics','--physical-semantics-policy',str(FIXTURE/'policy.json')]
        command += list(extra)+[str(path)]
        p=subprocess.run(command,cwd=ROOT,env=env,text=True,capture_output=True,timeout=60)
        text=p.stdout+p.stderr;(out/(name+'.log')).write_text(text)
        assert p.returncode==(1 if code else 0),(name,p.returncode,text[-4500:])
        if code:
            ds=json.loads(diag.read_text())['diagnostics'];assert any(d['code_id']==code and d['severity_name']=='error' for d in ds),(name,ds)
        for value in contains:assert value in text,(name,value,text[-4500:])
        rows.append(dict(id=name,command=command,exit=p.returncode,diagnostic=code,contains=list(contains)));return text
    run('same_kind','ENERGY double id(ENERGY double x){return x;} int main(void){ double e ENERGY=2.0; double f ENERGY=3.0; e=id(f); return 0;}',contains=['assignment expected=quantity.energy actual=quantity.energy state=declared-trust'])
    run('prefix_local','int main(void){ ENERGY double e=2; TORQUE double t=3; e=t; return 0;}',4316,['expected quantity.energy','actual quantity.torque'])
    run('initializer_mismatch','int main(void){ double t TORQUE=2; double e ENERGY=t; return 0;}',4316)
    run('argument_mismatch','void accept(double x ENERGY){} int main(void){ double t TORQUE=3; accept(t); return 0;}',4316)
    run('return_mismatch','ENERGY double wrong(double x TORQUE){return x;}',4316)
    run('merge_mismatch','int main(void){ double e ENERGY=2,t TORQUE=3; double z ENERGY=1?e:t; return 0;}',4316)
    run('merge_valid','int main(void){ double e ENERGY=2,f ENERGY=3; double z ENERGY=1?e:f; return 0;}')
    run('sum_valid','ENERGY double add(double a ENERGY,double b ENERGY){return a+b;}')
    run('difference_valid','ENERGY double sub(double a ENERGY,double b ENERGY){return a-b;}')
    run('sum_mismatch','double add(double a ENERGY,double b TORQUE){return a+b;}',4317)
    run('difference_mismatch','double sub(double a ENERGY,double b TORQUE){return a-b;}',4317)
    run('angle_ratio','double test(double a ANGLE,double b RATIO){return a+b;}',4317)
    run('comparison_mismatch','int test(double a ANGLE,double b RATIO){return a<b;}',4317)
    run('comparison_control','int test(double a ENERGY,double b ENERGY){return a<b;}',contains=['comparison boolean is an ordinary C control'])
    run('numeric_cast','int main(void){ double t TORQUE=3; double e ENERGY=(double)t; return 0;}',4316)
    run('unary_preserve','int main(void){ double e ENERGY=2; double f ENERGY=-e; return 0;}')
    run('unary_mismatch','int main(void){ double t TORQUE=2; double e ENERGY=-t; return 0;}',4316)
    run('compound_assignment','int main(void){ double e ENERGY=2,f ENERGY=3; e+=f; return 0;}')
    run('compound_assignment_mismatch','int main(void){ double e ENERGY=2,t TORQUE=3; e+=t; return 0;}',4317)
    run('literal_assumptions','ENERGY double f(void){ double e ENERGY=2; e=3; return 4;}',contains=['state=literal-trust'])
    run('literal_not_in_arithmetic','ANGLE double f(double e ANGLE){return e+1;}',contains=['missing operand kind or unsupported carrier'])
    run('missing_input','ENERGY double f(double raw){ double e ENERGY=raw; return e=raw;}',contains=['assignment expected=quantity.energy actual=<unknown> state=unknown'])
    run('unknown_call','extern double raw(void); ENERGY double f(void){return raw();}',contains=['return expected=quantity.energy actual=<unknown> state=unknown'])
    run('generic_product','ENERGY double f(double e ENERGY,double r RATIO){return e*r;}',contains=['unsupported physical operation; P6 rule required'])
    run('point_addition_rejected','POSITION double f(double p POSITION){return p+p;}',4317)
    run('point_delta_valid','POSITION double f(double p POSITION,double d DELTA){return p+d;}',contains=['point shifted by paired difference; origin preserved'])
    run('point_origin_valid','POSITION double f(double p POSITION){return p;}')
    run('point_wrong_origin','POSITION double f(double p LOCAL_POSITION){return p;}',4318)
    run('point_missing_origin','double p FISICS_DIM(length) FISICS_QUANTITY_KIND("quantity.position");',4318)
    run('difference_with_origin','double p DELTA FISICS_PHYSICAL_ORIGIN("origin.world");',4318)
    run('unknown_origin','double p FISICS_DIM(length) FISICS_QUANTITY_KIND("quantity.position") FISICS_PHYSICAL_ORIGIN("origin.missing");',4313)
    run('origin_without_kind','double p FISICS_PHYSICAL_ORIGIN("origin.world");',4311)
    run('duplicate_kind','double e ENERGY FISICS_QUANTITY_KIND("quantity.energy");',4311)
    run('duplicate_origin','double p POSITION FISICS_PHYSICAL_ORIGIN("origin.world");',4311)
    run('malformed_kind','double e FISICS_DIM(energy) [[fisics::quantity_kind(quantity.energy)]];',4311)
    run('unknown_kind','double e FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.missing");',4313)
    run('kind_dim_conflict','double e FISICS_DIM(length) FISICS_QUANTITY_KIND("quantity.energy");',4314)
    run('missing_dim','double e FISICS_QUANTITY_KIND("quantity.energy");',contains=['missing dimensional contract'])
    run('disabled','double e ENERGY;',4310,overlay=False)
    run('units_only','double e ENERGY;',4310,extra=['--overlay=physics-units'],overlay=False)
    run('all_unchanged','double e ENERGY;',4310,extra=['--overlay=all'],overlay=False)
    run('field_mismatch','struct S { double e ENERGY; double t TORQUE; }; void f(struct S *s){s->e=s->t;}',4316)
    run('field_valid','struct S { double e ENERGY; double t TORQUE; }; void f(struct S *s){double v ENERGY=s->e; s->e=v;}')
    run('array_valid','void f(void){double a[2] ENERGY={1,2}; double e ENERGY=a[1]; a[0]=e;}')
    run('array_mismatch','void f(void){double a[2] ENERGY={1,2}; double t TORQUE=3; a[0]=t;}',4316)
    run('array_initializer','void f(double t TORQUE){double a[2] ENERGY={1,t};}',4316)
    run('record_initializer','struct S {double e ENERGY;}; void f(double t TORQUE){struct S s={t};}',4316)
    run('designated_initializer','struct S {double e ENERGY; double t TORQUE;}; void f(double t TORQUE){struct S s={.e=t};}',4316)
    run('shadow_identity','void f(double outer TORQUE){double x ENERGY=1; {double x TORQUE=2; outer=x;} x=outer;}',4316)
    run('field_name_isolation','struct A {double x ENERGY;};struct B {double x TORQUE;};void f(struct A *a,struct B *b){a->x=b->x;}',4316)
    run('declarator_isolation','void f(void){double e ENERGY=1,t TORQUE=2; e=t;}',4316)
    run('shared_prefix','ENERGY double e=1,f=2; void test(void){e=f;}')
    run('explicit_deref','void f(void){double e ENERGY=1,t TORQUE=2; *(&e)=t;}',4316)
    run('raw_alias_unknown','void f(void){double e ENERGY=1,t TORQUE=2; double *p=&t; e=*p;}',contains=['assignment expected=quantity.energy actual=<unknown> state=unknown'])
    run('pointer_cast_unknown','void f(void){double e ENERGY=1,t TORQUE=2; e=*((double*)(void*)&t);}',contains=['assignment expected=quantity.energy actual=<unknown> state=unknown'])
    run('union_unknown','union U {double e ENERGY; double t TORQUE;};void f(union U *u){double e ENERGY=u->e;}',contains=['union/type-punning storage is unverified'])
    run('pointer_annotation_refused','double *p ENERGY;',4312)
    run('record_annotation_refused','struct S{double x;};struct S s FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.energy");',4312)
    run('typedef_annotation_refused','typedef double E ENERGY;',4312)
    run('return_redecl_mismatch','ENERGY double f(void); TORQUE double f(void);',4315)
    run('return_redecl_inherit','ENERGY double f(void); double f(void){double t TORQUE=2;return t;}',4316)
    run('param_redecl_mismatch','void f(double e ENERGY); void f(double t TORQUE);',4315)
    run('param_redecl_inherit','void f(double e ENERGY); void f(double e){double t TORQUE=2;e=t;}',4316)
    run('variable_late_kind','extern double e FISICS_DIM(energy); double e ENERGY; void f(double t TORQUE){e=t;}',4316)
    run('variable_late_kind_valid','extern double e FISICS_DIM(energy); double e ENERGY; void f(double q ENERGY){e=q;}')
    run('linked_extern_variable','extern double e ENERGY; void f(void){extern double e TORQUE;}',4315)
    run('linked_extern_variable_valid','extern double e ENERGY; void f(void){extern double e;double q ENERGY=1;e=q;}')
    run('linked_extern_function','ENERGY double f(double e ENERGY);void g(void){extern TORQUE double f(double e ENERGY);}',4315)
    run('linked_extern_function_valid','ENERGY double f(double e ENERGY);void g(void){extern double f(double e);double q ENERGY=1;q=f(q);}')
    run('variable_redecl_mismatch','extern double e ENERGY; extern double e TORQUE;',4315)
    run('variable_redecl_inherit','extern double e ENERGY; double e; void f(double t TORQUE){e=t;}',4316)
    run('indirect_argument','void f(void (*cb)(double x ENERGY),double t TORQUE){cb(t);}',4316)
    run('indirect_valid','void f(void (*cb)(double x ENERGY),double e ENERGY){cb(e);}')
    run('raw_record_cast_unknown','struct S {double e ENERGY;}; ENERGY double f(void *raw){return ((struct S*)raw)->e;}',contains=['raw pointer/cast storage is unverified'])
    run('raw_record_alias_unknown','struct S {double e ENERGY;}; ENERGY double f(void *raw){struct S *s=(struct S*)raw;return s->e;}',contains=['raw pointer/cast storage is unverified'])
    run('cast_callback_unknown','void f(void (*cb)(double x TORQUE),double e ENERGY){((void (*)(double x ENERGY))cb)(e);}',contains=['cast/untracked callable boundary is unverified'])
    run('callback_assignment_mismatch','void wrong(double x TORQUE){} void f(void){void (*cb)(double x ENERGY)=wrong;}',4316)
    run('callback_assignment_valid','void right(double x ENERGY){} void f(void){void (*cb)(double x ENERGY)=right;}')
    run('combined_attribute_refused','double e FISICS_DIM(energy) [[fisics::quantity_kind("quantity.energy"),fisics::physical_origin("origin.world")]];',4311)
    run('definition_after_annotation','ENERGY double f(void); double f(void){double e ENERGY=1;return e;}')
    run('array_parameter_valid','void take(double x[2] ENERGY){} void f(void){double a[2] ENERGY={1,2}; take(a);}')
    run('array_parameter_mismatch','void take(double x[2] ENERGY){} void f(void){double a[2] TORQUE={1,2}; take(a);}',4316)
    run('point_merge_origin','void f(double p POSITION,double q LOCAL_POSITION){double a POSITION=1?p:q;}',4318)
    run('point_delta_assignment','void f(double p POSITION,double d DELTA){p=d;}',4316)
    run('unknown_arithmetic_profile','double f(double a FISICS_DIM(1) FISICS_QUANTITY_KIND("quantity.unprofiled")){return a+a;}',contains=['affine/unprofiled arithmetic deferred'])
    # CLI guards execute before source compilation and output publication.
    for name,args in [('policy_missing',['--overlay=physics-semantics']),('overlay_missing',['--physical-semantics-policy',str(FIXTURE/'policy.json')]),('duplicate_policy',['--overlay=physics-semantics','--physical-semantics-policy',str(FIXTURE/'policy.json'),'--physical-semantics-policy',str(FIXTURE/'policy.json')]),('multiple_tu',['--overlay=physics-semantics','--physical-semantics-policy',str(FIXTURE/'policy.json'),str(out/'same_kind.c')])]:
        p=subprocess.run([str(compiler),*args,str(out/'same_kind.c')],cwd=ROOT,env=env,text=True,capture_output=True,timeout=30)
        assert p.returncode==1,(name,p.stdout,p.stderr);(out/(name+'.log')).write_text(p.stdout+p.stderr);rows.append(dict(id=name,exit=p.returncode,command=args))
    # Numerical/layout controls use identical numerical C with only annotation
    # markers erased for the ordinary compiler path.
    runtime=PREFIX+r"""#include <stdio.h>
#include <stddef.h>
struct Payload { double e ENERGY; double t TORQUE; };
ENERGY double pass(double e ENERGY){return +e;}
int main(void){struct Payload p={2,3};double a[2] ENERGY={4,5};p.e=pass(p.e+a[1]);printf("%lu %lu %.1f %.1f %.1f\n",(unsigned long)sizeof(p),(unsigned long)offsetof(struct Payload,t),p.e,p.t,a[1]);return 0;}
"""
    runtime_path=out/'runtime.c';runtime_path.write_text(runtime)
    import re
    erased=out/'runtime-erased.c';erased.write_text(re.sub(r'\b(?:ENERGY|TORQUE)\b','',runtime))
    runtime_env={k:v for k,v in env.items() if k!='DISABLE_CODEGEN'}
    def execute(name,command,expected=0):
        p=subprocess.run(list(map(str,command)),cwd=ROOT,env=runtime_env,text=True,capture_output=True,timeout=60)
        (out/(name+'.log')).write_text(p.stdout+p.stderr);assert p.returncode==expected,(name,p.returncode,p.stdout[-3000:],p.stderr)
        rows.append(dict(id=name,command=list(map(str,command)),exit=p.returncode));return p.stdout
    include=['-I'+str(ROOT/'include'),'-I'+str(FIXTURE)]
    physical=['--overlay=physics-semantics','--physical-semantics-policy',str(FIXTURE/'policy.json')]
    expected='16 8 7.0 3.0 5.0\n'
    for name,command in [('clang-runtime',['clang','-std=c99','-pedantic-errors',*include,runtime_path]),('physical-runtime',[compiler,*physical,*include,runtime_path]),('ordinary-runtime',[compiler,*include,erased])]:
        exe=out/name;execute(name+'-build',command+['-o',exe]);assert execute(name,[exe])==expected
    # Cross-compiler calling ABI, with each fisiCs invocation still exactly one TU.
    callee=out/'abi-callee.c';callee.write_text(PREFIX+'ENERGY double pass(double e ENERGY){return +e;}\n')
    caller=out/'abi-caller.c';caller.write_text(PREFIX+'#include <stdio.h>\nENERGY double pass(double e ENERGY);int main(void){double e ENERGY=7;printf("%.1f\\n",pass(e));return 0;}\n')
    for label,cc_callee,cc_caller in [('fisics-callee',[compiler,*physical],['clang','-std=c99','-pedantic-errors']),('fisics-caller',['clang','-std=c99','-pedantic-errors'],[compiler,*physical])]:
        co=out/(label+'-callee.o');ca=out/(label+'-caller.o');exe=out/label
        execute(label+'-callee',cc_callee+include+['-c',callee,'-o',co]);execute(label+'-caller',cc_caller+include+['-c',caller,'-o',ca]);execute(label+'-link',['clang',co,ca,'-o',exe]);assert execute(label,[exe])=='7.0\n'
    for path in out.iterdir():
        if path.suffix=='.o' or (path.is_file() and os.access(path,os.X_OK)):path.unlink()
    result=dict(passed=True,compiler_sha256=hashlib.sha256(compiler.read_bytes()).hexdigest(),cases=len(rows),commands=rows,scope='P5.3 scalar/homogeneous-array local compatibility; no physical strict/export or simulation acceptance')
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n');print('P5.3 physical semantics: PASS',len(rows),'cases')

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',type=Path,required=True);p.add_argument('--out',type=Path)
    a=p.parse_args()
    if a.out:a.out.mkdir(parents=True,exist_ok=False);check(a.compiler.resolve(),a.out.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix='fisics-physical-semantics-') as tmp:check(a.compiler.resolve(),Path(tmp))
if __name__=='__main__':main()
