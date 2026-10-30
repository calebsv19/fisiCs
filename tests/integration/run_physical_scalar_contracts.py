#!/usr/bin/env python3
'''Finite explicit ratio and translational timestep contracts (P6.1.2).'''
import argparse, copy, hashlib, json, os, subprocess, tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
FIX=ROOT/'tests/fixtures/physical_semantics'
def check(compiler,out,probe=None):
    commands=[];reports={};result={'passed':False,'commands':commands,'compiler_sha256':hashlib.sha256(compiler.read_bytes()).hexdigest()}
    def write(n,v):
        p=out/n;p.write_text(json.dumps(v,indent=2)+'\n');return p
    def run(n,args,expected=0):
        p=subprocess.run(list(map(str,args)),cwd=ROOT,env={**os.environ,'FISICS_MAX_PROCS':'0'},capture_output=True,text=True,timeout=120)
        log=p.stdout+p.stderr;(out/(n+'.log')).write_text(log)
        commands.append(dict(id=n,command=list(map(str,args)),exit=p.returncode,expected=expected))
        assert p.returncode==expected,(n,log[-4000:]);assert expected or 'Error:' not in log,(n,log[-4000:]);return log
    catalog=json.loads((FIX/'catalog.json').read_text());policy=json.loads((FIX/'policy.json').read_text())
    for k,d,t in [('duration',[0,0,1,0,0,0,0,0],'time'),('velocity',[1,0,-1,0,0,0,0,0],'length/time'),('acceleration',[1,0,-2,0,0,0,0,0],'length/time^2')]:
        catalog['quantity_kinds'].append(dict(id='quantity.'+k,dimension_system='fisiCs.si_dim8.v1',dimension_vector=d,dimension_text=t));policy['linear_kinds'].append('quantity.'+k)
    write('catalog.json',catalog);write('v1.json',policy)
    policy.update(schema_version=3,operation_rules=[])
    roles={k:'linear' for k in policy['linear_kinds']}
    for p in policy['affine_pairs']:roles[p['absolute_kind']]='absolute';roles[p['difference_kind']]='difference'
    def rule(op,a,b,k):
        args=[a] if b is None else [a,b]
        return dict(id='rule.'+op+'.'+a+'.'+(b or 'unary'),operation=op,operand_kinds=args,result_kind=k,reference_role=roles[k],origin_policy='none' if op in ['multiply','divide'] else 'preserve' if op=='fabs' else 'same',provenance='inputs')
    for k,role in roles.items():
        policy['operation_rules'].append(rule('fmax',k,k,k))
        if role!='absolute':policy['operation_rules'].append(rule('fabs',k,None,k))
        if role in ['linear','difference']:
            for op in ['multiply','divide']:policy['operation_rules'].append(rule(op,k,'quantity.ratio',k))
            if k!='quantity.ratio':policy['operation_rules'].append(rule('multiply','quantity.ratio',k,k))
    for a,k in [('acceleration','velocity'),('velocity','displacement')]:
        a='quantity.'+a;k='quantity.'+k;t='quantity.duration'
        policy['operation_rules'] += [rule('multiply',a,t,k),rule('multiply',t,a,k),rule('divide',k,t,a)]
    model=write('v3.json',policy)
    header=(FIX/'authoring.h').read_text()+'''
#define DT FISICS_DIM(time) FISICS_QUANTITY_KIND("quantity.duration")
#define V FISICS_DIM(length/time) FISICS_QUANTITY_KIND("quantity.velocity")
#define A FISICS_DIM(length/time^2) FISICS_QUANTITY_KIND("quantity.acceleration")
'''
    (out/'authoring.h').write_text(header);inc=['-I'+str(ROOT/'include'),'-I'+str(out)]
    def report(n,body,expected=0,permissions=(),pol=model,strict=True):
        src=out/(n+'.c');src.write_text('#include "authoring.h"\n'+body+'\n');dest=out/(n+'.json')
        args=[compiler,*inc,'--overlay=physics-semantics','--physical-semantics-policy',pol,'--physical-semantics-function','check','--emit-physical-semantics-json',dest]
        if strict:args+=['--physical-semantics-strict']
        args+=['--physical-semantics-allow-'+p for p in permissions]
        run(n,[*args,src],expected)
        if dest.exists():
            d=json.loads(dest.read_text());s=d['selection'];reports[n]=d
            assert s['required']==s['checked']+s['permitted_trust']+s['blocked']
            assert s['total']==s['required']+s['ordinary_controls']==len(d['obligations'])
            if strict:assert s['accepted']==(expected==0)
        return dest
    both=['literal-assumptions','declared-boundaries']
    try:
        for n,ret,params,expr in [
            ('scale','ENERGY','double e ENERGY,double r RATIO','e*r'),
            ('scale_left','ENERGY','double e ENERGY,double r RATIO','r*e'),
            ('scale_div','ENERGY','double e ENERGY,double r RATIO','e/r'),
            ('kick','V','double a A,double t DT','a*t'),
            ('kick_left','V','double a A,double t DT','t*a'),
            ('drift','DELTA','double v V,double t DT','v*t'),
            ('drift_left','DELTA','double v V,double t DT','t*v'),
            ('average_v','V','double d DELTA,double t DT','d/t'),
            ('average_a','A','double v V,double t DT','v/t')]:report(n,f'{ret} double check({params}){{return {expr};}}')
        report('update','POSITION double check(double p POSITION,double v V,double a A,double t DT){v+=a*t;return p+v*t;}')
        report('compound','ENERGY double check(double e ENERGY,double r RATIO){e*=r;e/=r;return e;}')
        report('difference','DELTA double check(double d DELTA,double r RATIO){return r*d/r;}')
        report('controls','ENERGY double check(double e ENERGY,double r RATIO){for(int i=0;i<2;i++){e=e*r;}return e;}')
        assert reports['controls']['selection']['ordinary_controls']>0
        report('trust_only','A double accel(void);V double check(double t DT){return accel()*t;}',1,permissions=['declared-boundaries'])
        empty=copy.deepcopy(policy);empty['operation_rules']=[]
        report('empty','V double check(double a A,double t DT){return a*t;}',1,permissions=both,pol=write('empty-policy.json',empty))
        for n,body in [
            ('angle','ENERGY double check(double e ENERGY,double r ANGLE){return e*r;}'),
            ('literal','ENERGY double check(double e ENERGY){return e*0.5;}'),
            ('raw','ENERGY double check(double e ENERGY,double r){return e*r;}'),
            ('point','POSITION double check(double p POSITION,double r RATIO){return p*r;}'),
            ('point_time','V double check(double p POSITION,double t DT){return p/t;}'),
            ('energy_product','ENERGY double check(double v V,double t DT){return v*t;}'),
            ('compound_kind_change','A double check(double a A,double t DT){a*=t;return a;}'),
            ('reverse_div','V double check(double d DELTA,double t DT){return t/d;}'),
            ('ratio_reciprocal','ENERGY double check(double e ENERGY,double r RATIO){return r/e;}'),
            ('array','ENERGY double check(double e[2] ENERGY,double r RATIO){return e*r;}')]:report(n,body,1,permissions=both)
        report('coefficient','V double check(double a A,double t DT){RATIO double half=0.5;DT double h=half*t;return a*h;}',1)
        report('coefficient_permitted','V double check(double a A,double t DT){RATIO double half=0.5;DT double h=half*t;return a*h;}',permissions=['literal-assumptions'])
        report('declared','A double accel(void); V double check(double t DT,double v V){V double clean=v+v;return clean+accel()*t;}',1)
        report('declared_permitted','A double accel(void); V double check(double t DT,double v V){V double clean=v+v;return clean+accel()*t;}',permissions=['declared-boundaries'])
        report('both','A double accel(void); V double check(double t DT,double v V){V double clean=v+v;RATIO double half=0.5;return clean+accel()*(half*t);}',1,permissions=['literal-assumptions'])
        report('both_permitted','A double accel(void); V double check(double t DT,double v V){V double clean=v+v;RATIO double half=0.5;return clean+accel()*(half*t);}',permissions=both)
        # Remove either metadata layer without reducing obligation denominator.
        source='V double check(double a A,double t DT){return a*t;}'
        for n,replace in [('no_kind',('FISICS_QUANTITY_KIND("quantity.acceleration")','')),('no_dim',('FISICS_DIM(length/time^2)',''))]:
            before=(out/'authoring.h').read_text();(out/'authoring.h').write_text(before.replace(*replace));report(n,source,1,permissions=both);(out/'authoring.h').write_text(before)
            assert reports[n]['selection']['total']==reports['kick']['selection']['total']
        report('v1_unchanged',source,1,pol=out/'v1.json',permissions=both)
        v2=copy.deepcopy(policy);v2['schema_version']=2;v2['operation_rules']=[r for r in v2['operation_rules'] if r['operation'] in ['fabs','fmax']]
        report('v2_no_scalar',source,1,pol=write('v2.json',v2),permissions=both)
        report('helper_v3','#include <math.h>\nENERGY double check(double e ENERGY){return fabs(e);}')
        variants={}
        def invalid(n,mutate,code=4303):
            p=copy.deepcopy(policy);mutate(p);variants[n]=p;path=write(n+'-policy.json',p);diag=out/(n+'-diag.json')
            run(n,[compiler,*inc,'--overlay=physics-semantics','--physical-semantics-policy',path,'--emit-diags-json',diag,out/'kick.c'],1)
            assert any(d['code_id']==code for d in json.loads(diag.read_text())['diagnostics'])
        scalar=next(i for i,r in enumerate(policy['operation_rules']) if r['operation']=='multiply' and r['operand_kinds']==['quantity.energy','quantity.ratio'])
        def change(**kw):return lambda p:p['operation_rules'][scalar].update(**kw)
        invalid('bad_ratio',change(operand_kinds=['quantity.energy','quantity.angle']))
        invalid('relabel',change(result_kind='quantity.torque'))
        invalid('origin',change(origin_policy='preserve'))
        invalid('point_rule',change(operand_kinds=['quantity.position','quantity.ratio'],result_kind='quantity.position',reference_role='absolute'))
        invalid('unprofiled_rule',change(operand_kinds=['quantity.unprofiled','quantity.ratio'],result_kind='quantity.unprofiled'))
        invalid('energy_rule',change(operand_kinds=['quantity.velocity','quantity.velocity'],result_kind='quantity.energy'))
        invalid('ambiguous',lambda p:p['operation_rules'].append(dict(p['operation_rules'][scalar],id='rule.other')))
        invalid('duplicate_id',lambda p:p['operation_rules'][scalar].update(id=p['operation_rules'][0]['id']))
        invalid('v2_scalar',lambda p:p.update(schema_version=2))
        invalid('unknown_version',lambda p:p.update(schema_version=5),4301)
        invalid('missing_rules',lambda p:p.pop('operation_rules'),4301)
        invalid('limit',lambda p:p.update(operation_rules=[dict(p['operation_rules'][scalar],id='rule.n'+str(i)) for i in range(4097)]),4304)
        if probe:result['native']=json.loads(run('native_allocation',[probe,out/'v1.json',model,out/'ambiguous-policy.json']))
        reverse=copy.deepcopy(policy);reverse['operation_rules'].reverse()
        report('permuted',source,pol=write('permuted-policy.json',reverse))
        assert reports['permuted']['operation_model']==reports['kick']['operation_model']
        assert reports['permuted']['policy']['fingerprint']==reports['kick']['policy']['fingerprint']
        # Independent canonical encoding (no compiler helper involved).
        def num(n):return n.to_bytes(8,'little')
        def txt(s):return num(len(s.encode()))+s.encode()
        def fnv(data,h=1469598103934665603):
            for b in data:h=((h^b)*1099511628211)&((1<<64)-1)
            return h
        m=reports['kick']['operation_model'];assert m['version']==2
        order={'fabs':2,'fmax':4,'multiply':100,'divide':101}
        assert m['rules']==sorted(policy['operation_rules'],key=lambda r:(order[r['operation']],r['operand_kinds']))
        payload=txt('fisiCs.physical_scalars.v1')+num(len(m['rules']))
        for r in m['rules']:
            args=r['operand_kinds'];payload+=txt(r['id'])+txt(r['operation'])+txt(args[0])+txt(args[1] if len(args)>1 else '')+txt(r['result_kind'])+num({'linear':1,'absolute':2,'difference':3}[r['reference_role']])+txt(r['origin_policy'])+txt('inputs')
        oh=fnv(payload);assert m['fingerprint']==f'{oh:016x}'
        base=int(reports['v1_unchanged']['policy']['fingerprint'],16)
        assert reports['kick']['policy']['fingerprint']==f"{fnv(txt('fisiCs.physical_semantics_policy.v3')+num(oh),base):016x}"
        ra=report('tu_a','V double shared(double a A,double t DT){return a*t;} V double check(double a A,double t DT,double v V){return v+a*t;}')
        rb=report('tu_b','V double shared(double a A,double t DT); V double check(double a A,double t DT,double v V){return shared(a,t)+v;}',permissions=['declared-boundaries'])
        run('cross',[compiler,'--physical-semantics-check',ra,rb]);run('cross_reverse',[compiler,'--physical-semantics-check',rb,ra])
        for n,mutate in [('tamper',lambda d:d['operation_model']['rules'][scalar].update(result_kind='quantity.torque')),('downgrade',lambda d:d.update(schema_version=2)),('removed',lambda d:d.pop('operation_model')),('model_version',lambda d:d['operation_model'].update(version=1))]:
            d=json.loads(ra.read_text());mutate(d);run(n,[compiler,'--physical-semantics-check',write(n+'.json',d),rb],1)
        before=model.read_bytes();model.write_bytes(before+b'\n');run('stale',[compiler,'--physical-semantics-check',ra,rb],1);model.write_bytes(before)
        # Exact kick-drift-kick / semi-implicit formula projections, not whole solver adoption.
        runtime=out/'runtime.c';runtime.write_text('#include "authoring.h"\n'+r'''
#include <stdio.h>
#include <stddef.h>
struct State{double p POSITION;double v V;};
V double kick(double v V,double a A,double t DT,double half RATIO){return v+a*(half*t);}
POSITION double drift(double p POSITION,double v V,double t DT){return p+v*t;}
POSITION double euler(double p POSITION,double v V,double a A,double t DT){v=v+a*t;return p+v*t;}
int main(void){struct State s={10,2};A double a=4;DT double t=0.25;V double v0=2;POSITION double p0=10;
s.v=kick(s.v,a,t,0.5);s.p=drift(s.p,s.v,t);s.v=kick(s.v,a,t,0.5);
printf("%zu %zu %.17g %.17g %.17g %.17g %.17g\n",sizeof(s),offsetof(struct State,v),s.p,s.v,(s.v-v0)/t,(s.p-p0)/t,euler(p0,v0,a,t));return 0;}
''')
        for n,flags in [('clang',['clang','-std=c99','-pedantic-errors']),('physical',[compiler,'--overlay=physics-semantics','--physical-semantics-policy',model])]:
            exe=out/n;run(n+'-build',[*flags,*inc,runtime,'-o',exe]);assert run(n+'-run',[exe])=='16 8 10.625 3 4 2.5 10.75\n';exe.unlink()
        result.update(passed=True,cases=len(commands),coverage={n:d['selection'] for n,d in reports.items()})
        print('P6.1.2 scalars: PASS',len(commands),'commands')
    finally:write('results.json',result)
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',type=Path,required=True);p.add_argument('--out',type=Path);p.add_argument('--probe',type=Path);a=p.parse_args()
    if a.out:a.out.mkdir(parents=True,exist_ok=False);check(a.compiler.resolve(),a.out.resolve(),a.probe)
    else:
        with tempfile.TemporaryDirectory(prefix='fisics-physical-scalars-') as t:check(a.compiler.resolve(),Path(t),a.probe)
if __name__=='__main__':main()
