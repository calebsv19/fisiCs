#!/usr/bin/env python3
"""Opt-in actual C/kernel and modeled fixed-resistor runtime receipt join."""
import argparse
import copy
import gzip
import json
import os
from pathlib import Path
import subprocess
import sys
from electrothermal_binding import (CALLS, EVIDENCE, FIELDS, PROFILE, SCHEMA, encoded,
                                    engine_inputs, kernel_join, load, require, sha,
                                    source_inputs, validate, verify_receipt)
from electrothermal_runtime import qualify as runtime, refusal
from electrothermal_adversarial import qualify as adversarial
from thermal_owner_contracts import qualify as contracts
from electrothermal_diagnostic_checks import qualify as diagnostic_checks
from electrothermal_diagnostics import CONTRACT_PATH, contract as diagnostic_contract

ROOT = Path(__file__).resolve().parents[2]
FIXTURES = ROOT/'tests/fixtures/physical_semantics/electrothermal'


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ['compiler', 'core-root', 'host-root', 'esp-root', 'native', 'native-identity', 'legacy-native', 'physics-root']:
        p.add_argument('--'+name, type=Path, required=True)
    p.add_argument('--binding', type=Path, default=FIXTURES/'fixed_resistor_boundary.json')
    p.add_argument('--out', type=Path)
    p.add_argument('--verify-receipt', type=Path, help='Read back an existing source-bound receipt; do not rewrite it')
    a = p.parse_args()
    require(sys.flags.optimize == 0, 'qualification requires Python assertions enabled')
    for name, value in vars(a).items():
        if isinstance(value, Path):
            setattr(a, name, value.resolve())
    sys.dont_write_bytecode = True
    sys.path[:0] = [str(a.esp_root/'simulation/scenarios/ina219_system_v1'), str(a.host_root/'tools')]
    import resistor_authoring as producer
    import electrical_heat as heat
    if a.verify_receipt:
        verify_receipt(a.verify_receipt, a, heat)
        print('PASS electrothermal receipt readback')
        return
    require(a.out is not None, '--out fresh directory required')
    binding = load(a.binding); catalog = load(a.core_root/'physics/thermal_catalog.json')
    validate(binding, catalog); diagnostic_contract()
    identity = load(a.native_identity); pins = source_inputs(a, identity)
    a.out.mkdir(parents=True, exist_ok=False)
    (a.out/'binding.json').write_bytes(encoded(binding))
    (a.out/'native-identity.json').write_bytes(encoded(identity))
    (a.out/'diagnostic-contract.json').write_bytes(CONTRACT_PATH.read_bytes())
    (a.out/'logs').mkdir(); (a.out/'tmp').mkdir()
    commands = []; env = dict(os.environ, PYTHONDONTWRITEBYTECODE='1', TMPDIR=str(a.out/'tmp'), FISICS_MAX_PROCS='0')
    env.pop('FISICS_OVERLAY', None)

    def run(name, args, expected=0, cwd=None):
        job = subprocess.run(list(map(str,args)), cwd=cwd or a.host_root, env=env,
                             capture_output=True, text=True, timeout=120)
        with gzip.open(a.out/'logs'/(name+'.log.gz'), 'wt') as log:
            log.write(job.stdout+job.stderr)
        commands.append(dict(id=name, command=list(map(str,args)), exit=job.returncode, expected=expected))
        require(job.returncode == expected, name+': unexpected status '+str(job.returncode)+' '+job.stderr[-1500:])
        return job.stdout

    try:
        inventory = contracts(a, a.out/'contracts', run)
        joins = kernel_join(inventory, binding)
        inc = ['-I'+str(a.core_root/'include'), '-I'+str(ROOT/'include')]
        flags = ['--overlay=physics-semantics', '-DCORE_THERMAL_PHYSICAL=1',
                 '--physical-semantics-policy', a.core_root/'physics/thermal_policy.json']
        for fixture in sorted(FIXTURES.glob('*.c')):
            reject = fixture.stem.endswith('_fail')
            report = a.out/(fixture.stem+'.json')
            run(fixture.stem, [a.compiler,*flags,*inc,'--physical-semantics-function','check',
                              '--physical-semantics-strict','--emit-physical-semantics-json',report,fixture], 1 if reject else 0)
            selected = load(report)['selection']
            require(selected['accepted'] is (not reject), 'fixture selection status')
            run(fixture.stem+'-ordinary-c', ['clang','-std=c11','-Werror',*inc,'-c',fixture,'-o',a.out/(fixture.stem+'.o')])
        run('producer-ledger', [sys.executable,'-B','-m','unittest','discover','-s','tests/runtime','-p','test_dc_power_ledger.py','-v'], cwd=a.esp_root)
        run('producer-authoring', [sys.executable,'-B','-m','unittest','discover','-s','tests/runtime','-p','test_resistor_authoring.py','-v'], cwd=a.esp_root)
        cases, failures = runtime(a, a.out/'runtime', producer, heat)
        adversarial_qualification = adversarial(a, a.out/'adversarial', producer, heat, failures)
        run('external-heat-regression', [sys.executable,'-B',a.host_root/'tests/test_external_heat.py',a.native])
        diagnostic_qualification = diagnostic_checks(a,a.out/'diagnostics',cases,heat,run,failures)
        native_engine = identity['engine_id']
        # Mutation controls reject finite meaning even when a changed manifest is freshly hashed.
        for label, edit in [
            ('binding-w-as-j', lambda b: b['fields']['native.source_j'].__setitem__(0,'quantity.power')),
            ('binding-wrong-kind', lambda b: b['fields']['native.source_j'].__setitem__(0,'quantity.torque')),
            ('binding-energy-form-on-power', lambda b: b['fields']['native.power_schedule[].power_w[]'].__setitem__(2,'energy.thermal')),
            ('binding-false-checked-ingress', lambda b: b['evidence'].update(ingress='c_body_checked')),
            ('binding-packet-ledger', lambda b: b['profile'].update(ledger='cell_exchange_j')),
            ('binding-rc-profile', lambda b: b['profile'].update(storage='capacitor'))]:
            changed = copy.deepcopy(binding); edit(changed)
            refusal(label, lambda: validate(changed, catalog), failures)
        changed = copy.deepcopy(identity)
        first = next(iter(changed['inputs'])); changed['inputs'][first] = '0'*64
        refusal('engine-source-drift', lambda: engine_inputs(a, changed), failures)
        changed = copy.deepcopy(identity); changed['engine_id'] = '0'*64
        refusal('engine-identity-drift', lambda: engine_inputs(a, changed), failures)
        bad_inventory = copy.deepcopy(inventory)
        for row in bad_inventory['functions']:
            if row['name'] == 'assembly_cycle_power': row['calls'] = []
        refusal('missing-real-kernel-caller', lambda: kernel_join(bad_inventory, binding), failures)
        require(all(sha(f) == h for f,h in pins.items()), 'source drift during qualification')
        # Keep the full evidence losslessly without retaining hundreds of repeated TU JSONs.
        for report in (a.out/'contracts').glob('*.json'):
            if report.name == 'inventory.json':
                continue
            with gzip.open(str(report)+'.gz', 'wb') as stream:
                stream.write(report.read_bytes())
            report.unlink()
        artifacts = {str(f.relative_to(a.out)):sha(f) for f in a.out.rglob('*') if f.is_file() and f.suffix not in ('.o',) and 'tmp' not in f.relative_to(a.out).parts}
        result = dict(schema='fisics.electrothermal_acceptance.v3', passed=True,
                      whole_owner_strict_claim=False, evidence=EVIDENCE, binding_sha256=sha(a.out/'binding.json'),
                      native_engine=native_engine, native_sha256=sha(a.native), inputs=pins,
                      artifacts=artifacts, kernel_bindings=joins, runtime=cases,
                      failures=failures, commands=commands, diagnostic_qualification=diagnostic_qualification,
                      strict_kernel_count=inventory['strict_kernel_count'], adversarial_qualification=adversarial_qualification)
        candidate = a.out/'candidate.json'; candidate.write_bytes(encoded(result))
        verify_receipt(candidate, a, heat)
        for label, edit in [
            ('joined-old-scope', lambda v: v.update(schema='fisics.electrothermal_acceptance.v2')),
            ('joined-adversarial-denominator', lambda v: v['adversarial_qualification'].update(refusal_controls=0)),
            ('joined-missing-adversarial-artifact', lambda v: v['artifacts'].pop('adversarial/report.json')),
            ('joined-engine', lambda v: v.update(native_engine='0'*64)),
            ('joined-missing-source-pin', lambda v: v['inputs'].pop(str(a.compiler))),
            ('joined-missing-artifact-pin', lambda v: v['artifacts'].pop('contracts/inventory.json')),
            ('joined-source', lambda v: v['runtime'][0].update(source_sha256='0'*64)),
            ('joined-plan', lambda v: v['runtime'][0].update(producer_plan_id='0'*64)),
            ('joined-circuit', lambda v: v['runtime'][0].update(circuit_sha256='0'*64)),
            ('joined-receipt', lambda v: v['runtime'][0].update(receipt_sha256='0'*64)),
            ('joined-allocation', lambda v: v['runtime'][0]['allocations'][0].update(layer_id='missing')),
            ('joined-accepted-energy', lambda v: v['runtime'][0].update(accepted_source_j=99)),
            ('joined-accepted-time', lambda v: v['runtime'][0].update(accepted_steps=99)),
            ('joined-duplicate-case', lambda v: v['runtime'].__setitem__(1,copy.deepcopy(v['runtime'][0]))),
            ('joined-diagnostic-sha', lambda v: v['runtime'][0].update(diagnostics_sha256='0'*64)),
            ('joined-diagnostic-support', lambda v: v['runtime'][0].update(diagnostics=v['runtime'][0]['partial_diagnostics'],diagnostics_sha256=v['runtime'][0]['partial_diagnostics_sha256'])),
            ('joined-diagnostic-denominator', lambda v: v['diagnostic_qualification'].update(full_history_samples=4)),
            ('joined-kernel', lambda v: v['kernel_bindings'][0].update(kernel='ct_heat_transfer')),
            ('joined-whole-owner', lambda v: v.update(whole_owner_strict_claim=True))]:
            changed = copy.deepcopy(result); edit(changed); candidate.write_bytes(encoded(changed))
            refusal(label, lambda: verify_receipt(candidate, a, heat), failures)
        # Validate emitted artifacts, not only the summary fields.
        original = (a.out/'binding.json').read_bytes()
        changed = copy.deepcopy(binding); changed['fields']['native.source_j'][2] = 'energy.chemical'
        (a.out/'binding.json').write_bytes(encoded(changed))
        try:
            candidate.write_bytes(encoded(result))
            refusal('artifact-bytes', lambda: verify_receipt(candidate, a, heat), failures)
            forged = copy.deepcopy(result)
            forged['binding_sha256'] = forged['artifacts']['binding.json'] = sha(a.out/'binding.json')
            candidate.write_bytes(encoded(forged))
            refusal('rehashed-wrong-meaning', lambda: verify_receipt(candidate, a, heat), failures)
        finally:
            (a.out/'binding.json').write_bytes(original)
        diagnostic_path = a.out/cases[0]['diagnostics']; original_diagnostic = diagnostic_path.read_bytes()
        changed = load(diagnostic_path); changed['samples'][37]['pending_allocated_at_sample_j'] = 0
        diagnostic_path.write_bytes(encoded(changed))
        try:
            candidate.write_bytes(encoded(result))
            refusal('diagnostic-artifact-bytes',lambda: verify_receipt(candidate,a,heat),failures)
            forged = copy.deepcopy(result)
            forged['runtime'][0]['diagnostics_sha256'] = forged['artifacts'][cases[0]['diagnostics']] = sha(diagnostic_path)
            candidate.write_bytes(encoded(forged))
            refusal('rehashed-wrong-diagnostic-meaning',lambda: verify_receipt(candidate,a,heat),failures)
        finally:
            diagnostic_path.write_bytes(original_diagnostic)
        result['failures'] = failures
        candidate.write_bytes(encoded(result)); verify_receipt(candidate, a, heat)
        os.replace(candidate, a.out/'results.json')
        print(json.dumps(dict(passed=True, commands=len(commands), strict_kernels=inventory['strict_kernel_count'],
                              kernel_bindings=len(joins), runtime_cases=len(cases), failure_controls=len(failures), diagnostic_samples=diagnostic_qualification['full_history_samples'])))
    except BaseException as error:
        (a.out/'failure.json').write_bytes(encoded(dict(passed=False, error=str(error), commands=commands)))
        raise


if __name__ == '__main__':
    try:
        main()
    except (ValueError, OSError, KeyError, AssertionError, subprocess.SubprocessError) as error:
        sys.exit('electrothermal acceptance refused: '+str(error))
