"""Actual producer/importer/native acceptance and finite runtime refusal controls."""
import copy
import math
from pathlib import Path
from electrothermal_binding import encoded, load, require, sha


def refusal(label, call, rows):
    try:
        call()
    except ValueError as error:
        rows.append(dict(id=label, error=str(error)))
    else:
        raise ValueError('missing refusal: '+label)


def qualify(a, out, producer, heat):
    out.mkdir(); rows = []; failures = []
    base = load(a.host_root/'examples/external_heat.json')
    single = dict(producer.DEFAULT, duration_us=1000000, switch_open_ms=0, switch_close_ms=0,
                  source_step_start_ms=0, source_step_end_ms=0)
    for name, setup, raw in [('authored', single, False), ('raw', single, True),
                             ('scheduled', dict(producer.DEFAULT, duration_us=10000000), False),
                             ('boundary-loss', dict(single, duration_us=10000000), False)]:
        folder = out/name; folder.mkdir()
        export = producer.execute(producer.prepare(setup))
        source = folder/'source.json'; source.write_bytes(encoded(export['ledger'] if raw else export))
        ledger = heat.ledger(source.read_bytes())
        model = a.esp_root/'simulation/scenarios/ina219_system_v1'
        require(ledger['plan']['model_sources'] == {n:sha(model/n) for n in heat.MODEL_FILES}, 'producer model source drift')
        manifest = copy.deepcopy(base)
        manifest.update(source_path=str(source), source_sha256=sha(source), expected_plan_id=ledger['plan_id'])
        manifest['scenario'].update(step_num=1, step_den=10 if name in ('scheduled','boundary-loss') else 100)
        manifest['scenario']['layers'][0]['cells'] = 1
        if name in ('authored','raw'):
            manifest['allocations'] = manifest['allocations'][:1]
        if name == 'boundary-loss':
            manifest['scenario']['ends'] = [dict(kind=2,temperature_k=293.15,h_w_m2_k=h,contact_m2_k_w=0) for h in (5,10)]
        path = folder/'manifest.json'; path.write_bytes(encoded(manifest))
        prepared = heat.prepare(manifest, source.read_bytes(), source)
        full = heat.execute(path, a.native, folder/'full')
        partial = heat.execute(path, a.native, folder/'partial', 37)
        resumed = heat.execute(None, a.native, folder/'resumed', resume=folder/'partial')
        for key in ['energy_j','temperature_k','face_transfer_j','source_j','sink_j']:
            require(full['state'][key] == resumed['state'][key], 'restart numerical mismatch')
        state = full['state']; deposited = prepared['receipt']['allocated_energy_j']
        require(abs(state['source_j']-deposited) <= 1e-10 and abs(state['stored_change_j']-deposited+state['sink_j']-state['boundary_into_j']) <= 1e-10, 'accepted ledger mismatch')
        require(state['sink_j'] == 0, 'unexpected sink')
        require(source.read_bytes() == encoded(export['ledger'] if raw else export), 'producer source mutation')
        # Fresh replay is observation-only and preserves exact source-engine identity.
        n = heat.Native(a.native)
        try:
            heat.bundle(folder/'full', n)
            before = n.call('inspect')['state']
            refusal(name+'-replay-step', lambda: n.call('step', {'steps':1}), failures)
            require(n.call('inspect')['state'] == before, 'replay refusal changed state')
        finally:
            n.close()
        rows.append(dict(bundle=str((folder/'full').relative_to(a.out)), source_sha256=sha(source),
                         producer_plan_id=ledger['plan_id'], circuit_sha256=ledger['plan']['circuit_sha256'],
                         receipt_sha256=sha(folder/'full/receipt.json'), scenario_sha256=prepared['receipt']['scenario_sha256'],
                         allocations=prepared['receipt']['allocations'], accepted_steps=state['completed_steps'],
                         accepted_source_j=state['source_j'], restart_split=partial['completed_steps']))
        if name == 'authored':
            selected = (path, folder, manifest, source)
    path, folder, manifest, source = selected
    for label, edit in [
        ('wrong-layer', lambda m: m['allocations'][0].update(layer_id='missing')),
        ('supply-component', lambda m: m['allocations'][0].update(component_id='rail.source.ideal_source')),
        ('duplicate-allocation', lambda m: m['allocations'].append(copy.deepcopy(m['allocations'][0]))),
        ('authored-power', lambda m: m['scenario']['layers'][0].update(power_w=1)),
        ('wrong-plan', lambda m: m.update(expected_plan_id='0'*64))]:
        bad = copy.deepcopy(manifest); edit(bad)
        refusal(label, lambda: heat.prepare(bad, source.read_bytes(), source), failures)
    for label, edit in [
        ('rc-profile', lambda v: v.update(schema='codework_rc_ledger_v1')),
        ('storage', lambda v: v['intervals'][0]['components'][0].update(storage_change_j=1)),
        ('missing-support', lambda v: v['intervals'][0].update(begin_us=1)),
        ('port-not-loss', lambda v: v['intervals'][0]['components'][2].update(dissipated_energy_j=v['intervals'][0]['components'][2]['energy_in_j']))]:
        bad = copy.deepcopy(heat.ledger(source.read_bytes())); edit(bad); raw = encoded(bad)
        m = dict(manifest, source_sha256=heat.sha(raw))
        refusal(label, lambda: heat.prepare(m, raw, source), failures)
    # Changed bytes refuse before publishing an experiment; preserve accepted outputs.
    hashes = {str(p):sha(p) for p in (out/'authored/full').rglob('*') if p.is_file()}
    original = source.read_bytes(); source.write_bytes(original+b' ')
    try:
        refusal('source-drift', lambda: heat.execute(path, a.native, out/'must-not-publish'), failures)
        require(not (out/'must-not-publish').exists(), 'failed import published output')
        refusal('restart-source-drift', lambda: heat.execute(None, a.native, out/'must-not-resume', resume=folder/'partial'), failures)
        require(not (out/'must-not-resume').exists(), 'failed resume published output')
    finally:
        source.write_bytes(original)
    require(all(sha(p) == h for p,h in hashes.items()), 'refusal changed accepted records')
    # Exercise old observation replay and strict checkpoint restart without relabelling.
    old = heat.execute(path, a.legacy_native, out/'legacy-full')
    heat.execute(path, a.legacy_native, out/'legacy-partial', 37)
    oldhash = {str(p):sha(p) for name in ['legacy-full','legacy-partial'] for p in (out/name).rglob('*') if p.is_file()}
    require(old['state']['engine'] != rows_engine(a, heat), 'historical helper must have a distinct engine')
    n = heat.Native(a.native)
    try:
        heat.bundle(out/'legacy-full', n)
        require(n.call('inspect')['state']['engine'] == old['state']['engine'], 'old replay relabelled')
        before = n.call('inspect')['state']
        refusal('old-replay-step', lambda: n.call('step', {'steps':1}), failures)
        require(n.call('inspect')['state'] == before, 'old replay state changed')
    finally:
        n.close()
    n = heat.Native(a.native)
    try:
        before = n.call('inspect')['state']
        refusal('old-checkpoint-engine', lambda: heat.bundle(out/'legacy-partial', n, True), failures)
        require(n.call('inspect')['state'] == before, 'checkpoint refusal state changed')
    finally:
        n.close()
    n = heat.Native(a.legacy_native)
    try:
        heat.bundle(out/'legacy-full', n)
    finally:
        n.close()
    resumed = heat.execute(None, a.legacy_native, out/'legacy-resumed', resume=out/'legacy-partial')
    require(all(resumed['state'][k] == old['state'][k] for k in ['energy_j','temperature_k','face_transfer_j','source_j','sink_j']), 'legacy restart mismatch')
    require(all(sha(p) == h for p,h in oldhash.items()), 'old records changed')
    return rows, failures


def rows_engine(a, heat):
    n = heat.Native(a.native)
    try:
        return n.meta['state']['engine']
    finally:
        n.close()
