"""Actual thermal C source, six variants, old C consumers and numerical mutants."""
from pathlib import Path
import hashlib,json,subprocess,re
TESTS=[('core_thermal_test',['core_thermal']),('core_thermal_rod_test',['core_thermal_rod']),('core_thermal_material_test',['core_thermal_material','core_thermal_material_rod']),('core_thermal_faint_test',['core_thermal_material','core_thermal_material_rod'])]

def qualify(a,out,run):
    root=Path(__file__).resolve().parents[2];core=a.core_root;baseline=a.baseline_core
    variants=[('original-clang','clang',baseline,[]),('original-fisics',a.compiler,baseline,[]),('adopted-clang','clang',core,[]),('portable-clang','clang',core,['-DCORE_THERMAL_PHYSICAL=1']),('units',a.compiler,core,['--overlay=physics-units','-DCORE_THERMAL_UNITS=1']),('physical',a.compiler,core,['--overlay=physics-semantics','-DCORE_THERMAL_PHYSICAL=1','--physical-semantics-policy',core/'physics/thermal_policy.json'])]
    objects={};results={};layouts={};out.mkdir()
    reference=out/'reference_profiles.c'
    reference.write_text(r'''#include "core_thermal_material_rod.h"
#include <assert.h>
int main(void){
 CoreThermalRod rod={0};rod.count=1;rod.area_m2=2;rod.width_m[0]=3;rod.capacity_j_per_m3_k[0]=4;rod.conductivity_w_per_m_k[0]=5;
 double t[1]={10},power[1]={12};CoreThermalRodState state;CoreThermalRodStep step;
 assert(core_thermal_rod_init(&rod,t,&state)==0 && state.energy_j[0]==240);
 assert(core_thermal_rod_step(&rod,&state,power,.25,&step)==0);
 assert(step.state.energy_j[0]==243 && step.temperature_k[0]==10.125 && step.source_j==3 && step.sink_j==0 && step.balance_residual_j==0);
 CoreThermalMaterialRod material={0};material.count=1;material.area_m2=2;material.width_m[0]=3;
 material.material[0]=(CoreThermalMaterial){5,100,4,4,5,5};
 assert(core_thermal_material_rod_init(&material,t,&state)==0 && state.energy_j[0]==120);
 CoreThermalMaterialRodStep next;assert(core_thermal_material_rod_step(&material,&state,power,.25,&next)==0);
 assert(next.result.state.energy_j[0]==123 && next.result.temperature_k[0]==10.125 && next.result.source_j==3 && next.result.balance_residual_j==0);
 return 0;
}''')
    # Same old public headers and test drivers call every new object variant.
    for name,tool,source,flags in variants:
        work=out/name;work.mkdir();obj={};inc=['-I'+str(source/'include'),'-I'+str(root/'include')]
        for src in sorted((source/'src').glob('*.c')):
            target=work/(src.stem+'.o');run(name+'-'+src.stem,[tool,*flags,*inc,'-ffp-contract=off','-c',src,'-o',target]);obj[src.stem]=target
        objects[name]=obj
        for test,parts in TESTS:
            testobj=work/(test+'.o');exe=work/test
            run(name+'-'+test+'-driver',['clang','-std=c11','-ffp-contract=off','-I'+str(baseline/'include'),'-c',baseline/'tests'/(test+'.c'),'-o',testobj])
            run(name+'-'+test+'-link',['clang',testobj,*[obj[k] for k in parts],'-lm','-o',exe])
            results[name+'/'+test]=run(name+'-'+test+'-run',[exe])
        refexe=work/'reference_profiles';run(name+'-reference-link',['clang','-I'+str(baseline/'include'),reference,*obj.values(),'-lm','-o',refexe]);run(name+'-reference-run',[refexe])
        run(name+'-faint-oracle',['python3','-B',a.host_root/'tests/test_faint_solid.py',work/'core_thermal_faint_test'])
        layout=work/'layout.c';layout.write_text('#include "core_thermal_material_rod.h"\n#include <stdio.h>\n#include <stddef.h>\nint main(void){printf("%zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu\\n",sizeof(CoreThermalModel),sizeof(CoreThermalState),sizeof(CoreThermalStep),sizeof(CoreThermalRod),sizeof(CoreThermalRodState),sizeof(CoreThermalRodStep),sizeof(CoreThermalMaterial),sizeof(CoreThermalProperties),sizeof(CoreThermalMaterialRod),sizeof(CoreThermalMaterialRodStep),offsetof(CoreThermalRodStep,face_transfer_j),offsetof(CoreThermalMaterialRod,material));return 0;}\n')
        exe=work/'layout';run(name+'-layout-compile',[tool,*flags,*inc,layout,'-o',exe]);layouts[name]=run(name+'-layout-run',[exe])
    assert len(set(layouts.values()))==1,layouts
    assert results['original-clang/core_thermal_faint_test']==results['adopted-clang/core_thermal_faint_test'],'faint output parity'
    # Numerical sign, precision and compensated-energy mutations must fail actual C tests.
    header=(core/'include/core_thermal_physical.h').read_text();mutations=[
      ('face-sign','return dt * g * (left - right);','return -dt * g * (left - right);','core_thermal_rod_test',['core_thermal_rod']),
      ('capacity-factor','return cv * a * dx;','return cv * a * dx * 2;','reference_profiles',['core_thermal','core_thermal_rod','core_thermal_material','core_thermal_material_rod']),
      ('enthalpy-law','cv0 + half * f * (cv1 - cv0)','cv0 + f * (cv1 - cv0)','core_thermal_material_test',['core_thermal_material','core_thermal_material_rod']),
      ('face-budget','return old + external + left - right;','return old + external + left + right;','core_thermal_material_test',['core_thermal_material','core_thermal_material_rod']),
      ('pair-sign','out->second = w1 * (p0 + p1) * dt - pair;','out->second = w1 * (p0 + p1) * dt + pair;','core_thermal_test',['core_thermal']),
      ('lost-increment','return dt * g * ((a - b) + (da - db));','return dt * g * (a - b);','core_thermal_faint_test',['core_thermal_material','core_thermal_material_rod']),
      ('lost-compensation','return e - v * h + dt * (p + gl * (left - old) + gr * (right - old));','return dt * (p + gl * (left - old) + gr * (right - old));','core_thermal_faint_test',['core_thermal_material','core_thermal_material_rod'])]
    controls=[]
    for name,old,new,test,parts in mutations:
        assert old in header,(name,old)
        work=out/('mutant-'+name);work.mkdir();(work/'core_thermal_physical.h').write_text(header.replace(old,new));obs=[]
        for part in parts:
            obj=work/(part+'.o');run(name+'-'+part+'-compile',['clang','-std=c11','-ffp-contract=off','-I'+str(work),'-I'+str(core/'include'),'-c',core/'src'/(part+'.c'),'-o',obj]);obs.append(obj)
        exe=work/'test';run(name+'-link',['clang','-I'+str(baseline/'include'),(reference if test=='reference_profiles' else baseline/'tests'/(test+'.c')),*obs,'-lm','-o',exe])
        # An abnormal signal or assertion is expected; never accept a timeout as detection.
        run(name+'-refusal',[exe],expected='failure');controls.append(name)
    return dict(passed=True,variants=[v[0] for v in variants],layout=layouts,objects={n:{k:str(v) for k,v in o.items()} for n,o in objects.items()},numerical_mutations=controls)
