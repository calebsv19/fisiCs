"""Read-only accepted-prefix accounting from real Thermal observations, not a solver."""
from fractions import Fraction
import math
import os
from pathlib import Path
import tempfile
from electrothermal_binding import encoded, load, require, sha

ROOT = Path(__file__).resolve().parents[2]
CONTRACT_PATH = ROOT/'tests/fixtures/physical_semantics/electrothermal/diagnostic_contract.json'
CONTRACT = dict(schema='fisics.electrothermal_diagnostic_contract.v1',
                native_energy_abs_j=1e-10, mapping_abs_j=1e-16, temperature_abs_k=1e-8,
                boundary='left_face_j_minus_right_face_j_positive_into_solid',
                storage='native_material_relative_initial_energy_is_not_imported_heat',
                time='exact_stored_sample_not_global_completed_steps',
                pending='remaining_allocated_horizon_at_each_sample_not_consumed',
                unallocated='full_electrical_horizon_not_deposited',
                temperature_control='constant_capacity_weighted_change_only',
                observations='canonical_hex_trace_live_json_checked_with_existing_tolerances',
                compiler_checked=False)


def contract():
    require(load(CONTRACT_PATH) == CONTRACT, 'diagnostic contract/tolerance mismatch')
    return sha(CONTRACT_PATH)


def close(actual, expected, tolerance, reason):
    require(math.isfinite(actual) and math.isfinite(expected) and abs(actual-expected) <= tolerance, reason)


def inspect_bundle(bundle, binary, heat, through=None):
    """Validate frozen input in a private native session; never step or save it."""
    bundle = Path(bundle); _, prepared, record = heat.bundle(bundle)
    names = ['manifest.json','source.json','receipt.json','scenario.json','complete.json','checkpoint/assembly.json']
    if record['result_sha256'] is not None:
        names.append('result.json')
    original = {name:sha(bundle/name) for name in names}
    available = heat.integer(record['completed_steps'], 0, prepared['scenario']['steps'])
    through = available if through is None else heat.integer(through, 0, available)
    trace_file = 'result.json' if record['result_sha256'] is not None else 'checkpoint/assembly.json'
    trace = load(bundle/trace_file)['payload']['trace']; scenario = prepared['scenario']; receipt = prepared['receipt']
    require(trace['next_step'] == available and len(trace['samples']) == available+1, 'diagnostic trace support')
    csha = contract(); native = heat.Native(binary); validator_engine = native.meta['state']['engine']
    try:
        if record['result_sha256'] is not None:
            heat.bundle(bundle, native)
        else:
            # Validation only in this private process. No resume, original-source
            # read, step, publication or engine/adapter relabelling is performed.
            native.call('checkpoint_open', {'path':str((bundle/'checkpoint').resolve())})
        initial = native.call('inspect', {'sample':0})['state']
        require(initial['engine'] == trace['engine'] and trace['scenario'] == scenario, 'diagnostic native provenance')
        spatial = trace['spatial']; cells = spatial['cells']; count = len(cells)
        require(len(initial['energy_j']) == count, 'diagnostic cell count')
        initial_frozen = [float.fromhex(x) for x in trace['samples'][0]]
        initial['temperature_k'] = initial_frozen[:count]; initial['energy_j'] = initial_frozen[count:2*count]
        layers = {x['id']:x for x in scenario['layers']}; references = []
        for i, cell in enumerate(cells):
            layer = layers[cell['layer_id']]
            capacity = scenario['area_m2']*(cell['end_m']-cell['begin_m'])*layer['cv_min'] if layer['cv_min'] == layer['cv_max'] else None
            references.append(dict(cell_id=cell['cell_id'], layer_id=cell['layer_id'],
                                   material_min_k=cell['min_k'], initial_temperature_k=initial['temperature_k'][i],
                                   initial_stored_energy_j=initial['energy_j'][i], constant_capacity_j_per_k=capacity))
        capacities = [x['constant_capacity_j_per_k'] for x in references]
        total_capacity = math.fsum(capacities) if all(c is not None and c > 0 for c in capacities) else None
        insulated = all(x['kind'] == 0 for x in scenario['ends'])
        rows = []; quantities = receipt['per_step_component_j']; allocations = receipt['allocations']
        require(len(quantities) == scenario['steps'], 'diagnostic full horizon support')
        for k in range(through+1):
            state = native.call('inspect', {'sample':k})['state']
            time = Fraction(k*scenario['step_num'], scenario['step_den'])
            require(state['sample'] == k and state['completed_steps'] == available and
                    Fraction(state['time_num'],state['time_den']) == time, 'diagnostic exact sample time')
            # Compare actual binary64 owner observations with the frozen trace.
            frozen = [float.fromhex(x) for x in trace['samples'][k]]
            live = state['temperature_k']+state['energy_j']+state['face_transfer_j']+[state['source_j'],state['sink_j']]
            require(len(live) == len(frozen) == 3*count+3, 'diagnostic observation shape')
            for index,(actual,exact) in enumerate(zip(live,frozen)):
                close(actual,exact,CONTRACT['temperature_abs_k'] if index < count else CONTRACT['native_energy_abs_j'], 'diagnostic observation mismatch')
            # The hex trace retains binary64; normal live JSON may round by an ULP.
            state['temperature_k'] = frozen[:count]; state['energy_j'] = frozen[count:2*count]
            state['face_transfer_j'] = frozen[2*count:3*count+1]; state['source_j'],state['sink_j'] = frozen[-2:]
            contributions = []
            for i, allocation in enumerate(allocations):
                accepted = math.fsum(q[i] for q in quantities[:k]); pending = math.fsum(q[i] for q in quantities[k:])
                close(accepted+pending, allocation['energy_j'], CONTRACT['mapping_abs_j'], 'component horizon accounting')
                contributions.append(dict(component_id=allocation['component_id'],layer_id=allocation['layer_id'],
                                          accepted_j=accepted,pending_at_sample_j=pending,horizon_j=allocation['energy_j']))
            accepted = math.fsum(x['accepted_j'] for x in contributions)
            pending = math.fsum(x['pending_at_sample_j'] for x in contributions)
            close(accepted+pending, receipt['allocated_energy_j'], CONTRACT['mapping_abs_j'], 'allocated horizon accounting')
            left, right = state['face_transfer_j'][0], -state['face_transfer_j'][-1]
            boundary = math.fsum([left,right]); stored = state['stored_change_j']
            energy_tol = CONTRACT['native_energy_abs_j']
            close(state['source_j'], accepted, energy_tol, 'receipt/native source prefix mismatch')
            require(state['sink_j'] == 0, 'imported power unexpectedly has a sink')
            close(state['boundary_into_j'], boundary, energy_tol, 'boundary sign/readback mismatch')
            close(stored, math.fsum(e-e0 for e,e0 in zip(state['energy_j'],initial['energy_j'])), energy_tol, 'native initial storage mismatch')
            residual = stored-accepted+state['sink_j']-boundary
            close(residual, 0, energy_tol, 'accepted storage/boundary closure')
            close(state['balance_residual_j'], stored-state['source_j']+state['sink_j']-boundary, energy_tol, 'native balance readback mismatch')
            temperature = None
            if total_capacity is not None:
                actual = math.fsum(c*(t-t0) for c,t,t0 in zip(capacities,state['temperature_k'],initial['temperature_k']))/total_capacity
                expected = (accepted-state['sink_j']+boundary)/total_capacity
                close(actual, expected, CONTRACT['temperature_abs_k'], 'constant capacity temperature closure')
                temperature = dict(capacity_j_per_k=total_capacity, weighted_change_k=actual,
                                   net_accepted_j_over_capacity_k=expected,residual_k=actual-expected,
                                   insulated_q_over_c_k=accepted/total_capacity if insulated else None)
            if insulated:
                close(boundary, 0, energy_tol, 'insulated boundary is not zero')
                close(stored, accepted, energy_tol, 'insulated accepted heat/storage mismatch')
            rows.append(dict(step=k,time_num=time.numerator,time_den=time.denominator,
                             accepted_allocated_j=accepted,pending_allocated_at_sample_j=pending,
                             components=contributions,native_source_j=state['source_j'],native_sink_j=state['sink_j'],
                             native_stored_energy_j=state['energy_j'],native_stored_change_j=stored,
                             boundary_left_into_j=left,boundary_right_into_j=right,boundary_net_into_j=boundary,
                             native_balance_residual_j=state['balance_residual_j'],receipt_storage_residual_j=residual,
                             temperature_k=state['temperature_k'],temperature_control=temperature))
        require(all(sha(bundle/name) == digest for name,digest in original.items()), 'diagnostic input changed')
        value = dict(schema='fisics.electrothermal_diagnostics.v1',contract_sha256=csha,
                     evidence='native_validated_observations_and_external_receipt_accounting_not_compiler_proof',
                     record_kind='completed_observations' if record['result_sha256'] is not None else 'checkpoint',
                     available_completed_steps=available,reported_through_step=through,horizon_steps=scenario['steps'],
                     exact_step=dict(numerator=scenario['step_num'],denominator=scenario['step_den']),
                     provenance=dict(source_sha256=receipt['source_sha256'],plan_id=receipt['plan_id'],
                                     circuit_sha256=receipt['circuit_sha256'],scenario_sha256=receipt['scenario_sha256'],
                                     transfer_receipt_sha256=sha(bundle/'receipt.json'),files=original,
                                     native_source_engine=trace['engine'],validator_engine=validator_engine,
                                     validator_sha256=sha(binary)),
                     horizon=dict(allocated_j=receipt['allocated_energy_j'],unallocated_not_deposited_j=receipt['unallocated_dissipation_j'],
                                  electrical_supply_j=receipt['source_supply_energy_j'],electrical_dissipation_j=receipt['total_dissipation_j']),
                     material=dict(reference=spatial['enthalpy_reference'],cells=references,
                                   initial_total_stored_j=math.fsum(initial['energy_j']),insulated=insulated),
                     maxima=dict(receipt_storage_residual_abs_j=max(abs(x['receipt_storage_residual_j']) for x in rows),
                                 native_balance_residual_abs_j=max(abs(x['native_balance_residual_j']) for x in rows)),
                     samples=rows)
        return value
    finally:
        native.close()


def verify_report(value, bundle, binary, heat):
    expected = inspect_bundle(bundle,binary,heat,value['reported_through_step'])
    require(encoded(value) == encoded(expected), 'diagnostic meaning/native observation mismatch')
    return value


def publish(value, destination, bundle):
    destination = Path(destination).absolute()
    require(not destination.resolve().is_relative_to(Path(bundle).resolve()), 'diagnostic output inside immutable input')
    raw = encoded(value); name = None
    try:
        fd, name = tempfile.mkstemp(prefix='.electrothermal-', dir=destination.parent)
        with os.fdopen(fd,'wb') as stream:
            stream.write(raw);stream.flush();os.fsync(stream.fileno())
        # Atomic create-only publication; existing output always survives.
        os.link(name,destination)
    finally:
        if name is not None:
            os.unlink(name)


def summary_markdown(reports):
    lines = ['# Accepted-prefix electrothermal diagnostics', '',
             'All energy columns are joules. Pending is allocated horizon energy after the selected sample; unallocated is the full electrical horizon and is never deposited.', '',
             '| Case | Available / shown steps | Allocated horizon | Accepted at sample | Pending at sample | Unallocated horizon | Stored change | Signed boundary into solid | Closure residual |',
             '| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |']
    for name, report in reports:
        row = report['samples'][-1]; horizon = report['horizon']
        values = [horizon['allocated_j'],row['accepted_allocated_j'],row['pending_allocated_at_sample_j'],horizon['unallocated_not_deposited_j'],row['native_stored_change_j'],row['boundary_net_into_j'],row['receipt_storage_residual_j']]
        lines.append('| '+name+' | '+str(report['available_completed_steps'])+' / '+str(report['reported_through_step'])+' | '+' | '.join(format(x,'.12g') for x in values)+' |')
    lines += ['', 'Initial material-relative storage is reported separately in each JSON; it is not imported heat. Boundary loss is negative boundary energy, not a source/sink relabel. Native source identities and validation-tool identities remain distinct.', '']
    return '\n'.join(lines)
