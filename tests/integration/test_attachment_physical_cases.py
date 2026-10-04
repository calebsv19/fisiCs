#!/usr/bin/env python3
"""Qualify physical handoffs using compiler-produced variants of public catalogs."""
import copy
import os
import json
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
COMPILER = Path(os.environ.get('FISICS_TEST_COMPILER', ROOT / 'fisics')).resolve()
EXAMPLES_BUILD = Path(os.environ.get('FISICS_EXAMPLES_BUILD_DIR', ROOT / 'build/examples')).resolve()
sys.path.insert(0, str(ROOT / 'tools'))
import attachment_handoff as api

SOURCE = ROOT / 'examples/semantic_metadata/hardware_project'
BUILD = EXAMPLES_BUILD / 'semantic_metadata/hardware_project'


def load(path):
    return json.loads(path.read_text())


def write(root, name, value):
    path = root / name
    path.write_bytes(api.encoded(value))
    return path


def compile_graph(root, kind, manifest):
    path = write(root, kind + '.manifest.json', manifest)
    output = root / (kind + '.graph.json')
    args = [str(COMPILER), '--' + kind + '-project-manifest', str(path),
            '--emit-' + kind + '-project-graph-json', str(output)]
    result = subprocess.run(args, capture_output=True, text=True)
    assert result.returncode == 0, result.stdout + result.stderr
    first = output.read_bytes()
    result = subprocess.run(args, capture_output=True, text=True)
    assert result.returncode == 0 and first == output.read_bytes(), result.stderr
    return first


def qualify(root, variant):
    semantic = load(BUILD / 'semantic.manifest.json')
    semantic['translation_units'] = [dict(metadata_path=str(BUILD / (s + '.semantic.json')))
                                     for s in ('controller', 'signals')]
    semantic['allowed_external_targets'] = ['frame.test.body', 'frame.test.other', 'reference.test.origin']
    graph_bytes = dict(semantic=compile_graph(root, 'semantic', semantic))
    catalog = load(SOURCE / 'physical.catalog.json')
    target = catalog['contracts'][0]
    if variant == 'vector_frame':
        target['value_form'].update(kind='vector', component_count=3)
        target['frame_id'] = 'frame.test.body'
    elif variant == 'tensor_frame':
        target['value_form'].update(kind='tensor', tensor_rank=2, tensor_shape=[2, 3])
        target['frame_id'] = 'frame.test.body'
    elif variant == 'relative':
        target['reference_semantics'].update(kind='relative', reference_subject='reference.test.origin')
    elif variant == 'cyclic':
        target['reference_semantics'].update(kind='cyclic', period_binary64='0x1p+4')
    elif variant == 'convertible':
        target['unit_policy'].update(mode='convertible_unit', unit='kelvin')
    elif variant == 'dimension_only':
        target['unit_policy'].update(mode='dimension_only', unit=None)
    physical = load(BUILD / 'physical.manifest.json')
    physical['semantic_project_graph_path'] = str(root / 'semantic.graph.json')
    physical['units_metadata_paths'] = [str(BUILD / (s + '.units.json')) for s in ('controller', 'signals')]
    physical['physical_catalog_paths'] = [str(write(root, 'catalog.json', catalog))]
    graph_bytes['physical'] = compile_graph(root, 'physical', physical)
    vendor = load(SOURCE / 'vendor.hardware.json')
    model = next(m for m in vendor['device_models'] if m['model_id'] == 'device.environment.sensor')
    model['physical_ports'][0]['requirement'].update(value_form=target['value_form']['kind'],
                                                   reference_kind=target['reference_semantics']['kind'])
    hardware = load(BUILD / 'hardware.manifest.json')
    hardware['physical_project_graph_path'] = str(root / 'physical.graph.json')
    hardware['hardware_catalog_paths'] = [str(SOURCE / 'project.hardware.json'), str(write(root, 'vendor.json', vendor))]
    graph_bytes['hardware'] = compile_graph(root, 'hardware', hardware)
    attachments = load(SOURCE / 'attachments.json')
    expected = {k: copy.deepcopy(target[k]) for k in ('quantity_kind', 'unit_policy', 'value_form', 'reference_semantics', 'frame_id')}
    attachments[0]['ports'][0]['physical_contract'] = expected
    manifest = dict(schema='fisiCs.attachment_handoff', schema_version=1, project_id=semantic['project_id'],
                    artifacts={k: dict(path=k + '.graph.json', sha256=api.sha(raw)) for k, raw in graph_bytes.items()},
                    attachments=attachments)
    path = write(root, 'handoff.json', manifest)
    receipt = api.verify_handoff(path)
    output = next(a for a in receipt['attachments'] if a['attachment_id'] == 'attachment.temperature')['ports'][0]
    assert output['physical_contract'] == expected
    assert 'conversion' in output
    # All five facets must survive exactly and reject disagreement independently.
    for field in expected:
        changed = copy.deepcopy(manifest)
        changed['attachments'][0]['ports'][0]['physical_contract'][field] = 'wrong'
        write(root, 'handoff.json', changed)
        try:
            api.verify_handoff(path)
        except api.HandoffError:
            pass
        else:
            raise AssertionError('accepted mismatched ' + field)
    # Well-typed alternatives must also fail; this is not just type validation.
    changed = copy.deepcopy(manifest)
    mismatch = changed['attachments'][0]['ports'][0]['physical_contract']
    if variant == 'vector_frame':
        mismatch['frame_id'] = 'frame.test.other'
    elif variant == 'tensor_frame':
        mismatch['value_form']['tensor_shape'] = [3, 2]
    elif variant == 'relative':
        mismatch['reference_semantics']['reference_subject'] = 'frame.test.other'
    elif variant == 'cyclic':
        mismatch['reference_semantics']['period_binary64'] = '0x1p+5'
    elif variant == 'convertible':
        mismatch['unit_policy']['mode'] = 'exact_unit'
    else:
        mismatch['unit_policy']['unit'] = 'kelvin'
    write(root, 'handoff.json', changed)
    try:
        api.verify_handoff(path)
    except api.HandoffError:
        pass
    else:
        raise AssertionError('accepted well-typed mismatch for ' + variant)
    write(root, 'handoff.json', manifest)
    assert api.encoded(receipt) == api.encoded(api.verify_handoff(path))


with tempfile.TemporaryDirectory(prefix='fisics-physical-handoff-') as temp:
    for variant in ('vector_frame', 'tensor_frame', 'relative', 'cyclic', 'convertible', 'dimension_only'):
        root = Path(temp) / variant
        root.mkdir()
        qualify(root, variant)
        print('PHYSICAL HANDOFF PASS:', variant)
print('PHYSICAL HANDOFF: 6 compiler-generated cases, 36 facet rejections, byte determinism')
