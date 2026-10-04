#!/usr/bin/env python3
"""Opt-in producer provenance over real compiler output, retaining v1 compatibility."""
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
from physical_graph_provenance import produce

BUILD = EXAMPLES_BUILD / 'semantic_metadata/hardware_project'
with tempfile.TemporaryDirectory(prefix='fisics-parent-proof-') as temp:
    root = Path(temp)
    first = produce(COMPILER, BUILD / 'physical.manifest.json', root / 'first')
    second = produce(COMPILER, BUILD / 'physical.manifest.json', root / 'second')
    assert api.encoded(first) == api.encoded(second)
    bundle = root / 'first'
    hardware = json.loads((BUILD / 'hardware.manifest.json').read_text())
    hardware['physical_project_graph_path'] = str(bundle / 'physical.graph.json')
    manifest_path = bundle / 'hardware.manifest.json'
    manifest_path.write_bytes(api.encoded(hardware))
    result = subprocess.run([str(COMPILER), '--hardware-project-manifest', str(manifest_path),
                             '--emit-hardware-project-graph-json', str(bundle / 'hardware.graph.json')],
                            capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    manifest = dict(schema='fisiCs.attachment_handoff', schema_version=2, project_id=first['project_id'],
                    artifacts={k: dict(path=k + '.graph.json', sha256=api.sha(api.read(bundle / (k + '.graph.json'))))
                               for k in api.KINDS},
                    provenance=dict(path='provenance.json', sha256=api.sha(api.encoded(first))),
                    attachments=json.loads((ROOT / 'examples/semantic_metadata/hardware_project/attachments.json').read_text()))
    handoff = bundle / 'handoff.json'

    def check(value, good):
        handoff.write_bytes(api.encoded(value))
        try:
            receipt = api.verify_handoff(handoff)
        except api.HandoffError:
            assert not good
            return
        assert good
        return receipt

    receipt = check(manifest, True)
    assert receipt['schema_version'] == 2
    assert receipt['evidence']['semantic_parent'] == 'producer_receipt_verified'
    assert receipt['evidence']['authenticity'] == 'unsigned'
    missing = copy.deepcopy(manifest); del missing['provenance']; check(missing, False)
    unsupported = copy.deepcopy(manifest); unsupported['schema_version'] = 3; check(unsupported, False)
    # Updating a bundle pin cannot silently substitute a different semantic parent.
    semantic = bundle / 'semantic.graph.json'
    original = semantic.read_bytes()
    semantic.write_bytes(original + b' ')
    drift = copy.deepcopy(manifest)
    drift['artifacts']['semantic']['sha256'] = api.sha(semantic.read_bytes())
    check(drift, False)
    legacy = copy.deepcopy(drift); legacy['schema_version'] = 1; del legacy['provenance']
    assert 'semantic_parent' not in check(legacy, True)['evidence']
    semantic.write_bytes(original)
    altered = copy.deepcopy(first); altered['physical']['length_bytes'] += 1
    (bundle / 'provenance.json').write_bytes(api.encoded(altered))
    bad = copy.deepcopy(manifest); bad['provenance']['sha256'] = api.sha(api.encoded(altered))
    check(bad, False)
    try:
        produce(COMPILER, BUILD / 'physical.manifest.json', bundle)
    except FileExistsError:
        pass
    else:
        raise AssertionError('producer overwrote retained output')
print('PROVENANCE PASS: deterministic producer, v2 admission, missing/version/parent/output rejection, v1 compatibility, create-only')
