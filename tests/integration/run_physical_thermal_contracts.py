#!/usr/bin/env python3
"""Six compiler-only thermal prerequisite gates; no solver source adoption."""
import argparse, copy, hashlib, json, math, os, subprocess, tempfile
from decimal import Decimal, localcontext
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
FIX = ROOT/'tests/fixtures/physical_semantics/thermal'


def check(compiler, out, probe=None):
    commands, reports = [], {}
    result = dict(passed=False, commands=commands, coverage={},
                  compiler_sha256=hashlib.sha256(compiler.read_bytes()).hexdigest(),
                  scope='compiler prerequisites only; thermal owner source unchanged')
    env = {**os.environ, 'FISICS_MAX_PROCS':'0', 'PYTHONDONTWRITEBYTECODE':'1'}
    env.pop('FISICS_OVERLAY', None)
    def write(name, value):
        p=out/name;p.write_text(json.dumps(value,indent=2)+'\n');return p
    def run(name, args, expected=0):
        args=list(map(str,args))
        p=subprocess.run(args,cwd=ROOT,env=env,capture_output=True,text=True,timeout=120)
        log=p.stdout+p.stderr;(out/(name+'.log')).write_text(log)
        commands.append(dict(id=name,command=args,exit=p.returncode,expected=expected))
        assert p.returncode==expected,(name,p.returncode,log[-3000:])
        assert expected or 'Error:' not in log,(name,log[-3000:])
        return p.stdout
    policy=json.loads((FIX/'policy.json').read_text())
    catalog=json.loads((FIX/'catalog.json').read_text())
    write('catalog.json',catalog);model=write('policy.json',policy)
    (out/'authoring.h').write_text((FIX/'authoring.h').read_text())
    inc=['-I'+str(ROOT/'include'),'-I'+str(out)]
    def macro(kind):return 'TH_'+kind.removeprefix('quantity.').replace('.','_').upper()
    def report(name, body, expected=0, pol=model, permissions=(), overlay='physical', math_header=True):
        src=out/(name+'.c');src.write_text('#include "authoring.h"\n'+('#include <math.h>\n' if math_header else '')+body+'\n')
        dest=out/(name+'.json')
        if overlay=='physical':
            args=['-DTHERMAL_PHYSICAL=1','--overlay=physics-semantics','--physical-semantics-policy',pol,
                  '--physical-semantics-function','check','--physical-semantics-strict',
                  '--emit-physical-semantics-json',dest]
            args+=['--physical-semantics-allow-'+x for x in permissions]
        else:
            args=['-DTHERMAL_DIMENSIONAL=1','--overlay=physics-units','--units-coverage-function','check',
                  '--units-coverage-strict','--emit-units-coverage-json',dest]
            args+=['--units-coverage-allow-'+x for x in permissions]
        run(name,[compiler,*inc,*args,src],expected)
        if dest.exists():
            d=json.loads(dest.read_text())
            if overlay=='units':d['selection']=dict(d['strict_policy'],enumeration_complete=d['scope']['enumeration_complete'])
            s=d['selection'];reports[name]=d
            assert s['accepted']==(expected==0),(name,s)
            assert s['required']==s['checked']+s['permitted_trust']+s['blocked'],name
            if not expected:
                assert s['enumeration_complete'] and s['checked']>0 and not s['blocked'],name
                assert not s['permitted_trust'],(name,s)
            return d
        assert expected,name
    both=['literal-assumptions','declared-boundaries']
    try:
        # 1. Every finite arithmetic tuple, both authored multiplication orders.
        arithmetic=[r for r in policy['operation_rules'] if r['operation'] in ['multiply','divide']]
        for i,r in enumerate(arithmetic):
            a,b=map(macro,r['operand_kinds']);k=macro(r['result_kind']);op='*' if r['operation']=='multiply' else '/'
            body=f'{k} double check(double a {a},double b {b}){{return a{op}b;}}'
            report('arithmetic-'+str(i),body)
            report('unknown-'+str(i),body.replace('double a '+a,'double a'),1,permissions=both)
            p=copy.deepcopy(policy);p['operation_rules'].remove(r)
            report('missing-'+str(i),body,1,pol=write('missing-'+str(i)+'-policy.json',p),permissions=both)
        report('affine-delta','TH_TEMPERATURE_DIFFERENCE double check(double a TH_TEMPERATURE_ABSOLUTE,double b TH_TEMPERATURE_ABSOLUTE){return a-b;}')
        report('capacity-ratio','TH_HEAT_CAPACITY double check(double c TH_HEAT_CAPACITY,double r TH_RATIO){return c/(r+r);}')
        # 2. Exact standard math identity and complete strict obligations.
        for op in ['expm1','log']:
            report(op,f'TH_RATIO double check(double r TH_RATIO){{return {op}(r);}}')
            report(op+'-units',f'TH_RATIO double check(double r TH_RATIO){{return {op}(r);}}',overlay='units')
            for suffix,typ in [('f','float'),('l','long double')]:
                report(op+suffix,f'TH_RATIO {typ} check({typ} r TH_RATIO){{return {op}{suffix}(r);}}')
            for k in ['energy','angle','temperature.absolute']:
                report(op+'-bad-'+k.replace('.','_'),f'TH_RATIO double check(double x {macro(k)}){{return {op}(x);}}',1,permissions=both)
            report(op+'-dimensionful-units',f'TH_RATIO double check(double x TH_DURATION){{return {op}(x);}}',1,permissions=both,overlay='units')
            report(op+'-raw',f'TH_RATIO double check(double x){{return {op}(x);}}',1,permissions=both)
            report(op+'-cast',f'TH_RATIO double check(double x TH_ENERGY){{return {op}((double)x);}}',1,permissions=both)
            report(op+'-user',f'#undef {op}\ndouble {op}(double x){{return x;}} TH_RATIO double check(double x TH_RATIO){{return {op}(x);}}',1,permissions=both)
            report(op+'-callback',f'TH_RATIO double check(double x TH_RATIO,double (*cb)(double)){{return cb(x);}}',1,permissions=both)
        for suffix,typ in [('', 'double'),('f','float'),('l','long double')]:
            report('fmin'+suffix,f'TH_ENERGY {typ} check({typ} a TH_ENERGY,{typ} b TH_ENERGY){{return fmin{suffix}(a,b);}}')
        report('fmin-absolute','TH_TEMPERATURE_ABSOLUTE double check(double a TH_TEMPERATURE_ABSOLUTE,double b TH_TEMPERATURE_ABSOLUTE){return fmin(a,b);}')
        report('fmin-difference','TH_TEMPERATURE_DIFFERENCE double check(double a TH_TEMPERATURE_DIFFERENCE,double b TH_TEMPERATURE_DIFFERENCE){return fmin(a,b);}')
        report('fmin-units','TH_ENERGY double check(double a TH_ENERGY,double b TH_ENERGY){return fmin(a,b);}',overlay='units')
        report('fmin-kind','TH_ENERGY double check(double a TH_ENERGY,double b TH_TORQUE){return fmin(a,b);}',1,permissions=both)
        report('fmin-dim','TH_ENERGY double check(double a TH_ENERGY,double b TH_DURATION){return fmin(a,b);}',1,permissions=both,overlay='units')
        report('fmin-unit','#include <fisics/extensions.h>\nTH_ENERGY double check(double a FISICS_UNIT(joule),double b FISICS_UNIT(kilojoule)){return fmin(a,b);}',1,permissions=both,overlay='units')
        report('fmin-origin','TH_TEMPERATURE_ABSOLUTE double check(double a TH_TEMPERATURE_ABSOLUTE,double b FISICS_DIM(temperature) FISICS_QUANTITY_KIND("quantity.temperature.absolute") FISICS_PHYSICAL_ORIGIN("origin.other")){return fmin(a,b);}',1,permissions=both)
        other=copy.deepcopy(policy);other['origins'].append(dict(id='origin.other',absolute_kind='quantity.temperature.absolute'))
        report('fmin-different-known-origin','TH_TEMPERATURE_ABSOLUTE double check(double a TH_TEMPERATURE_ABSOLUTE,double b FISICS_DIM(temperature) FISICS_QUANTITY_KIND("quantity.temperature.absolute") FISICS_PHYSICAL_ORIGIN("origin.other")){return fmin(a,b);}',1,pol=write('other-origin.json',other),permissions=both)
        for op in ['expm1','log','fmin']:
            binary=op=='fmin';kind='TH_ENERGY' if binary else 'TH_RATIO'
            params=f'double a {kind},double b {kind}' if binary else f'double a {kind}'
            call=f'{op}(a,b)' if binary else f'{op}(a)'
            body=f'{kind} double check({params}){{return {call};}}'
            stripped=copy.deepcopy(policy)
            stripped['operation_rules']=[r for r in stripped['operation_rules'] if r['operation']!=op]
            report(op+'-missing-rule',body,1,pol=write(op+'-missing-policy.json',stripped),permissions=both)
            pointer=f'{kind} double check({params}){{double (*cb)('+('double,double' if binary else 'double')+f')={op};return cb('+('a,b' if binary else 'a')+');}'
            report(op+'-pointer-to-standard',pointer,1,permissions=both)
            builtin=f'double __builtin_{op}('+('double,double' if binary else 'double')+');'+body.replace(op+'(', '__builtin_'+op+'(')
            report(op+'-unseeded-builtin',builtin,1,permissions=both)
        # 3. No storage reconstruction, thermal classification or raw laundering.
        for name,body in [
            ('absolute-product','TH_ENERGY double check(double c TH_HEAT_CAPACITY,double t TH_TEMPERATURE_ABSOLUTE){return c*t;}'),
            ('absolute-reconstruction','TH_TEMPERATURE_ABSOLUTE double check(double e TH_ENERGY,double c TH_HEAT_CAPACITY){return e/c;}'),
            ('power-is-not-thermal','TH_THERMAL double check(double p TH_POWER,double t TH_DURATION){return p*t;}'),
            ('energy-is-not-torque','TH_TORQUE double check(double p TH_POWER,double t TH_DURATION){return p*t;}'),
            ('angle-not-ratio','TH_RATIO double check(double z TH_ANGLE){return expm1(z);}'),
            ('reversed-division','TH_POWER double check(double e TH_ENERGY,double t TH_DURATION){return t/e;}'),
            ('cast-laundering','TH_POWER double check(double e TH_TORQUE,double t TH_DURATION){return (double)e/t;}'),
            ('array-laundering','TH_POWER double check(double e[2] TH_TORQUE,double t TH_DURATION){return e[0]/t;}'),
        ]:report(name,body,1,permissions=both)
        report('typed-array-preserves-kind','TH_POWER double check(double e[2] TH_ENERGY,double t TH_DURATION){return e[0]/t;}')
        report('no-op-cast-preserves-kind','TH_POWER double check(double e TH_ENERGY,double t TH_DURATION){return (double)e/t;}')
        report('thermal-sum','TH_THERMAL double check(double a TH_THERMAL,double b TH_THERMAL){return a+b;}')
        report('fmin-thermal','TH_THERMAL double check(double a TH_THERMAL,double b TH_THERMAL){return fmin(a,b);}')
        # 4. Malformed models, canonical identities, version compatibility, allocation.
        def invalid(name, mutate, code=4303):
            p=copy.deepcopy(policy);mutate(p);path=write(name+'-policy.json',p)
            diag=out/(name+'-diags.json')
            run(name,[compiler,*inc,'-DTHERMAL_PHYSICAL=1','--overlay=physics-semantics','--physical-semantics-policy',path,'--emit-diags-json',diag,out/'expm1.c'],1)
            d=json.loads(diag.read_text());assert any(x['code_id']==code for x in d['diagnostics']),(name,d)
            return path
        idx=next(i for i,r in enumerate(policy['operation_rules']) if r['operand_kinds']==['quantity.power','quantity.duration'])
        def mutate(**kw):return lambda p:p['operation_rules'][idx].update(**kw)
        bad=invalid('duplicate',lambda p:p['operation_rules'].append(dict(p['operation_rules'][idx],id='rule.duplicate')))
        invalid('wrong-result',mutate(result_kind='quantity.torque'))
        invalid('unknown-kind',mutate(operand_kinds=['quantity.power','quantity.missing']))
        invalid('wrong-role',mutate(reference_role='absolute'))
        invalid('origin-policy',mutate(origin_policy='same'))
        invalid('unknown-operation',mutate(operation='exp'))
        invalid('extra-field',mutate(unrecognized=True))
        for op in ['expm1','log','fmin']:
            n=next(i for i,r in enumerate(policy['operation_rules']) if r['operation']==op)
            invalid(op+'-bad-origin',lambda p,n=n:p['operation_rules'][n].update(origin_policy='preserve'))
            invalid(op+'-bad-arity',lambda p,n=n:p['operation_rules'][n].update(operand_kinds=[]))
            if op!='fmin':
                invalid(op+'-angle-rule',lambda p,n=n:p['operation_rules'][n].update(operand_kinds=['quantity.angle'],result_kind='quantity.angle'))
        def downgrade(p):
            p.update(schema_version=6);p.pop('energy_forms');p.pop('energy_transfers')
        invalid('old-schema-no-thermal',downgrade)
        # Canonical Dim8 is checked even if the catalog claims a different vector.
        badcat=copy.deepcopy(catalog)
        next(x for x in badcat['quantity_kinds'] if x['id']=='quantity.heat_capacity').update(dimension_vector=[2,1,-3,0,-1,0,0,0],dimension_text='energy/time/temperature')
        write('wrong-catalog.json',badcat)
        invalid('wrong-canonical-dim',lambda p:p.update(catalog_paths=['wrong-catalog.json']))
        if probe:
            result['native']=json.loads(run('native-allocation',[probe,ROOT/'tests/fixtures/physical_semantics/policy.json',model,bad]))
        reverse=copy.deepcopy(policy);reverse['operation_rules'].reverse()
        d=report('permuted','TH_RATIO double check(double r TH_RATIO){return expm1(r);}',pol=write('permuted-policy.json',reverse))
        assert d['operation_model']==reports['expm1']['operation_model']
        # Independent encoding of operation fingerprint and arity/order.
        order={'fabs':2,'fmax':4,'expm1':5,'log':6,'fmin':7,'multiply':100,'divide':101}
        m=d['operation_model'];assert m['rules']==sorted(policy['operation_rules'],key=lambda x:(order[x['operation']],x['operand_kinds']))
        def num(x):return x.to_bytes(8,'little')
        def txt(x):return num(len(x.encode()))+x.encode()
        data=txt('fisiCs.physical_mechanics.v1')+num(len(m['rules']))
        for row in m['rules']:
            args=row['operand_kinds'];data+=txt(row['id'])+txt(row['operation'])+txt(args[0])+txt(args[1] if len(args)>1 else '')+txt(row['result_kind'])+num({'linear':1,'absolute':2,'difference':3}[row['reference_role']])+txt(row['origin_policy'])+txt('inputs')
        h=1469598103934665603
        for b in data:h=((h^b)*1099511628211)&((1<<64)-1)
        assert m['fingerprint']==f'{h:016x}'
        # 5. Receipt compatibility, stale inputs and tampering must still refuse.
        a=report('tu-a','TH_ENERGY double check(double p TH_POWER,double t TH_DURATION){return p*t;}',math_header=False)
        b=report('tu-b','TH_ENERGY double check(double p TH_POWER,double t TH_DURATION){return p*t;}',math_header=False)
        run('join',[compiler,'--physical-semantics-check',out/'tu-a.json',out/'tu-b.json'])
        run('join-reverse',[compiler,'--physical-semantics-check',out/'tu-b.json',out/'tu-a.json'])
        for name,mut in [('tamper-result',lambda x:x['operation_model']['rules'][0].update(result_kind='quantity.torque')),('tamper-origin',lambda x:x['operation_model']['rules'][0].update(origin_policy='none')),('tamper-arity',lambda x:x['operation_model']['rules'][0].update(operand_kinds=[])),('remove-model',lambda x:x.pop('operation_model'))]:
            value=copy.deepcopy(a);mut(value)
            run(name,[compiler,'--physical-semantics-check',write(name+'.json',value),out/'tu-b.json'],1)
        before=model.read_bytes();model.write_bytes(before+b'\n')
        run('stale-policy',[compiler,'--physical-semantics-check',out/'tu-a.json',out/'tu-b.json'],1);model.write_bytes(before)
        # 6. Executable numerical/layout parity across six compiler variants.
        runtime=out/'runtime.c';runtime.write_text('#include "authoring.h"\n#include <math.h>\n#include <stdio.h>\n#include <stddef.h>\n'+r'''
struct Inputs { double power TH_POWER; double capacity TH_HEAT_CAPACITY; double dt TH_DURATION; };
TH_ENERGY double integrated(double p TH_POWER,double dt TH_DURATION){return p*dt;}
TH_TEMPERATURE_DIFFERENCE double rise(double e TH_ENERGY,double c TH_HEAT_CAPACITY){return e/c;}
TH_RATIO double decay(double z TH_RATIO){return -expm1(-z);}
TH_RATIO double log_ratio(double r TH_RATIO){return log(r);}
TH_ENERGY double select_energy(double a TH_ENERGY,double b TH_ENERGY){return fmin(a,b);}
int main(void){struct Inputs s={12,8,.25};double e=integrated(s.power,s.dt);
printf("%zu %zu %zu %.17g %.17g %.17g %.17g %.17g\n",sizeof(s),_Alignof(struct Inputs),offsetof(struct Inputs,dt),e,rise(e,s.capacity),decay(1e-12),log_ratio(2),select_energy(e,2));return 0;}
''')
        variants=[('clang','clang',[]),('clang-portable','clang',['-DTHERMAL_DIMENSIONAL=1','-DTHERMAL_PHYSICAL=1']),('base',compiler,[]),('units',compiler,['-DTHERMAL_DIMENSIONAL=1','--overlay=physics-units']),('physical',compiler,['-DTHERMAL_PHYSICAL=1','--overlay=physics-semantics','--physical-semantics-policy',model]),('physical-permuted',compiler,['-DTHERMAL_PHYSICAL=1','--overlay=physics-semantics','--physical-semantics-policy',out/'permuted-policy.json'])]
        outputs={}
        for name,tool,flags in variants:
            obj=out/(name+'.o');exe=out/(name+'-runtime')
            run(name+'-compile',[tool,*flags,*inc,'-ffp-contract=off','-c',runtime,'-o',obj])
            run(name+'-link',['clang',obj,'-lm','-o',exe]);outputs[name]=run(name+'-run',[exe]);obj.unlink();exe.unlink()
        assert len(set(outputs.values()))==1,outputs
        values=outputs['clang'].split();assert values[:3]==['24','8','16'] and values[3:5]==['3','0.375'] and values[-1]=='2',values
        with localcontext() as context:
            context.prec=80
            z=Decimal.from_float(1e-12)
            expected_decay=float(1-(-z).exp());expected_log=float(Decimal(2).ln())
        assert abs(float(values[5])-expected_decay)<=math.ulp(expected_decay)
        assert abs(float(values[6])-expected_log)<=math.ulp(expected_log)
        result['independent_math_oracle']='80-digit Decimal exp/log, at most one double ULP'
        result.update(passed=True,coverage={n:d['selection'] for n,d in reports.items()},runtime=outputs,arithmetic_rules=len(arithmetic),steps_completed=6)
        print('Thermal compiler prerequisites: PASS',len(commands),'commands')
    finally:write('results.json',result)


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--compiler',type=Path,required=True);p.add_argument('--probe',type=Path);p.add_argument('--out',type=Path)
    a=p.parse_args()
    if a.out:a.out.mkdir(parents=True,exist_ok=False);check(a.compiler.resolve(),a.out.resolve(),a.probe)
    else:
        with tempfile.TemporaryDirectory(prefix='fisics-thermal-contracts-') as t:check(a.compiler.resolve(),Path(t),a.probe)
if __name__=='__main__':main()
