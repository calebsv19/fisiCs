"""Finite electrical/thermal meaning and source/receipt joins; no solver equations."""
import gzip
import hashlib
import json
from pathlib import Path

SCHEMA = 'fisics.electrothermal_binding.v1'
FIELDS = {
    'components[].dissipated_power_w': ['quantity.power', 'watt', None],
    'components[].dissipated_energy_j': ['quantity.energy', 'joule', 'energy.thermal'],
    'native.power_schedule[].power_w[]': ['quantity.power', 'watt', None],
    'native.source_j': ['quantity.energy', 'joule', 'energy.thermal'],
    'native.step_seconds': ['quantity.duration', 'second', None],
    'native.temperature_k[]': ['quantity.temperature.absolute', 'kelvin', None],
}
CALLS = {'assembly_cycle_power': ['ct_share_power', 'ct_remaining_power'],
         'assembly_cycle_integral': ['ct_cycle_energy']}
PROFILE = dict(schemas=['codework_dc_resistor_ledger_v1', 'esp_resistor_result_v1'],
               model='source_shunt_fixed_resistor_v1', origin='modeled',
               storage='memoryless_no_electrical_storage', storage_change_j=0,
               component_kinds=['branch_shunt', 'load_resistor', 'source_shunt'],
               timebase=dict(origin='run_start', ticks_per_second=1000000),
               support='half_open_exact_overlap_uniform_rate', receiver_interval_limit=64,
               ledger='source_j', packet_ledger=False, authored_power='zero',
               mapping='explicit_component_to_layer', unallocated='reported_not_deposited')
EVIDENCE = dict(c_kernels='selected_actual_tu_checked',
                ingress='declared_validated_external_profile',
                python='independent_runtime_not_c_body_proof',
                controllers='raw_not_whole_owner_strict', authentication=False)


def require(condition, reason):
    if not condition:
        raise ValueError(reason)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def encoded(value):
    return (json.dumps(value, sort_keys=True, separators=(',', ':'), allow_nan=False)+'\n').encode()


def load(path):
    def pairs(items):
        result = {}
        for key, value in items:
            require(key not in result, 'duplicate JSON key')
            result[key] = value
        return result
    text = gzip.decompress(Path(path).read_bytes()).decode() if str(path).endswith('.gz') else Path(path).read_text()
    return json.loads(text, object_pairs_hook=pairs,
                      parse_constant=lambda _: require(False, 'nonfinite JSON'))


def validate(binding, catalog):
    require(set(binding) == {'schema', 'profile', 'fields', 'calls', 'evidence'}, 'binding keys')
    require(binding['schema'] == SCHEMA, 'binding schema')
    # Exact finite v1 meaning: correctly rehashing a relabelled W/J/profile is insufficient.
    require(encoded(binding['profile']) == encoded(PROFILE), 'unsupported physical profile')
    require(binding['fields'] == FIELDS, 'physical field meaning mismatch')
    require(binding['calls'] == CALLS, 'actual call contract mismatch')
    require(binding['evidence'] == EVIDENCE, 'evidence boundary mismatch')
    kinds = {x['id']: x for x in catalog['quantity_kinds']}
    dimensions = {'quantity.power': [2,1,-3,0,0,0,0,0],
                  'quantity.energy': [2,1,-2,0,0,0,0,0],
                  'quantity.duration': [0,0,1,0,0,0,0,0],
                  'quantity.temperature.absolute': [0,0,0,0,1,0,0,0]}
    for kind, dim in dimensions.items():
        require(kind in kinds and kinds[kind]['dimension_vector'] == dim, 'catalog kind/dimension mismatch')


def engine_inputs(a, identity):
    """Read the owner's manifest against live source, including its complete input set."""
    host = a.host_root; shared = a.core_root.parents[1]
    paths = []
    for folder in ['src/app','src/runtime','src/experiment','src/workbench','src/assembly','src/coupling','include']:
        paths += [p for p in (host/folder).rglob('*') if p.suffix in ('.c', '.h')]
    paths += [host/'src/main.c', host/'VERSION']
    paths += [host/'tools'/n for n in ['esp_import.py','electrical_identity.py','heated_study.py','heated_member.py','electrical_heat.py']]
    for folder in ['core/core_thermal', 'core/core_sim']:
        names = ['core_sim_time', 'core_sim_schedule', 'core_sim_exchange'] if folder.endswith('core_sim') else None
        for part, suffix in [('src', '.c'), ('include', '.h')]:
            paths += [p for p in (shared/folder/part).glob('*'+suffix) if names is None or p.stem in names]
        paths.append(shared/folder/'VERSION')
    paths += [shared/'shape/external/cjson'/n for n in ['cJSON.c','cJSON.h']]
    inputs = {str(p.relative_to(host)) if p.is_relative_to(host) else 'shared/'+str(p.relative_to(shared)):sha(p) for p in paths}
    require(inputs == identity['inputs'], 'native engine source manifest drift')
    digest = hashlib.sha256(json.dumps(inputs, sort_keys=True).encode())
    compiler = identity['compiler'].encode(); flags = identity['flags']
    require(set(flags) == {'CPPFLAGS','CFLAGS','LDFLAGS','LDLIBS'}, 'engine flag schema')
    digest.update(compiler+json.dumps(flags, sort_keys=True).encode())
    baseline = load(host/'config/semantic_engine_baseline.json')
    legacy = inputs == baseline['inputs'] and hashlib.sha256(compiler+json.dumps(flags, sort_keys=True).encode()).hexdigest() == baseline['toolchain_sha256']
    require(identity['engine_id'] == (baseline['legacy_engine_id'] if legacy else digest.hexdigest()), 'native engine identity mismatch')
    return paths


def source_inputs(a, identity):
    root = Path(__file__).resolve().parents[2]
    fixtures = root/'tests/fixtures/physical_semantics/electrothermal'
    paths = engine_inputs(a, identity)
    paths += [a.compiler, a.native, a.native_identity, a.legacy_native, a.binding]
    paths += list((a.core_root/'physics').glob('*.json'))
    model = a.esp_root/'simulation/scenarios/ina219_system_v1'
    paths += list(model.rglob('*.py')) + [model/'examples/source_shunt_resistor.json']
    paths += [a.host_root/'examples/external_heat.json', a.host_root/'scripts/engine_identity.py',
              a.host_root/'config/semantic_engine_baseline.json', root/'include/fisics/extensions.h']
    paths += [Path(__file__).with_name(n) for n in ['run_electrothermal_boundary_contracts.py',
              'electrothermal_binding.py', 'electrothermal_runtime.py', 'electrothermal_adversarial.py', 'thermal_owner_contracts.py',
              'electrothermal_diagnostics.py', 'electrothermal_diagnostic_checks.py', 'run_electrothermal_diagnostics.py']]
    paths += list(fixtures.glob('*.c')) + list(fixtures.glob('*.json'))
    paths += list((a.physics_root/'include').rglob('*.h')) + list((a.physics_root/'src/tools').glob('*.h'))
    return {str(p):sha(p) for p in paths}


def kernel_join(inventory, binding):
    require(inventory['passed'] and inventory['whole_owner_strict_claim'] is False, 'whole-owner claim')
    rows = []
    for caller, kernels in sorted(binding['calls'].items()):
        facts = [x for x in inventory['functions'] if x['name'] == caller and x['file'].endswith('/src/assembly/cycle.c')]
        require(len(facts) == 1, 'missing actual caller')
        for kernel in kernels:
            require(kernel in facts[0]['calls'], 'kernel no longer called')
            selected = [x for x in inventory['coverage'] if x.get('label') == kernel and x['mode'] == 'physical']
            require(len(selected) == 1, 'missing selected kernel receipt')
            s = selected[0]['selection']
            require(s['accepted'] and s['checked'] > 0 and s['blocked'] == 0, 'kernel not checked')
            require(s['required'] == s['checked']+s['permitted_trust'], 'kernel denominator')
            rows.append(dict(caller=caller, body_sha256=facts[0]['body_sha256'], kernel=kernel,
                             selection=s, report='contracts/'+kernel+'-physical.json.gz'))
    return rows


def verify_receipt(path, a, heat):
    """Verify bytes AND finite meaning. A digest is provenance, not authentication."""
    path = Path(path); root = path.parent; value = load(path)
    require(value['schema'] == 'fisics.electrothermal_acceptance.v3' and value['passed'] is True, 'acceptance schema/state')
    require(value['whole_owner_strict_claim'] is False and value['evidence'] == EVIDENCE, 'evidence claim')
    require(value['inputs'] == source_inputs(a, load(a.native_identity)), 'missing/stale source pins')
    required = {'binding.json','native-identity.json','contracts/inventory.json','diagnostic-contract.json','diagnostics/summary.md', 'adversarial/report.json'} | {'contracts/'+k+'-physical.json.gz' for kernels in CALLS.values() for k in kernels}
    fixtures = Path(__file__).resolve().parents[2]/'tests/fixtures/physical_semantics/electrothermal'
    required |= {f.stem+'.json' for f in fixtures.glob('*.c')}
    require(required <= set(value['artifacts']), 'missing artifact pins')
    for file, digest in value['inputs'].items():
        require(sha(file) == digest, 'bound input drift: '+file)
    for file, digest in value['artifacts'].items():
        target = (root/file).resolve()
        require(target.is_relative_to(root.resolve()) and sha(target) == digest, 'artifact drift: '+file)
    for fixture in fixtures.glob('*.c'):
        selection = load(root/(fixture.stem+'.json'))['selection']
        require(selection['strict'] is True and selection['enumeration_complete'] is True and
                selection['accepted'] is (not fixture.stem.endswith('_fail')), 'static fixture acceptance mismatch')
    binding = load(root/'binding.json'); validate(binding, load(a.core_root/'physics/thermal_catalog.json'))
    require(value['binding_sha256'] == sha(root/'binding.json'), 'binding fingerprint')
    identity = load(root/'native-identity.json'); engine_inputs(a, identity)
    require(value['native_engine'] == identity['engine_id'] and value['native_sha256'] == sha(a.native), 'native join mismatch')
    n = heat.Native(a.native)
    try:
        require(n.meta['state']['engine'] == value['native_engine'], 'native executable/manifest mismatch')
    finally:
        n.close()
    inventory = load(root/'contracts/inventory.json')
    require(value['kernel_bindings'] == kernel_join(inventory, binding), 'kernel binding mismatch')
    for row in value['kernel_bindings']:
        require(load(root/row['report'])['selection'] == row['selection'], 'kernel report mismatch')
    from electrothermal_adversarial import verify as verify_adversarial
    verify_adversarial(root, value['adversarial_qualification'], a, heat)
    from electrothermal_diagnostics import CONTRACT_PATH, summary_markdown, verify_report
    require(sha(root/'diagnostic-contract.json') == sha(CONTRACT_PATH), 'diagnostic contract fingerprint')
    diagnostic_reports = []; full_samples = partial_samples = 0
    for case in value['runtime']:
        folder = root/case['bundle']
        require(folder.resolve().is_relative_to(root.resolve()), 'bundle outside receipt')
        _, prepared, record = heat.bundle(folder)
        source = heat.ledger((folder/'source.json').read_bytes())
        require(source['plan_id'] == case['producer_plan_id'] and source['plan']['circuit_sha256'] == case['circuit_sha256'], 'producer join mismatch')
        require(case['source_sha256'] == sha(folder/'source.json'), 'source join mismatch')
        require(case['receipt_sha256'] == sha(folder/'receipt.json') and case['scenario_sha256'] == prepared['receipt']['scenario_sha256'], 'runtime receipt join mismatch')
        require(case['allocations'] == prepared['receipt']['allocations'], 'allocation join mismatch')
        result = load(folder/'result.json')['payload']['trace']
        require(result['engine'] == value['native_engine'] and record['completed_steps'] == prepared['scenario']['steps'], 'runtime engine/horizon mismatch')
        require(case['accepted_steps'] == record['completed_steps'], 'accepted time join mismatch')
        model = a.esp_root/'simulation/scenarios/ina219_system_v1'
        require(source['plan']['model_sources'] == {name:sha(model/name) for name in heat.MODEL_FILES}, 'producer source provenance mismatch')
        n = heat.Native(a.native)
        try:
            heat.bundle(folder, n)
            state = n.call('inspect')['state']
            require(case['accepted_source_j'] == state['source_j'], 'accepted energy join mismatch')
        finally:
            n.close()
        for field in ['diagnostics','partial_diagnostics']:
            require(case[field] in value['artifacts'] and sha(root/case[field]) == case[field+'_sha256'], 'diagnostic artifact join mismatch')
        report = verify_report(load(root/case['diagnostics']),folder,a.native,heat)
        part = verify_report(load(root/case['partial_diagnostics']),folder.parent/'partial',a.native,heat)
        require(report['available_completed_steps'] == case['accepted_steps'] == report['reported_through_step'], 'full diagnostic support mismatch')
        require(part['available_completed_steps'] == case['restart_split'] == part['reported_through_step'] and
                part['samples'] == report['samples'][:case['restart_split']+1], 'partial diagnostic support mismatch')
        diagnostic_reports += [(folder.parent.name,report),(folder.parent.name+' at37',part)]
        full_samples += len(report['samples']); partial_samples += len(part['samples'])
    require({x['bundle'] for x in value['runtime']} == {'runtime/authored/full','runtime/raw/full','runtime/scheduled/full','runtime/boundary-loss/full'} and len(value['runtime']) == 4, 'runtime profile coverage')
    require(value['diagnostic_qualification'] == dict(passed=True,full_history_samples=full_samples,
            partial_history_samples=partial_samples,cases=4,summary='diagnostics/summary.md'), 'diagnostic qualification denominator')
    require((root/'diagnostics/summary.md').read_text() == summary_markdown(diagnostic_reports), 'diagnostic summary meaning mismatch')
    return value
