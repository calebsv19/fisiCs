"""Matched actual core and assembly TU cost; immutable existing budget."""
import json
from pathlib import Path
from electrothermal_binding import load, require


def qualify(a,work,run):
    root=Path(__file__).resolve().parents[2];shared=a.core_root.parents[1]
    args=['-DCORE_THERMAL_PHYSICAL=1','-ffp-contract=off',*['-I'+str(p) for p in [root/'include',a.core_root/'include',shared/'core/core_sim/include',shared/'shape/external/cjson',a.host_root/'include',a.host_root/'src',a.physics_root/'include',a.physics_root/'src/tools']]]
    sources=[dict(id=p.stem,source=str(p),args=args) for p in sorted((a.core_root/'src').glob('*.c'))]
    sources.append(dict(id='assembly_cycle',source=str(a.host_root/'src/assembly/cycle.c'),args=args))
    strict=[]
    for fn,p in [('ct_exchange',a.core_root/'src/core_thermal.c'),('ct_pair_delta',a.core_root/'src/core_thermal.c'),('ct_enthalpy',a.core_root/'src/core_thermal_material.c'),('ct_material_rhs',a.core_root/'src/core_thermal_material_rod.c'),('ct_exchange_power',a.host_root/'src/experiment/participant.c'),('ct_receiver_change',a.host_root/'src/coupling/receiver.c')]+[(n,a.host_root/'src/assembly/cycle.c') for n in ['ct_share_power','ct_remaining_power','ct_cycle_energy']]:
        strict.append(dict(id=fn,function=fn,source=str(p),args=args,physical=True,permissions=['literal-assumptions']))
    dependencies=[p for folder in [a.core_root/'src',a.core_root/'include',a.core_root/'physics'] for p in folder.glob('*') if p.is_file()]+[a.host_root/'src/assembly/cycle.c',a.host_root/'src/assembly/assembly.h',a.host_root/'src/experiment/participant.c',a.host_root/'src/coupling/receiver.c']
    manifest=a.out/'cost-manifest.json';manifest.write_text(json.dumps(dict(physical_policy=str(a.core_root/'physics/thermal_policy.json'),translation_units=sources,strict_functions=strict,dependencies=list(map(str,dependencies))),indent=2)+'\n')
    run('cost-qualification',['python3','-B',root/'tests/performance/measure_overlay_cost.py','--compiler',a.compiler,'--manifest',manifest,'--budget',root/'tests/performance/solver_overlay_budget.json','--out',work],cwd=root,timeout=600)
    value=load(work/'results.json');require(value['passed'],'cost qualification')
    return value
