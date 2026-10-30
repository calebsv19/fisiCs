"""Finite importer refusal, physical-reference and native transaction controls.

This is test code. Import admission proves ledger consistency; only the actual
producer rerun below compares the supplied numbers with the declared model.
"""
import copy
from pathlib import Path
import shutil
import re
from electrothermal_binding import encoded, load, require, sha

MANIFEST_IDS = ('unknown-component', 'wrong-layer', 'duplicate-allocation',
                'supply-component', 'authored-layer-power', 'authored-cycle-power', 'wrong-plan')
LEDGER_IDS = ('gap', 'overlap', 'future-support', 'missing-tail', 'rc-profile',
              'negative-loss', 'nonfinite-loss', 'port-as-loss', 'changed-model')
ATOMIC_IDS = ('batch-zero', 'batch-prefix', 'repeat-prefix')
SPLITS = (0, 1, 2, 9, 10, 47, 72, 73, 99)
STATE_KEYS = ('energy_j', 'temperature_k', 'face_transfer_j', 'source_j', 'sink_j',
              'stored_change_j', 'boundary_into_j', 'completed_steps', 'time_num', 'time_den')


def physical_state(native):
    state = native.call('inspect')['state']
    # Diagnostic text is allowed to explain a failure. Accepted numerical state,
    # scenario, sample count and revision must remain unchanged.
    return {key:value for key,value in state.items() if key != 'last_native_diagnostic'}


def model_reference(ledger, setup, producer):
    fresh = producer.execute(producer.prepare(setup))['ledger']
    require(encoded(ledger) == encoded(fresh), 'independent producer/model reference mismatch')


def qualify(a, out, producer, heat, failures):
    out.mkdir(); controls = []; atomic = []; restarts = []
    source_case = a.out/'runtime/scheduled'
    original_manifest = load(source_case/'manifest.json')
    original_source = (source_case/'source.json').read_bytes()
    ledger = heat.ledger(original_source)
    accepted = {str(p):sha(p) for p in (a.out/'runtime').rglob('*') if p.is_file()}

    def refuse(label, call, reason, destination=None):
        try:
            call()
        except (ValueError, FileExistsError, FileNotFoundError) as error:
            require(reason in str(error), label+': wrong refusal layer: '+str(error))
            require(destination is None or not destination.exists(), label+': published failed output')
            row = dict(id=label, expected_reason=reason, error=str(error), no_output=destination is not None)
            controls.append(row); failures.append(dict(id='adversarial-'+label, error=str(error)))
        else:
            raise ValueError('missing refusal: '+label)

    for label, edit, reason in [
        ('unknown-component', lambda m:m['allocations'][0].update(component_id='missing'), 'component allocation'),
        ('wrong-layer', lambda m:m['allocations'][0].update(layer_id='missing'), 'unknown target layer'),
        ('duplicate-allocation', lambda m:m['allocations'].append(copy.deepcopy(m['allocations'][0])), 'component allocation'),
        ('supply-component', lambda m:m['allocations'][0].update(component_id='rail.source.ideal_source'), 'component allocation'),
        ('authored-layer-power', lambda m:m['scenario']['layers'][0].update(power_w=1), 'double count'),
        ('authored-cycle-power', lambda m:m['scenario']['power_schedule'][0].update(power_w=[1]), 'double count'),
        ('wrong-plan', lambda m:m.update(expected_plan_id='0'*64), 'unexpected source plan')]:
        manifest = copy.deepcopy(original_manifest); edit(manifest)
        path = out/(label+'.json'); path.write_bytes(encoded(manifest)); dest = out/(label+'-output')
        refuse(label, lambda:heat.execute(path,a.native,dest),reason,dest)

    def port_loss(value):
        c = next(c for c in value['intervals'][0]['components'] if c['kind']=='source_shunt')
        c['dissipated_energy_j'] = c['energy_in_j']

    for label, edit, reason in [
        ('gap', lambda v:v['intervals'][1].update(begin_us=v['intervals'][1]['begin_us']+1), 'coverage'),
        ('overlap', lambda v:v['intervals'][1].update(begin_us=v['intervals'][1]['begin_us']-1), 'coverage'),
        ('future-support', lambda v:v['intervals'][-1].update(end_us=v['plan']['duration_us']+1), 'integer'),
        ('missing-tail', lambda v:v['intervals'].pop(), 'incomplete horizon'),
        ('rc-profile', lambda v:v.update(schema='codework_rc_ledger_v1'), 'unsupported ledger'),
        ('negative-loss', lambda v:v['intervals'][0]['components'][2].update(dissipated_energy_j=-1), 'negative dissipation'),
        ('nonfinite-loss', lambda v:None, 'nonfinite'),
        ('port-as-loss', port_loss, 'electrical balance/integration'),
        ('changed-model', lambda v:v['plan'].update(model='unsupported_storage_model'), 'electrical profile')]:
        bad = copy.deepcopy(ledger); edit(bad)
        # Rehash support IDs and the source digest so a byte mismatch cannot
        # accidentally stand in for the intended semantic refusal.
        bad['plan_id'] = heat.identity(bad['plan'])
        for row in bad['intervals']:
            row['plan_id'] = bad['plan_id']
            row['interval_id'] = heat.identity(dict(plan=bad['plan_id'],begin_us=row['begin_us'],end_us=row['end_us']))
        raw = encoded(bad)
        if label == 'nonfinite-loss':
            raw = re.sub(rb'("dissipated_energy_j":)[0-9eE.+-]+',rb'\1NaN',raw,count=1)
        source = out/(label+'-source.json'); source.write_bytes(raw)
        manifest = copy.deepcopy(original_manifest)
        manifest.update(source_path=str(source),source_sha256=sha(source))
        path = out/(label+'.json'); path.write_bytes(encoded(manifest)); dest = out/(label+'-output')
        refuse(label,lambda:heat.execute(path,a.native,dest),reason,dest)

    # A complete, balanced ledger can be consistently scaled yet disagree with
    # the actual fixed circuit. Import admission cannot authenticate that model.
    wrong = copy.deepcopy(ledger)
    for row in wrong['intervals']:
        for key in ('source_power_w','dissipated_power_w','balance_residual_w'): row[key] *= 2
        for component in row['components']:
            for key in component:
                if key.endswith(('_j','_w')): component[key] *= 2
    for key in ('known_source_energy_j','known_dissipated_energy_j','balance_residual_j'): wrong['totals'][key] *= 2
    for component in wrong['totals']['components']:
        for key in component:
            if key.endswith('_j'): component[key] *= 2
    witness = out/'balanced-wrong-model.json'; witness.write_bytes(encoded(wrong))
    manifest = copy.deepcopy(original_manifest); manifest.update(source_path=str(witness),source_sha256=sha(witness))
    heat.prepare(manifest,witness.read_bytes(),witness)  # Explicit positive admission control.
    setup = load(source_case/'source.json')['prepared']['setup']
    (out/'producer-setup.json').write_bytes(encoded(setup))
    model_reference(ledger,setup,producer)
    refuse('balanced-wrong-model',lambda:model_reference(wrong,setup,producer),'producer/model reference mismatch')

    # Real material bound: first candidate is valid, second exceeds max_k.
    # No injected failure, altered solver tolerance or replacement thermal law.
    single = a.out/'runtime/authored'; manifest = load(single/'manifest.json')
    manifest['scenario']['layers'][0]['max_k'] = 293.1504
    path = out/'bounded-material.json'; path.write_bytes(encoded(manifest))
    prepared = heat.prepare(manifest,(single/'source.json').read_bytes(),single/'source.json')
    n = heat.Native(a.native)
    try:
        n.call('replace',dict(scenario=prepared['scenario']))
        for label, steps, accepted_steps in [('batch-zero',2,0),('batch-prefix',2,1),('repeat-prefix',1,1)]:
            if label == 'batch-prefix':
                n.call('step',dict(steps=1))
            before_dir = out/(label+'-before'); after_dir = out/(label+'-after')
            n.call('checkpoint_save',dict(path=str(before_dir)))
            before = physical_state(n)
            refuse(label,lambda:n.call('step',dict(steps=steps)),'refused_accepted_state_retained')
            require(physical_state(n) == before,label+': numerical state/revision changed')
            require(before['completed_steps']==accepted_steps,'accepted prefix control')
            n.call('checkpoint_save',dict(path=str(after_dir)))
            before_trace=load(before_dir/'assembly.json')['payload']['trace']
            require(before_trace==load(after_dir/'assembly.json')['payload']['trace'],'failed batch changed lossless journal')
            atomic.append(dict(id=label,accepted_steps=accepted_steps,before=before_dir.name+'/assembly.json',after=after_dir.name+'/assembly.json'))
        # Recover in the same live session by loading the original, valid prefix.
        heat.bundle(single/'partial',n,True)
        n.call('step',dict(steps=63))
        reference=heat.Native(a.native)
        try:
            heat.bundle(single/'full',reference)
            require(all(n.call('inspect')['state'][k]==reference.call('inspect')['state'][k] for k in STATE_KEYS),'post-failure recovery parity')
        finally:reference.close()
        n.call('checkpoint_save',dict(path=str(out/'native-recovered')))
    finally:n.close()
    dest=out/'failed-execute-output'
    refuse('native-execute',lambda:heat.execute(path,a.native,dest),'refused_accepted_state_retained',dest)
    prefix=heat.execute(path,a.native,out/'bounded-prefix',1)
    require(prefix['completed_steps']==1,'valid first native step not published')
    dest=out/'failed-resume-output'
    refuse('native-resume',lambda:heat.execute(None,a.native,dest,resume=out/'bounded-prefix'),'refused_accepted_state_retained',dest)

    # Check immutable publication, missing original source, adapter changes and
    # recovery using private copies. Never change the real owner source/helpers.
    clone=out/'adapter-prefix';shutil.copytree(single/'partial',clone)
    marker=clone/'complete.json'; original=marker.read_bytes();record=load(marker);record['contract']='0'*64
    marker.write_bytes(encoded(record));dest=out/'adapter-output'
    refuse('adapter-restart',lambda:heat.execute(None,a.native,dest,resume=clone),'adapter changed',dest)
    marker.write_bytes(original)
    heat.execute(None,a.native,out/'adapter-recovered',resume=clone)
    source=single/'source.json'; absent=source.with_suffix('.held');source.rename(absent)
    try:
        dest=out/'missing-source-output'
        refuse('missing-original-source',lambda:heat.execute(None,a.native,dest,resume=single/'partial'),'No such file',dest)
    finally:absent.rename(source)
    heat.execute(None,a.native,out/'source-recovered',resume=single/'partial')
    dest=out/'existing';dest.mkdir();sentinel=dest/'keep';sentinel.write_bytes(b'original destination\n')
    refuse('existing-destination',lambda:heat.execute(single/'manifest.json',a.native,dest),'File exists')
    require(list(dest.iterdir())==[sentinel] and sentinel.read_bytes()==b'original destination\n','existing destination modified')

    expected=load(source_case/'full/result.json')['payload']['trace']
    for split in SPLITS:
        prefix=out/('split-'+str(split));resumed=out/('resume-'+str(split))
        heat.execute(source_case/'manifest.json',a.native,prefix,split)
        heat.execute(None,a.native,resumed,resume=prefix)
        trace=load(resumed/'result.json')['payload']['trace']
        require(trace==expected,'exact scheduled restart trace mismatch at '+str(split))
        restarts.append(dict(split=split,prefix=prefix.name,resumed=resumed.name))
    require(all(sha(p)==digest for p,digest in accepted.items()),'adversarial controls changed accepted records/source')
    report=dict(schema='fisics.electrothermal_adversarial.v1',passed=True,controls=controls,
                atomic=atomic,restarts=restarts,model_witness=dict(importer_admitted=True,model_reference_rejected=True),
                accepted_records_unchanged=True,recovery_parity=True)
    (out/'report.json').write_bytes(encoded(report))
    summary=dict(report=str((out/'report.json').relative_to(a.out)),sha256=sha(out/'report.json'),
                 refusal_controls=len(controls),restart_splits=list(SPLITS),atomic_controls=len(atomic))
    verify(a.out,summary,a,heat)
    return summary


def verify(root, summary, a, heat):
    path=Path(root)/'adversarial/report.json'
    require(summary==dict(report='adversarial/report.json',sha256=sha(path),refusal_controls=25,
                          restart_splits=list(SPLITS),atomic_controls=3),'adversarial qualification denominator/fingerprint')
    report=load(path);folder=path.parent
    require(report['schema']=='fisics.electrothermal_adversarial.v1' and report['passed'] is True,'adversarial schema/state')
    expected=set(MANIFEST_IDS+LEDGER_IDS+ATOMIC_IDS+('balanced-wrong-model','native-execute','native-resume','adapter-restart','missing-original-source','existing-destination'))
    require(len(report['controls'])==len(expected) and {r['id'] for r in report['controls']}==expected,'adversarial control coverage')
    require(report['accepted_records_unchanged'] is True and report['recovery_parity'] is True,'adversarial recovery')
    for row in report['controls']:
        require(row['expected_reason'] in row['error'], 'wrong adversarial refusal layer')
        require(row['no_output'] is (row['id'] not in ATOMIC_IDS+('balanced-wrong-model','existing-destination')), 'no-output proof denominator')
    require(report['model_witness']==dict(importer_admitted=True,model_reference_rejected=True),'model proof layer')
    import resistor_authoring as producer
    good=heat.ledger((Path(root)/'runtime/scheduled/source.json').read_bytes());setup=load(folder/'producer-setup.json')
    model_reference(good,setup,producer)
    wrong=heat.ledger((folder/'balanced-wrong-model.json').read_bytes())
    require(encoded(wrong)!=encoded(producer.execute(producer.prepare(setup))['ledger']),'wrong model witness lost')
    require(len(report['atomic'])==3 and [r['id'] for r in report['atomic']]==list(ATOMIC_IDS),'atomic coverage')
    for row,steps in zip(report['atomic'],(0,1,1)):
        before=load(folder/row['before'])['payload']['trace'];after=load(folder/row['after'])['payload']['trace']
        require(before==after and before['next_step']==row['accepted_steps']==steps,'atomic journal changed')
    require([r['split'] for r in report['restarts']]==list(SPLITS),'restart coverage')
    recovery_trace=load(Path(root)/'runtime/authored/full/result.json')['payload']['trace']
    require(load(folder/'native-recovered/assembly.json')['payload']['trace']==recovery_trace, 'native recovery trace parity')
    for name in ('adapter-recovered','source-recovered'):
        heat.bundle(folder/name)
        require(load(folder/name/'result.json')['payload']['trace']==recovery_trace, 'restart recovery trace parity')
    expected_trace=load(Path(root)/'runtime/scheduled/full/result.json')['payload']['trace']
    for row in report['restarts']:
        _,_,record=heat.bundle(folder/row['prefix'])
        require(record['completed_steps']==row['split'],'restart prefix time')
        heat.bundle(folder/row['resumed'])
        require(load(folder/row['resumed']/'result.json')['payload']['trace']==expected_trace,'restart trace parity')
