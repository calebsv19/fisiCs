#!/usr/bin/env python3
"""Bounded executable adapter; graph validation precedes ESP profile execution."""
from __future__ import annotations

import argparse
import copy
import shutil
import tempfile
import sys
from pathlib import Path

import run_esp_can_simulation as sim
sys.path.insert(0, str(sim.FISICS_ROOT / 'tools'))
import aht20_declaration
from attachment_handoff import HandoffError

ROOT = sim.FISICS_ROOT
FIXTURE = ROOT / 'tests/fixtures/esp_aht20_binding_v1/binding.json'
PROFILE = 'simulation/profiles/aht20_family_v1/profile.json'
SUPPORTED_PORTS = aht20_declaration.SUPPORTED_PORTS


def validate(binding, contract, graph_bytes, profile_bytes):
    """Compatibility entry point; generic declarations are owned by tools/."""
    try:
        return aht20_declaration.validate(binding, contract, graph_bytes, profile_bytes)
    except (HandoffError, KeyError, TypeError, ValueError) as exc:
        raise sim.SimulationError(str(exc)) from exc


def project(binding, run, contract):
    sim.require(run.get('family') == 'aht20', 'wrong result family')
    routes = {r['semantic_id']: r for r in contract['routes']}
    records = []
    for index, record in enumerate(run['records']):
        signals = [s for s in record['state']['signals'] if s.get('instance') == binding['instance']]
        sim.require(len(signals) == 1 and signals[0]['sensor'] == binding['sensor'],
                    'bound sensor missing or duplicated')
        for port in binding['ports']:
            sample = signals[0][port['source_field']]
            records.append(dict(device_id=binding['device_id'], port_id=port['port_id'],
                                subject_id=port['subject_id'], source_record_index=index,
                                at_us=record['at_us'], sample=copy.deepcopy(sample),
                                physical_value=sim.physical_value(sample['raw'], routes[port['subject_id']]),
                                usable=sample['validity'] == 0 and sample['quality'] > 0))
    sim.require(bool(records), 'empty bound result')
    return records


def verify_result(root):
    """Verify retained bytes and the deterministic join without executing a model."""
    result = sim.load_json(root / 'result.json')
    sim.require(result.get('schema') == 'fisiCs.esp_bound_simulation'
                and type(result.get('schema_version')) is int
                and result['schema_version'] == 1, 'unsupported result schema')
    claims = dict(executions_completed=2, evidence='host_simulation',
                  physical_hardware=False, native_adoption='not_implemented')
    for name, expected in claims.items():
        sim.require(type(result.get(name)) is type(expected) and result[name] == expected,
                    f'unsupported retained result claim: {name}')
    sim.require(set(result['static_graphs']) == {'semantic', 'physical', 'hardware'},
                'incomplete retained graph set')
    required = {'binding.json', 'run.json', 'projection-contract.json',
                'semantic.graph.json', 'physical.graph.json', 'hardware.graph.json'}
    sim.require(required <= set(result['retained_artifacts']), 'incomplete retained manifest')
    for name, digest in result['retained_artifacts'].items():
        path = root / name
        sim.require(not Path(name).is_absolute() and '..' not in Path(name).parts
                    and path.resolve().is_relative_to(root.resolve())
                    and not path.is_symlink(), 'unsafe retained artifact path')
        sim.require(sim.sha256_path(path) == digest, f'retained artifact mismatch: {name}')
    binding = sim.load_json(root / 'binding.json')
    sim.require(binding == result['binding'] and
                sim.sha256_bytes(sim.canonical_bytes(binding)) == result['binding_sha256'],
                'retained binding mismatch')
    profile_digest = result.get('profile_sha256')
    sim.require(type(profile_digest) is str and len(profile_digest) == 64
                and all(char in '0123456789abcdef' for char in profile_digest)
                and profile_digest == binding.get('profile_sha256'),
                'retained profile digest mismatch')
    sim.require(type(binding.get('profile_id')) is str, 'retained profile ID missing')
    safe_name = binding['profile_id'].replace('.', '-')
    plan_names = [name for repeat in (1, 2) for name in (
        f'{safe_name}.{repeat}.plan.json', f'{safe_name}.{repeat}.run/plan.json')]
    sim.require(type(result.get('plan_sha256')) is str and
                all(result['retained_artifacts'].get(name) == result['plan_sha256']
                    for name in plan_names), 'retained plan digest mismatch')
    for kind, digest in result['static_graphs'].items():
        sim.require(sim.sha256_path(root / f'{kind}.graph.json') == digest, 'retained graph mismatch')
    sim.require(sim.sha256_path(root / 'run.json') == result['run_sha256'], 'retained run mismatch')
    contract = sim.load_contract(root / 'projection-contract.json')
    sim.require(project(binding, sim.load_json(root / 'run.json'), contract) == result['records'],
                'retained projection mismatch')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--esp-work-root', type=Path)
    parser.add_argument('--compiler', type=Path)
    parser.add_argument('--verify', type=Path, help='read-only retained result verification')
    parser.add_argument('--binding', type=Path, default=FIXTURE)
    parser.add_argument('--out', type=Path, help='new retained result directory')
    args = parser.parse_args()
    if args.verify:
        verify_result(args.verify)
        print('AHT20 retained binding=PASS model_execution=NONE')
        return
    sim.require(args.esp_work_root is not None and args.compiler is not None and args.out is not None,
                'execution requires --esp-work-root, --compiler and --out')
    esp = args.esp_work_root.resolve()
    sim.require(not args.out.exists(), 'output exists; choose a new retained result directory')
    before = sim.git(esp, 'status', '--porcelain=v2', '--branch')
    contract = sim.load_contract(sim.DEFAULT_FIXTURE_ROOT / 'contract.json')
    receipt = sim.run_pass2(esp, args.compiler.resolve())
    sim.validate_static_binding(contract, receipt)
    graphs = {k: (sim.PASS2_BUILD / f'{k}.graph.json').read_bytes()
              for k in ('semantic', 'physical', 'hardware')}
    binding = sim.load_json(args.binding)
    validate(binding, contract, graphs, (esp / PROFILE).read_bytes())
    profile = next(p for p in contract['profiles'] if p['profile_id'] == binding['profile_id'])
    # Publish only after execution and all checks succeed. Existing results are never overwritten.
    with tempfile.TemporaryDirectory(prefix='fisics-aht20-binding-') as temp:
        stage = Path(temp)
        run, execution = sim.execute_profile_twice(esp, profile, stage, stage)
        records = project(binding, run, contract)
        sim.require(sim.sha256_path(esp / PROFILE) == binding['profile_sha256'],
                    'profile changed during execution')
        sim.require(before == sim.git(esp, 'status', '--porcelain=v2', '--branch'),
                    'external ESP tree changed')
        result = dict(schema='fisiCs.esp_bound_simulation', schema_version=1,
                      binding=binding, binding_sha256=sim.sha256_bytes(sim.canonical_bytes(binding)),
                      static_graphs=contract['static_graphs'],
                      profile_sha256=binding['profile_sha256'],
                      run_sha256=execution['run_sha256'], plan_sha256=execution['plan_sha256'],
                      executions_completed=2, records=records,
                      evidence='host_simulation', physical_hardware=False,
                      native_adoption='not_implemented')
        args.out.mkdir(parents=True, exist_ok=False)
        for kind, data in graphs.items():
            (args.out / f'{kind}.graph.json').write_bytes(data)
        # Preserve backend byte identity; canonical reserialization changes its digest.
        shutil.copy2(stage / 'environment-aht20-v1.run.json', args.out / 'run.json')
        sim.canonical_write(args.out / 'binding.json', binding)
        sim.canonical_write(args.out / 'static-receipt.json', receipt)
        sim.canonical_write(args.out / 'projection-contract.json', contract)
        # Preserve the backend-owned immutable execution/consumer artifacts, not
        # just our projection. Their original embedded provenance is unchanged.
        for item in stage.iterdir():
            if item.is_dir():
                shutil.copytree(item, args.out / item.name)
            else:
                shutil.copy2(item, args.out / item.name)
        result['retained_artifacts'] = {
            str(p.relative_to(args.out)): sim.sha256_path(p)
            for p in sorted(args.out.rglob('*')) if p.is_file()
        }
        sim.canonical_write(args.out / 'result.json', result)
        verify_result(args.out)
    print(f'AHT20 executable binding=PASS executions=2 ports=2 records={len(records)} output={args.out}')


if __name__ == '__main__':
    try:
        main()
    except (sim.SimulationError, KeyError, TypeError, ValueError, OSError) as exc:
        raise SystemExit(f'AHT20 executable binding: FAIL: {exc}') from exc
