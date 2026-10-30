#!/usr/bin/env python3
"""Bounded physical-kind extension for actual dynamic cross-block helper bodies.
The source owner is read-only. Raw matrix/DOF restoration remains refused.
"""
from pathlib import Path
import argparse,hashlib,json,os,subprocess
ROOT=Path(__file__).resolve().parents[2]

def prepare(core,out):
    out.mkdir()
    header=core/'src/core_structural_dynamic_energy.h'
    text=header.read_text()
    for direction,vi,vj,kind in [('tr','DE_V','DE_W','translation_rotation'),('rt','DE_W','DE_V','rotation_translation')]:
        lines=[line for line in text.splitlines() if ' cs_de_kinetic_'+direction+'(' in line or ' cs_de_damping_'+direction+'(' in line]
        expected_kinetic=f'static inline double CS_PHYSICAL_DIM(energy) cs_de_kinetic_{direction}(double vi CS_PHYSICAL_DIM({"length/time" if direction=="tr" else "1/time"}),double mass CS_PHYSICAL_DIM(mass*length),double vj CS_PHYSICAL_DIM({"1/time" if direction=="tr" else "length/time"})){{return .5*vi*mass*vj;}}'
        left='length/time' if direction=='tr' else '1/time'
        right='1/time' if direction=='tr' else 'length/time'
        expected_damping=f'static inline double CS_PHYSICAL_DIM(energy) cs_de_damping_{direction}(double dt CS_PHYSICAL_DIM(time),double alpha CS_PHYSICAL_DIM(1/time),double oi CS_PHYSICAL_DIM({left}),double vi CS_PHYSICAL_DIM({left}),double mass CS_PHYSICAL_DIM(mass*length),double oj CS_PHYSICAL_DIM({right}),double vj CS_PHYSICAL_DIM({right})){{return dt*alpha*.25*(oi+vi)*mass*(oj+vj);}}'
        assert lines==[expected_kinetic,expected_damping], 'owner helper shape changed; review required'
        kinetic=f'static inline double DE_E cs_de_kinetic_{direction}(double vi {vi},double mass CS_PHYSICAL_DIM(mass*length) DE_KIND("quantity.inertia_{kind}"),double vj {vj}){{double half DE_R=.5;return half*vi*mass*vj;}}'
        damping=f'static inline double DE_E cs_de_damping_{direction}(double dt DE_T,double alpha DE_A,double oi {vi},double vi {vi},double mass CS_PHYSICAL_DIM(mass*length) DE_KIND("quantity.inertia_{kind}"),double oj {vj},double vj {vj}){{double quarter DE_R=.25;return dt*alpha*quarter*(oi+vi)*mass*(oj+vj);}}'
        for old,new in zip(lines,[kinetic,damping]):
            assert old.count('cs_de_kinetic_')+old.count('cs_de_damping_')==1
            text=text.replace(old,new)
    (out/header.name).write_text(text)
    (out/'core_structural_frame_motion_integrate.c').write_bytes((core/'src/core_structural_frame_motion_integrate.c').read_bytes())
    policy=json.loads((core/'physics/dynamic_energy_policy.json').read_text())
    policy['policy_id']='structural.dynamic.coupling.p9'
    catalog={'schema':'fisiCs.physical_contract_catalog','schema_version':1,'catalog_id':'structural.dynamic.coupling.p9','quantity_kinds':[], 'contracts':[]}
    for kind in ['translation_rotation','rotation_translation']:
        identity='quantity.inertia_'+kind
        catalog['quantity_kinds'].append(dict(id=identity,dimension_system='fisiCs.si_dim8.v1',dimension_vector=[1,1,0,0,0,0,0,0],dimension_text='mass*length'))
        policy['linear_kinds'].append(identity)
        for operand,result in [('quantity.velocity','quantity.rotational_momentum'),('quantity.angular_rate','quantity.momentum')]:
            policy['operation_rules'].append(dict(id='rule.coupling.'+kind+'.'+operand.split('.')[-1],operation='multiply',operand_kinds=[operand,identity],result_kind=result,reference_role='linear',origin_policy='none',provenance='inputs'))
    cat=out/'catalog.json';cat.write_text(json.dumps(catalog,indent=2)+'\n')
    policy['catalog_paths']=[str(core/'physics/frame_catalog.json'),str(core/'physics/dynamic_energy_catalog.json'),str(cat)]
    (out/'policy.json').write_text(json.dumps(policy,indent=2)+'\n')
    return out/'policy.json'

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for n in ['compiler','core-root','out']:p.add_argument('--'+n,type=Path,required=True)
    p.add_argument('--baseline',type=Path);a=p.parse_args()
    compiler=a.compiler.resolve();core=a.core_root.resolve();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False)
    env={**os.environ,'FISICS_MAX_PROCS':'0','PYTHONDONTWRITEBYTECODE':'1'};env.pop('FISICS_OVERLAY',None)
    sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
    result={'passed':False,'commands':[],'selections':[],'inputs':{str(p):sha(p) for p in [compiler,Path(__file__),*sorted((core/'src').glob('*.h')),core/'src/core_structural_frame_motion_integrate.c',*sorted((core/'physics').glob('*.json'))]}}
    def save():(out/'results.json').write_text(json.dumps(result,indent=2)+'\n')
    def run(name,args,code=0):
        cmd=list(map(str,args));q=subprocess.run(cmd,cwd=ROOT,env=env,capture_output=True,text=True,timeout=180);(out/(name+'.log')).write_text(q.stdout+q.stderr);result['commands'].append(dict(id=name,argv=cmd,exit=q.returncode,expected=code));save();assert q.returncode==code,(name,q.returncode,(q.stdout+q.stderr)[-2000:]);return q.stdout
    try:
        candidate=out/'candidate';policy=prepare(core,candidate)
        inc=['-I'+str(ROOT/'include'),'-I'+str(candidate),'-I'+str(core/'src'),'-I'+str(core/'include'),'-I'+str(core.parent/'core_base/include')]
        flags=['--overlay=physics-semantics','--physical-semantics-policy',policy,'-DCORE_STRUCTURAL_PHYSICAL_SEMANTICS=1',*inc]
        source=candidate/'core_structural_frame_motion_integrate.c'
        if a.baseline:run('baseline-refuses-new-policy',[a.baseline.resolve(),*flags,'--physical-semantics-function','cs_de_kinetic_tr','--physical-semantics-strict','--emit-physical-semantics-json',out/'baseline.json',source],1)
        for fn in ['cs_de_kinetic_tr','cs_de_kinetic_rt','cs_de_damping_tr','cs_de_damping_rt','cs_de_kinetic','cs_de_damping','integrate']:
            ok=fn.endswith(('_tr','_rt'));report=out/(fn+'.json')
            run(fn,[compiler,*flags,'--physical-semantics-strict','--physical-semantics-allow-literal-assumptions','--physical-semantics-function',fn,'--emit-physical-semantics-json',report,source],0 if ok else 1)
            d=json.loads(report.read_text());assert d['selection']['accepted']==ok
            result['selections'].append(dict(function=fn,selection=d['selection']))
        tr='CS_PHYSICAL_DIM(mass*length) DE_KIND("quantity.inertia_translation_rotation")'
        rt='CS_PHYSICAL_DIM(mass*length) DE_KIND("quantity.inertia_rotation_translation")'
        cases={
          'valid-tr':(f'double v DE_V,double m {tr},double w DE_W','cs_de_kinetic_tr(v,m,w)',True),
          'valid-rt':(f'double w DE_W,double m {rt},double v DE_V','cs_de_kinetic_rt(w,m,v)',True),
          'swap-role':(f'double v DE_V,double m {rt},double w DE_W','cs_de_kinetic_tr(v,m,w)',False),
          'rate-is-decay':(f'double v DE_V,double m {tr},double w DE_A','cs_de_kinetic_tr(v,m,w)',False),
          'untyped-mass':('double v DE_V,double m,double w DE_W','cs_de_kinetic_tr(v,m,w)',False),
          'cast-role':(f'double v DE_V,double m {rt},double w DE_W','cs_de_kinetic_tr(v,(double)m,w)',False),
          'raw-alias':('double *v,double *m,double *w','cs_de_kinetic_tr(*v,*m,*w)',False),
          'dim-only':('double v CS_PHYSICAL_DIM(length/time),double m CS_PHYSICAL_DIM(mass*length),double w CS_PHYSICAL_DIM(1/time)','cs_de_kinetic_tr(v,m,w)',False)}
        for name,(params,body,ok) in cases.items():
            path=out/(name+'.c');path.write_text('#include "core_structural_dynamic_energy.h"\ndouble DE_E check('+params+'){return '+body+';}\n')
            run(name,[compiler,*flags,'--physical-semantics-strict','--physical-semantics-allow-declared-boundaries','--physical-semantics-allow-literal-assumptions','--physical-semantics-function','check','--emit-physical-semantics-json',out/(name+'.json'),path],0 if ok else 1)
        model=json.loads(policy.read_text());cat=json.loads((candidate/'catalog.json').read_text())
        for name,field,value in [('wrong-dim','dimension_vector',[0,1,0,0,0,0,0,0]),('wrong-id','id','quantity.inertia_unrecognized')]:
            changed=json.loads(json.dumps(cat));changed['quantity_kinds'][0][field]=value;q=out/(name+'-catalog.json');q.write_text(json.dumps(changed));m=json.loads(json.dumps(model));m['catalog_paths'][-1]=str(q)
            if field=='id':m['linear_kinds']=[value if x==cat['quantity_kinds'][0]['id'] else x for x in m['linear_kinds']];m['operation_rules']=[{**x,'operand_kinds':[value if k==cat['quantity_kinds'][0]['id'] else k for k in x['operand_kinds']]} for x in m['operation_rules']]
            mp=out/(name+'-policy.json');mp.write_text(json.dumps(m));run(name,[compiler,'--overlay=physics-semantics','--physical-semantics-policy',mp,'-c',out/'valid-tr.c',*inc,'-DCORE_STRUCTURAL_PHYSICAL_SEMANTICS=1','-o',out/(name+'.o')],1);assert not (out/(name+'.o')).exists()
        missing=json.loads(json.dumps(model));missing['operation_rules']=[x for x in missing['operation_rules'] if x['id']!='rule.coupling.translation_rotation.velocity'];mp=out/'missing-rule-policy.json';mp.write_text(json.dumps(missing));run('missing-rule',[compiler,'--overlay=physics-semantics','--physical-semantics-policy',mp,*inc,'-DCORE_STRUCTURAL_PHYSICAL_SEMANTICS=1','--physical-semantics-strict','--physical-semantics-allow-literal-assumptions','--physical-semantics-function','cs_de_kinetic_tr','--emit-physical-semantics-json',out/'missing-rule.json',source],1)
        # Actual copied integration translation unit, unchanged except included private header candidate.
        for mode,tool,opts in [('clang','clang',['-std=c11','-O0','-ffp-contract=off']),('physical',compiler,flags)]:
            run(mode+'-actual-object',[tool,*opts,*inc,'-c',source,'-o',out/(mode+'.o')])
        # Units-only baseline and new physical helper use the same faithful source.
        for fn in ['cs_de_kinetic_tr','cs_de_kinetic_rt','cs_de_damping_tr','cs_de_damping_rt']:
            run('units-'+fn,[compiler,'--overlay=physics-units',*inc,'--units-coverage-strict','--units-coverage-allow-literal-assumptions','--units-coverage-function',fn,'--emit-units-coverage-json',out/('units-'+fn+'.json'),source])
        result['passed']=True
        assert all(sha(Path(n))==h for n,h in result['inputs'].items()),'source drift'
    finally:save()
    print('Dynamic coupling contracts PASS',len(result['commands']),'commands')
if __name__=='__main__':main()
