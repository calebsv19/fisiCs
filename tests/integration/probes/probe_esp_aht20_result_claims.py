#!/usr/bin/env python3
"""Frontier probe: retained AHT20 proof labels must not be caller-upgradable."""
import copy
from pathlib import Path
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'tests/integration'))
import run_esp_aht20_binding as adapter

sim = adapter.sim
with tempfile.TemporaryDirectory(prefix='fisics-aht20-claims-') as temp:
    root = Path(temp)
    binding = sim.load_json(adapter.FIXTURE)
    contract = sim.load_contract(sim.DEFAULT_FIXTURE_ROOT / 'contract.json')
    sample = dict(raw=2000, sequence=1, validity=0, quality=1, session=1, received_ms=0)
    run = dict(family='aht20', records=[dict(at_us=0, state=dict(signals=[
        dict(instance=12, sensor='rear_left', temperature=sample, humidity=sample)]))])
    sim.canonical_write(root / 'binding.json', binding)
    sim.canonical_write(root / 'run.json', run)
    sim.canonical_write(root / 'projection-contract.json', contract)
    safe_name = binding['profile_id'].replace('.', '-')
    plan_bytes = sim.canonical_bytes(dict(profile_id=binding['profile_id'], scenario='baseline'))
    for repeat in (1, 2):
        (root / f'{safe_name}.{repeat}.plan.json').write_bytes(plan_bytes)
        run_root = root / f'{safe_name}.{repeat}.run'
        run_root.mkdir()
        (run_root / 'plan.json').write_bytes(plan_bytes)
    for kind in ('semantic', 'physical', 'hardware'):
        (root / f'{kind}.graph.json').write_bytes((sim.PASS2_BUILD / f'{kind}.graph.json').read_bytes())
    baseline = dict(schema='fisiCs.esp_bound_simulation', schema_version=1,
                    binding=binding, binding_sha256=sim.sha256_path(root / 'binding.json'),
                    run_sha256=sim.sha256_path(root / 'run.json'),
                    profile_sha256=binding['profile_sha256'],
                    plan_sha256=sim.sha256_bytes(plan_bytes),
                    static_graphs=contract['static_graphs'],
                    records=adapter.project(binding, run, contract),
                    executions_completed=2, evidence='host_simulation',
                    physical_hardware=False, native_adoption='not_implemented',
                    retained_artifacts={str(p.relative_to(root)): sim.sha256_path(p)
                                        for p in root.rglob('*') if p.is_file()})
    sim.canonical_write(root / 'result.json', baseline)
    adapter.verify_result(root)
    mutations = {
        'unsupported_integer_version': ('schema_version', 2),
        'boolean_schema_version': ('schema_version', True),
        'execution_count': ('executions_completed', 200),
        'evidence_promotion': ('evidence', 'physically_accepted'),
        'hardware_promotion': ('physical_hardware', True),
        'native_promotion': ('native_adoption', 'installed_and_accepted'),
    }
    accepted = []
    for label, (field, value) in mutations.items():
        result = copy.deepcopy(baseline)
        result[field] = value
        sim.canonical_write(root / 'result.json', result)
        try:
            adapter.verify_result(root)
        except (sim.SimulationError, KeyError, TypeError, ValueError):
            print('REJECTED', label)
        else:
            accepted.append(label)
            print('BLOCKED', label, 'unsupported retained claim accepted')
    # Current-threshold companion: an actual projection change is rejected.
    changed_projection = copy.deepcopy(baseline)
    changed_projection['records'][0]['usable'] = not changed_projection['records'][0]['usable']
    sim.canonical_write(root / 'result.json', changed_projection)
    try:
        adapter.verify_result(root)
    except sim.SimulationError:
        print('REJECTED changed_projection')
    else:
        raise AssertionError('current-threshold projection integrity regressed')
    if accepted:
        print('ESP-AHT20-RESULT-CLAIMS-001 failure_kind=harness_error severity=high '
              'origin=new_gap source_lane=integration_probe trust_layer=retained_readback '
              'owner_lane=esp_aht20_result_verifier raw_status=BLOCKED '
              'raw_detail=' + ','.join(accepted))
        raise SystemExit(1)
    print('ESP-AHT20-RESULT-CLAIMS-001 raw_status=RESOLVED')
