"""Actual native host and reciprocal old-header ABI consumers; no solver laws."""
from pathlib import Path
from electrothermal_binding import load, require, sha
from thermal_owner_numerics import TESTS


def qualify(a, work, run, numerical):
    work.mkdir(); shared=a.core_root.parents[1];root=Path(__file__).resolve().parents[2]
    build=work/'host'
    run('host-build-test',['make','-j4','BUILD_DIR='+str(build),'SHARED_ROOT='+str(shared),
                           'build','test','test-cycles','test-external-heat'],cwd=a.host_root)
    native=build/'bin/thermal_assembly_agent';identity=build/'generated/thermal_semantic_identity.json'
    # Actual native tools are retained for exact replay, unlike disposable ABI
    # hybrids whose host's Clang engine identity cannot describe overlaid objects.
    import shutil
    for src,name in [(native,'thermal_assembly_agent'),(identity,'thermal_semantic_identity.json')]:
        shutil.copy2(src,a.out/'tools'/name)
    a.native=a.out/'tools/thermal_assembly_agent';a.native_identity=a.out/'tools/thermal_semantic_identity.json'
    model=work/'model.rod';model.write_text((a.host_root/'tests/fixtures/material_contact.rod').read_text().replace('2 1 1 100','2 1 0.1 20'))
    old=work/'legacy';run('legacy-create',[a.baseline_bin,'--experiment-create',model,old,901,1,10])
    run('legacy-seven',[a.baseline_bin,'--experiment-run',old,7]);before=(old/'checkpoint.txt').read_bytes()
    run('new-refuses-old',[build/'bin/thermal_sim','--experiment-inspect',old],expected=2)
    require((old/'checkpoint.txt').read_bytes()==before,'old checkpoint changed on refusal')
    run('old-observation',[a.baseline_bin,'--experiment-inspect',old]);run('old-continuation',[a.baseline_bin,'--experiment-run',old,20])
    # Reverse direction complements every old Clang C driver already linked to
    # each six-variant core by thermal_owner_numerics.
    for name,parts in TESTS:
        obj=work/(name+'-fisics-driver.o');exe=work/(name+'-fisics-driver')
        run(name+'-reverse-compile',[a.compiler,'-ffp-contract=off','-I'+str(a.baseline_core/'include'),'-c',a.baseline_core/'tests'/(name+'.c'),'-o',obj])
        run(name+'-reverse-link',['clang',obj,*[numerical['objects']['adopted-clang'][k] for k in parts],'-lm','-o',exe])
        run(name+'-reverse-run',[exe])
    allobjs=list((build/'obj/src').rglob('*.o'))
    app=[x for x in allobjs if '/assembly/' not in str(x) and '/workbench/' not in str(x) and '/inspection/' not in str(x) and '/tests/' not in str(x)]
    sim=[build/'obj'/(n+'.o') for n in ['core_sim_time','core_sim_schedule','core_sim_exchange']]
    for mode in ['units','physical']:
        flags=['--overlay=physics-units','-DCORE_THERMAL_UNITS=1'] if mode=='units' else ['--overlay=physics-semantics','-DCORE_THERMAL_PHYSICAL=1','--physical-semantics-policy',a.core_root/'physics/thermal_policy.json']
        inc=['-I'+str(p) for p in [a.core_root/'include',shared/'core/core_sim/include',shared/'shape/external/cjson',a.host_root/'include',a.host_root/'src',root/'include']]
        participant=work/('participant-'+mode+'.o');cycle=work/('cycle-'+mode+'.o')
        run('participant-'+mode+'-compile',[a.compiler,*flags,*inc,'-ffp-contract=off','-c',a.host_root/'src/experiment/participant.c','-o',participant])
        run('cycle-'+mode+'-compile',[a.compiler,*flags,*inc,'-ffp-contract=off','-c',a.host_root/'src/assembly/cycle.c','-o',cycle])
        coreobjs=list(numerical['objects'][mode].values());exe=work/('consumer-'+mode)
        run('consumer-'+mode+'-link',['clang',*[x for x in app if x!=build/'obj/src/experiment/participant.o'],participant,*sim,*coreobjs,'-lm','-o',exe])
        assembly=work/('assembly-'+mode)
        assemblyobjs=[x for x in allobjs if x not in [build/'obj/src/main.o',build/'obj/src/experiment/participant.o',build/'obj/src/assembly/cycle.o']]
        run('assembly-'+mode+'-link',['clang',*assemblyobjs,participant,cycle,*sim,*coreobjs,build/'obj/cJSON.o','-lm','-o',assembly])
        for script in ['test_cycles.py','test_external_heat.py']:
            run('assembly-'+mode+'-'+script,['python3','-B',a.host_root/'tests'/script,assembly])
        for script in ['test_two_body.py','test_rod.py','test_material.py']:
            run('consumer-'+mode+'-'+script,['python3','-B',a.host_root/'tests'/script,exe])
    return dict(passed=True,variants=['native','units','physical'],reverse_old_header_drivers=[t[0] for t in TESTS],
                native_sha256=sha(a.native),engine=load(a.native_identity)['engine_id'],
                hybrid_engines='disposable ABI/numerical probes only; never restart provenance',
                legacy='new identity refuses unchanged; exact old helper observes and continues')
