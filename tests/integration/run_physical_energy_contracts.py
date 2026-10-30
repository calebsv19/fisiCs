#!/usr/bin/env python3
"""Bounded scalar energy forms: aggregation, explicit transfers and exact v7 evidence."""
import argparse,copy,hashlib,json,os,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
FIX=ROOT/'tests/fixtures/physical_semantics'
def check(compiler,probe,flow,out):
    commands=[];cases=[];result={'passed':False,'commands':commands,'cases':cases,'compiler_sha256':hashlib.sha256(compiler.read_bytes()).hexdigest()}
    env={**os.environ,'FISICS_MAX_PROCS':'0','PYTHONDONTWRITEBYTECODE':'1'}
    env.pop('FISICS_OVERLAY',None);env.pop('DISABLE_CODEGEN',None)
    def write(n,x):p=out/n;p.write_text(x if isinstance(x,str) else json.dumps(x,indent=2)+'\n');return p
    def run(n,args,code=0):
        p=subprocess.run(list(map(str,args)),cwd=out,env=env,capture_output=True,text=True,timeout=120);write(n+'.log',p.stdout+p.stderr)
        commands.append(dict(id=n,command=list(map(str,args)),exit=p.returncode,expected=code));assert p.returncode==code,(n,p.returncode,(p.stdout+p.stderr)[-3000:]);return p.stdout
    catalog=json.loads((FIX/'catalog.json').read_text());write('catalog.json',catalog)
    policy=json.loads((FIX/'policy.json').read_text());policy.update(schema_version=7,policy_id='pilot.energy.v7',operation_rules=[],representations=[],shape_helpers=[],frame_contexts=[],reference_points=[],frames=[],frame_transforms=[],energy_forms=['energy.thermal','energy.chemical','energy.kinetic','energy.mixed'],energy_transfers=[dict(id='transfer.chemical_to_thermal',source_form='energy.chemical',target_form='energy.thermal',process='process.combustion')])
    def rule(op,a,b,k):return dict(id='rule.'+op+'.'+a+'.'+(b or 'unary'),operation=op,operand_kinds=[a]+([b] if b else []),result_kind=k,reference_role='linear',origin_policy='none' if op in ['multiply','divide'] else 'preserve' if op=='fabs' else 'same',provenance='inputs')
    for op in ['multiply','divide']:policy['operation_rules'].append(rule(op,'quantity.energy','quantity.ratio','quantity.energy'))
    policy['operation_rules'] += [rule('multiply','quantity.ratio','quantity.energy','quantity.energy'),rule('fabs','quantity.energy',None,'quantity.energy'),rule('fmax','quantity.energy','quantity.energy','quantity.energy')]
    path=write('policy.json',policy);flags=['-I'+str(ROOT/'include'),'--overlay=physics-semantics','--physical-semantics-policy',path]
    def E(form=None):return 'FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.energy")'+(f' FISICS_ENERGY_FORM("energy.{form}")' if form else '')
    H='#include <fisics/extensions.h>\n';C=E('chemical');T=E('thermal');K=E('kinetic');M=E('mixed');G=E();R='FISICS_DIM(dimensionless) FISICS_QUANTITY_KIND("quantity.ratio")'
    transfer=f'double convert(double a {C}) {T} FISICS_ENERGY_TRANSFER("transfer.chemical_to_thermal");\n'
    def case(n,body,error=0,accepted=True,trust=False,form=None):
        src=write(n+'.c',H+body+'\n');diag=out/(n+'.diags.json');rec=out/(n+'.json')
        run(n+'_source',[compiler,*flags,'--dump-sema','--emit-diags-json',diag,src],int(bool(error)))
        ds=json.loads(diag.read_text())['diagnostics']
        if error:assert any(d['code_id']==error for d in ds),(n,ds)
        if error in [4317,4315]:assert not any(d['code_id']==4112 for d in ds)
        args=[compiler,*flags,'--physical-semantics-function','check','--physical-semantics-strict','--emit-physical-semantics-json',rec]
        if trust:args+=['--physical-semantics-allow-declared-boundaries']
        run(n+'_strict',args+[src],int(bool(error) or not accepted))
        d=json.loads(rec.read_text());assert d['schema_version']==7 and len(d)==18 and d['selection']['accepted']==(not error and accepted)
        assert all('energy_form' in e['fact'] for e in d['expressions'])
        if form:assert any(e['fact']['energy_form']=='energy.'+form for e in d['expressions']),n
        cases.append(dict(id=n,error=error,accepted=d['selection']['accepted'],selection=d['selection']))
        return src,d
    try:
        native=run('model_allocation_sweep',[probe,path,path]);result['native']=json.loads(native)
        for n,mut,code in [
          ('duplicate_form',lambda p:p['energy_forms'].append(p['energy_forms'][0]),4303),
          ('missing_mixed',lambda p:p['energy_forms'].remove('energy.mixed'),4303),
          ('invalid_form',lambda p:p['energy_forms'].append('Bad name'),4303),
          ('unknown_source',lambda p:p['energy_transfers'][0].update(source_form='energy.absent'),4303),
          ('identity_transfer',lambda p:p['energy_transfers'][0].update(target_form='energy.chemical'),4303),
          ('mixed_transfer',lambda p:p['energy_transfers'][0].update(source_form='energy.mixed'),4303),
          ('duplicate_transfer',lambda p:p['energy_transfers'].append(copy.deepcopy(p['energy_transfers'][0])),4303),
          ('extra_transfer_field',lambda p:p['energy_transfers'][0].update(efficiency=1),4303),
          ('missing_process',lambda p:p['energy_transfers'][0].pop('process'),4303),
          ('invalid_process',lambda p:p['energy_transfers'][0].update(process='Bad name'),4303),
          ('missing_table',lambda p:p.pop('energy_transfers'),4301),
          ('forms_limit',lambda p:p.update(energy_forms=['energy.f'+str(i) for i in range(129)]),4304),
          ('transfers_limit',lambda p:p.update(energy_transfers=p['energy_transfers']*257),4304),
          ('legacy_v6_refuses',lambda p:p.update(schema_version=6),4301),
        ]:
            p=copy.deepcopy(policy);mut(p);f=write(n+'.policy.json',p);run(n,[probe,path,f,code])
        p=copy.deepcopy(policy);p['energy_forms'].reverse();p['linear_kinds'].reverse();run('canonical_reorder',[probe,path,write('reordered.json',p)])
        for name,change in [('process',dict(process='process.other')),('direction',dict(source_form='energy.thermal',target_form='energy.chemical'))]:
            changed=copy.deepcopy(policy);changed['energy_transfers'][0].update(change)
            run('fingerprint_'+name,[probe,path,write(name+'_change.json',changed),'changed'])
        case('same_form_float',f'float check(float a {T},float b {T}) {T}{{return a+b;}}')
        case('native_whitespace',f'double check(double a FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.energy") [[fisics::energy_form\t("energy.chemical")]]) {T}{{return a;}}',4317,False)
        case('same_form_add',f'double check(double a {T},double b {T}) {T}{{return a+b;}}',form='thermal')
        a,ad=case('mixed_sum',f'double check(double a {T},double b {K}) {M}{{return a+b;}}',form='mixed')
        case('mixed_subtract',f'double check(double a {T},double b {K}) {M}{{return a-b;}}',form='mixed')
        case('mixed_narrow',f'double check(double a {T},double b {K}) {T}{{return a+b;}}',4317,False)
        case('mixed_then_add',f'double check(double a {T},double b {K}) {M}{{return (a+b)+a;}}',form='mixed')
        case('general_sum',f'double check(double a {T},double b {K}) {G}{{return a+b;}}',form='mixed')
        case('safe_widen',f'double check(double a {T}) {M}{{a=a+a;return a;}}')
        case('unknown_operand',f'double check(double a {T},double b {G}) {T}{{return a+b;}}',4317,False)
        case('generic_unknown_result',f'double check(double a {T},double b {G}) {G}{{return a+b;}}')
        case('conditional_mixed',f'double check(int flag,double a {T},double b {K}) {M}{{return flag?a:b;}}',form='mixed')
        case('conditional_narrow',f'double check(int flag,double a {T},double b {K}) {T}{{return flag?a:b;}}',4317,False)
        case('ratio_scale',f'double check(double a {T},double r {R}) {T}{{return a*r;}}',form='thermal')
        case('ratio_scale_reverse',f'double check(double a {T},double r {R}) {T}{{return r*a;}}')
        case('ratio_divide',f'double check(double a {T},double r {R}) {T}{{return a/r;}}')
        case('fabs_preserves','#include <math.h>\n'+f'double check(double a {T}) {T}{{return fabs(a);}}')
        case('fmax_mixed','#include <math.h>\n'+f'double check(double a {T},double b {K}) {M}{{return fmax(a,b);}}',form='mixed')
        case('relabel_assignment',f'double check(double a {C},double b {T}) {T}{{b=a;return b;}}',4317,False)
        case('cast_relabel',f'double check(double a {C}) {T}{{return (double)a;}}',4317,False)
        case('erased_form_restore',f'double check(double a {T}) {T}{{double b {G}=a;return b;}}',4317,False)
        case('untyped_restore',f'double check(double a {C}) {T}{{double b=a;return b;}}',accepted=False,trust=True)
        case('transfer',transfer+f'double check(double a {C}) {T}{{a=a+a;return convert(a);}}',trust=True,form='thermal')
        case('transfer_permission_missing',transfer+f'double check(double a {C}) {T}{{a=a+a;return convert(a);}}',accepted=False)
        case('transfer_wrong_source',transfer+f'double check(double a {K}) {T}{{return convert(a);}}',4317,False,True)
        case('transfer_mixed_source',transfer+f'double check(double a {M}) {T}{{return convert(a);}}',4317,False,True)
        case('ordinary_helper_relabel',transfer.replace(' FISICS_ENERGY_TRANSFER("transfer.chemical_to_thermal")','')+f'double check(double a {C}) {T}{{a=a+a;return convert(a);}}',4317,False,True)
        case('wrong_transfer_direction',transfer.replace(f'double a {C}',f'double a {T}')+f'double check(double a {T}) {T}{{return convert(a);}}',4312,False,True)
        case('nonenergy_form',f'double check(double a FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.torque") FISICS_ENERGY_FORM("energy.thermal")){{return a;}}',4312,False)
        case('duplicate_annotation',f'double check(double a {T} FISICS_ENERGY_FORM("energy.thermal")) {T}{{return a;}}',4312,False)
        case('unknown_form',f'double check(double a {E("absent")}) {G}{{return a;}}',4312,False)
        case('redeclaration_conflict',f'double shared {C};extern double shared {T};double check(double a {T}) {T}{{return a+a;}}',4315,False)
        # Source declarations remain isolated per declarator, and missing qualifiers never infer from spelling.
        case('declarator_isolation',f'double x {T}, y {C};double check(double a {T}) {T}{{x=a+a;return x;}}')
        # No body authentication: an annotated implementation still cannot return its input under a changed form.
        case('transfer_body_relabel',f'double convert(double a {C}) {T} FISICS_ENERGY_TRANSFER("transfer.chemical_to_thermal"){{return a;}}double check(double a {C}) {T}{{return convert(a);}}',4317,False,True)
        case('array_form',f'double values[3] {T};double check(double a {T}) {T}{{return a+a;}}',4312,False)
        case('integer_form',f'int stored {T};double check(double a {T}) {T}{{return a+a;}}',4312,False)
        case('transfer_float',transfer.replace('double convert','float convert').replace('double a','float a')+f'double check(double a {C}) {T}{{return convert(a);}}',4312,False,True)
        case('transfer_extra_parameter',transfer.replace(f'double a {C}',f'double a {C},int mode')+f'double check(double a {C}) {T}{{return convert(a,0);}}',4312,False,True)
        case('transfer_unknown_input',transfer+f'double check(double a {G}) {T}{{return convert(a);}}',4317,False,True)
        case('compound_mixed_narrow',f'double check(double a {T},double b {K}) {T}{{a+=b;return a;}}',4317,False)
        case('compound_same_form',f'double check(double a {T},double b {T}) {T}{{a+=b;return a;}}')
        off=write('overlay_off.c',H+f'double v {T};')
        run('overlay_off',[compiler,'-I'+str(ROOT/'include'),'--dump-sema',off],1)
        # Exact receipts and cross-TU declaration consistency, including transfer identity.
        one=write('tu_a.c',H+transfer+f'double check(double a {T}) {T}{{return a+a;}}')
        two=write('tu_b.c',H+transfer+f'double check(double a {T}) {T}{{return a+a;}}')
        for i,src in enumerate([one,two]):run('tu_'+str(i),[compiler,*flags,'--physical-semantics-function','check','--physical-semantics-strict','--emit-physical-semantics-json',out/f'tu_{i}.json',src])
        run('tu_consistency',[compiler,'--physical-semantics-check',out/'tu_0.json',out/'tu_1.json'])
        base=json.loads((out/'tu_1.json').read_text())
        for n,mut in [
          ('tamper_unknown_form',lambda d:d['expressions'][0]['fact'].update(energy_form='energy.absent')),
          ('tamper_model',lambda d:d['energy_model']['transfers'][0].update(conservation='proved')),
          ('tamper_transfer_signature',lambda d:next(x for x in d['declarations'] if x['name']=='convert').update(energy_transfer=None)),
          ('tamper_extra_root',lambda d:d.update(invented=True)),
        ]:
            d=copy.deepcopy(base);mut(d);run(n,[compiler,'--physical-semantics-check',out/'tu_0.json',write(n+'.json',d)],1)
        original=two.read_bytes();two.write_text(two.read_text()+'\n');run('stale_source',[compiler,'--physical-semantics-check',out/'tu_0.json',out/'tu_1.json'],1);two.write_bytes(original)
        if flow:
            native_source=write('native_state.c',a.read_text().replace('#include <fisics/extensions.h>','#include "'+str(ROOT/'include/fisics/extensions.h')+'"'))
            run('state_allocation_sweep',[flow,path,native_source,'sweep'])
        # An explicit bookkeeping pilot: implementation compiled as ordinary C,
        # separately checked caller, exact binary fractions and independent oracle.
        impl=write('transfer_impl.c','double convert(double a){return 0.75*a;}\ndouble retained(double a){return 0.25*a;}\n')
        caller=write('energy_caller.c',H+transfer+f'double check(double a {C},double b {T},double c {K}) {M}{{a=a+a;return convert(a)+b+c;}}\n')
        driver=write('energy_driver.c','#include <stdio.h>\ndouble check(double,double,double);double convert(double);double retained(double);\nint main(void){for(int i=0;i<16;i++){double c=i*4,t=i+2,k=i*2;printf("%.17g %.17g %.17g %.17g\\n",check(c,t,k),convert(c),retained(c),convert(c)+retained(c));}return 0;}\n')
        oracle=[[1.5*(i*4)+(i+2)+i*2,0.75*(i*4),0.25*(i*4),i*4] for i in range(16)]
        run('pilot_caller_strict',[compiler,*flags,'--physical-semantics-function','check','--physical-semantics-strict','--physical-semantics-allow-declared-boundaries','--emit-physical-semantics-json',out/'pilot_caller.json',caller])
        values=[]
        for cc,ci in [('clang','clang'),('fisics','fisics'),('clang','fisics'),('fisics','clang')]:
            tag=cc+'_'+ci;objs=[]
            for name,src,tool in [('caller',caller,cc),('impl',impl,ci),('driver',driver,cc)]:
                obj=out/(tag+'_'+name+'.o');objs.append(obj)
                opts=[*flags] if tool=='fisics' and name=='caller' else ['-I'+str(ROOT/'include')]
                run(tag+'_'+name,[compiler if tool=='fisics' else 'clang',*opts,'-c',src,'-o',obj])
            exe=out/tag;run(tag+'_link',['clang',*objs,'-o',exe]);observed=[[float(v) for v in line.split()] for line in run(tag+'_run',[exe]).splitlines()]
            assert observed==oracle,(tag,observed);values.append(dict(variant=tag,values=64,exact=True))
        # A changed transfer body remains statically possible but the runtime oracle detects it.
        mutant=write('mutant_impl.c',impl.read_text().replace('0.75','0.5'));obj=out/'mutant.o'
        run('mutant_compile',[compiler,'-c',mutant,'-o',obj]);exe=out/'mutant'
        run('mutant_link',['clang',out/'fisics_fisics_caller.o',out/'fisics_fisics_driver.o',obj,'-o',exe]);observed=[[float(v) for v in line.split()] for line in run('mutant_run',[exe]).splitlines()]
        assert observed!=oracle and any(row[3]!=i*4 for i,row in enumerate(observed))
        result['numerical_pilot']=dict(variants=values,oracle='exact binary-rational bookkeeping; 3/4 transferred and 1/4 retained',mutant_detected=True,body_proof='ordinary C implementation tested separately; declared transfer interface only')
        result['passed']=True
    finally:write('result.json',result)
    print(json.dumps(dict(passed=True,cases=len(cases),commands=len(commands))))
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',type=Path,required=True);p.add_argument('--probe',type=Path,required=True);p.add_argument('--flow-probe',type=Path);p.add_argument('--out',type=Path);a=p.parse_args()
    if a.out:a.out.mkdir(parents=True,exist_ok=False);check(a.compiler.resolve(),a.probe.resolve(),a.flow_probe.resolve() if a.flow_probe else None,a.out.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix='fisics-energy-') as d:check(a.compiler.resolve(),a.probe.resolve(),a.flow_probe.resolve() if a.flow_probe else None,Path(d))
if __name__=='__main__':main()
