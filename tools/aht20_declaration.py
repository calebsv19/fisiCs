"""AHT20 witness policy over the generic declaration handoff; no execution imports."""
from pathlib import Path
import tempfile
import attachment_handoff as handoff

SUPPORTED_PORTS = [
    dict(port_id='port.temperature', subject_id='signal.bench.environment.air_temperature',
         quantity_kind='quantity.environment.air_temperature.absolute', unit='kelvin', source_field='temperature'),
    dict(port_id='port.humidity', subject_id='signal.bench.environment.relative_humidity',
         quantity_kind='quantity.environment.relative_humidity', unit=None, source_field='humidity'),
]


def validate(binding, contract, graph_bytes, profile_bytes):
    """Preserve witness policy while delegating all generic declaration joins."""
    handoff.keys(binding, ('schema', 'schema_version', 'adapter', 'device_id', 'profile_id',
                           'profile_sha256', 'scenario', 'sensor', 'instance', 'ports'))
    expected = dict(schema='fisiCs.esp_aht20_executable_binding', schema_version=1,
                    adapter='aht20_rear_left_v1', device_id='device.can.aht20',
                    profile_id='environment.aht20.v1', scenario='environment', sensor='rear_left', instance=12)
    for key, value in expected.items():
        handoff.need(type(binding[key]) is type(value) and binding[key] == value, 'unsupported binding ' + key)
    handoff.need(handoff.encoded(binding['ports']) == handoff.encoded(SUPPORTED_PORTS),
                 'unsupported or duplicate port assignment')
    handoff.need(binding['profile_sha256'] == handoff.sha(profile_bytes), 'profile digest mismatch')
    profile = handoff.decode(profile_bytes)
    handoff.need(profile['id'] == binding['profile_id'] and profile['state'] == 'admitted'
                 and profile['adapter'] == 'family_v1', 'profile is not admitted by this adapter')
    handoff.need(set(graph_bytes) == set(handoff.KINDS), 'missing graph')
    ports = []
    for port in SUPPORTED_PORTS:
        temperature = port['source_field'] == 'temperature'
        ports.append(dict(port_id=port['port_id'], subject_id=port['subject_id'], physical_contract=dict(
            quantity_kind=port['quantity_kind'],
            unit_policy=dict(mode='exact_unit' if temperature else 'dimension_only', unit=port['unit']),
            value_form=dict(kind='scalar', component_count=None, tensor_rank=None, tensor_shape=[]),
            reference_semantics=dict(kind='absolute' if temperature else 'linear', reference_subject=None,
                                     period_binary64=None), frame_id=None)))
    manifest = dict(schema='fisiCs.attachment_handoff', schema_version=1, project_id=contract['project_id'],
                    artifacts={k: dict(path=k + '.graph.json', sha256=contract['static_graphs'][k])
                               for k in handoff.KINDS},
                    attachments=[dict(attachment_id='attachment.aht20', device_id=binding['device_id'],
                                      adapter=dict(id=binding['adapter'], revision='1'), ports=ports)])
    with tempfile.TemporaryDirectory(prefix='fisics-aht20-declaration-') as temp:
        root = Path(temp)
        for kind, raw in graph_bytes.items():
            (root / f'{kind}.graph.json').write_bytes(raw)
        path = root / 'handoff.json'
        path.write_bytes(handoff.encoded(manifest))
        receipt = handoff.verify_handoff(path)
    handoff.need(receipt['attachments'][0]['model'] ==
                 dict(model_id='device.sensor.aht20', revision='completed-response-v1'), 'unsupported device model')
    return binding
