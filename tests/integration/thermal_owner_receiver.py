"""Current read-only gas inputs exercise the adopted receiver boundary."""
from pathlib import Path
import hashlib,json

def qualify(a,out,run):
    out.mkdir();host=a.host_root;shared=a.core_root.parents[1];physics=a.physics_root
    gas=[physics/'src/tools/physics_sim_heated_face.c']+[physics/f'src/app/{n}.c' for n in ['cfd_open_atmosphere3d','cfd_open_atmosphere3d_box','cfd_open_atmosphere3d_box_preconditioner','cfd_open_atmosphere3d_lateral','cfd_passive3d','cfd_cartesian3d','cfd_sparse_mg','cfd_memory']]
    core=[a.core_root/'src'/(n+'.c') for n in ['core_thermal','core_thermal_rod','core_thermal_material','core_thermal_material_rod']]+[shared/'core/core_sim/src'/(n+'.c') for n in ['core_sim_time','core_sim_schedule','core_sim_exchange']]
    app=[host/('src/'+p+'.c') for p in ['experiment/participant','coupling/inert','coupling/receiver']]
    def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
    fixture=host/'examples/inert_receiver_fire_face.json';value=json.loads(fixture.read_text())
    headers=[p for folder in [physics/'include/app',physics/'src/tools',a.core_root/'include',shared/'core/core_sim/include',host/'include/thermal_sim',host/'src/coupling'] for p in folder.glob('*.h')]
    pins={str(p):sha(p) for p in gas+core+app+headers+[fixture,physics/'VERSION']}
    producer={str(p):sha(p) for p in gas+[h for h in headers if h.is_relative_to(physics)]+[physics/'VERSION']}
    ids=dict(SOURCE=hashlib.sha256(json.dumps(pins,sort_keys=True).encode()).hexdigest(),FIXTURE=sha(fixture),PRODUCER=hashlib.sha256(json.dumps(producer,sort_keys=True).encode()).hexdigest(),SUPPORT=hashlib.sha256(json.dumps(value['comparison_gas_support'],sort_keys=True,separators=(',',':')).encode()).hexdigest())
    flags=['-O2','-std=c11','-ffp-contract=off','-Wall','-Wextra','-Werror',*['-I'+str(p) for p in [host/'include',host/'src/coupling',physics/'include',physics/'src/tools',a.core_root/'include',shared/'core/core_sim/include']],*['-DTHERMAL_RECEIVER_'+k+'_ID="'+v+'"' for k,v in ids.items()]]
    for name in ['test_receiver','receiver_weak_repro']:
        exe=out/name;run(name+'-build',['clang',*flags,*gas,*core,*app,host/'tests'/(name+'.c'),'-lm','-o',exe]);run(name+'-run',[exe])
    assert all(sha(Path(k))==v for k,v in pins.items()),'receiver/producer source drift'
    return dict(passed=True,inputs=pins,producer_inputs=producer,scope='bounded existing receiver; PhysicsSim read-only; no new inter-domain feedback claim')
