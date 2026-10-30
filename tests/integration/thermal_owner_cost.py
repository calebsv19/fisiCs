"""Same matched cost budgets as the existing solver qualification lanes."""
from pathlib import Path
import json

def qualify(a,out,run):
    root=Path(__file__).resolve().parents[2];core=a.core_root;host=a.host_root;shared=core.parents[1]
    args=['-DCORE_THERMAL_PHYSICAL=1','-ffp-contract=off','-I'+str(root/'include'),'-I'+str(core/'include'),'-I'+str(shared/'core/core_sim/include'),'-I'+str(shared/'shape/external/cjson'),'-I'+str(host/'include'),'-I'+str(host/'src'),'-I'+str(a.physics_root/'include'),'-I'+str(a.physics_root/'src/tools')]
    sources=[dict(id=p.stem,source=str(p),args=args) for p in sorted((core/'src').glob('*.c'))]
    strict=[]
    for fn,p in [('ct_exchange',core/'src/core_thermal.c'),('ct_pair_delta',core/'src/core_thermal.c'),('ct_enthalpy',core/'src/core_thermal_material.c'),('ct_material_rhs',core/'src/core_thermal_material_rod.c'),('ct_exchange_power',host/'src/experiment/participant.c'),('ct_receiver_change',host/'src/coupling/receiver.c')]:
        strict.append(dict(id=fn,function=fn,source=str(p),args=args,physical=True,permissions=['literal-assumptions']))
    dependencies=[p for folder in [core/'src',core/'include',core/'physics'] for p in folder.glob('*') if p.is_file()]
    manifest=a.out/'cost-manifest.json';manifest.write_text(json.dumps(dict(physical_policy=str(core/'physics/thermal_policy.json'),translation_units=sources,strict_functions=strict,dependencies=list(map(str,dependencies))),indent=2))
    run('cost-qualification',['python3','-B',root/'tests/performance/measure_overlay_cost.py','--compiler',a.compiler,'--manifest',manifest,'--budget',root/'tests/performance/solver_overlay_budget.json','--out',out],cwd=root)
    result=json.loads((out/'results.json').read_text());assert result['passed'];return result
