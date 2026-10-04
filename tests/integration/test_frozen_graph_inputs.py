#!/usr/bin/env python3
"""Real compiler detached replay and fail-closed frozen graph input tests."""
import copy
import os
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
COMPILER = Path(os.environ.get('FISICS_TEST_COMPILER', ROOT / 'fisics')).resolve()
EXAMPLES_BUILD = Path(os.environ.get('FISICS_EXAMPLES_BUILD_DIR', ROOT / 'build/examples')).resolve()
sys.path.insert(0, str(ROOT / 'tools'))
import attachment_handoff as api
import frozen_graph_inputs as frozen
BUILD = EXAMPLES_BUILD / 'semantic_metadata/hardware_project'
SOURCE = ROOT / 'examples/semantic_metadata/hardware_project'


def reject(call, message=''):
    try:
        call()
    except (api.HandoffError, OSError, ValueError, KeyError, TypeError) as exc:
        assert message in str(exc), str(exc)
    else:
        raise AssertionError('expected rejection: ' + message)


with tempfile.TemporaryDirectory(prefix='fisics-frozen-') as temp:
    root = Path(temp).resolve()
    original = root / 'original'
    original.mkdir()
    for stem in ('controller', 'signals'):
        shutil.copyfile(SOURCE / (stem + '.c'), original / (stem + '.c'))
        command = [str(COMPILER), '--overlay=physics-units,semantic-metadata',
                   '-I' + str(ROOT / 'include'), '-c', str(original / (stem + '.c')),
                   '-o', str(original / (stem + '.o')),
                   '--emit-semantic-metadata-json', str(original / (stem + '.semantic.json')),
                   '--emit-units-metadata-json', str(original / (stem + '.units.json'))]
        result = subprocess.run(command, capture_output=True, text=True)
        assert result.returncode == 0, result.stderr
    for name in ('physical.catalog.json', 'project.hardware.json', 'vendor.hardware.json',
                 'hardware.topology.json', 'attachments.json'):
        shutil.copyfile(SOURCE / name, original / name)
    roots = {}
    for kind in api.KINDS:
        obj = api.decode((BUILD / (kind + '.manifest.json')).read_bytes())
        if kind == 'physical': obj['physical_catalog_paths'] = ['physical.catalog.json']
        if kind == 'hardware':
            obj['hardware_catalog_paths'] = ['project.hardware.json', 'vendor.hardware.json']
            obj['hardware_topology_path'] = 'hardware.topology.json'
        path = original / (kind + '.manifest.json')
        path.write_bytes(api.encoded(obj))
        roots[kind] = str(path)
    roots['attachments'] = str(original / 'attachments.json')
    snapshot = root / 'snapshot'
    closure = frozen.freeze(COMPILER, roots, snapshot)
    digest = api.sha(api.encoded(closure))
    again = frozen.freeze(COMPILER, roots, root / 'snapshot-again')
    assert api.encoded(closure) == api.encoded(again)
    assert len(closure['inputs']) == 14, len(closure['inputs'])
    frozen.verify(snapshot, digest)
    noncanonical = api.encoded(closure) + b' '
    (snapshot / 'closure.json').write_bytes(noncanonical)
    reject(lambda: frozen.verify(snapshot, api.sha(noncanonical)), 'canonical encoding')
    reject(lambda: frozen.build(COMPILER, snapshot, api.sha(noncanonical), root / 'noncanonical'),
           'canonical encoding')
    assert not (root / 'noncanonical').exists()
    cli = subprocess.run([sys.executable, '-B', str(ROOT / 'tools/frozen_graph_inputs.py'),
                          'verify', '--snapshot', str(snapshot), '--expected-sha256', api.sha(noncanonical)],
                         capture_output=True, text=True)
    assert cli.returncode == 2 and not cli.stdout and 'canonical encoding' in cli.stderr
    (snapshot / 'closure.json').write_bytes(api.encoded(closure))
    reject(lambda: frozen.freeze(COMPILER, roots, snapshot), 'File exists')
    reject(lambda: frozen.verify(snapshot, '0' * 64), 'closure digest')
    assert frozen.MAX_FILES == 1024 and frozen.MAX_TOTAL == 256 * 1024 * 1024
    aliased_roots = dict(roots, attachments=roots['semantic'])
    reject(lambda: frozen.freeze(COMPILER, aliased_roots, root / 'role-alias'),
           'input aliases different roles')
    assert not (root / 'role-alias').exists()
    with patch.object(frozen, 'MAX_FILES', 1):
        reject(lambda: frozen.freeze(COMPILER, roots, root / 'file-budget'),
               'frozen input budget exceeded')
    assert not (root / 'file-budget').exists()
    with patch.object(frozen, 'MAX_TOTAL', 1):
        reject(lambda: frozen.freeze(COMPILER, roots, root / 'byte-budget'),
               'frozen input budget exceeded')
        reject(lambda: frozen.verify(snapshot, digest), 'digest/length')
    assert not (root / 'byte-budget').exists()
    for entry in closure['inputs']:
        path = snapshot / entry['path']
        raw = path.read_bytes()
        path.write_bytes(raw + b' ')
        reject(lambda: frozen.verify(snapshot, digest), 'digest/length')
        path.write_bytes(raw)
        if raw:
            path.write_bytes(bytes([raw[0] ^ 1]) + raw[1:])
            reject(lambda: frozen.verify(snapshot, digest), 'digest/length')
            path.write_bytes(raw)
    entry = closure['inputs'][0]
    path = snapshot / entry['path']
    raw = path.read_bytes()
    path.unlink()
    reject(lambda: frozen.verify(snapshot, digest), 'regular file')
    path.symlink_to(original / 'attachments.json')
    reject(lambda: frozen.verify(snapshot, digest), 'symlink')
    path.unlink()
    path.write_bytes(raw)
    for change in ('missing', 'duplicate', 'escape', 'version'):
        altered = copy.deepcopy(closure)
        if change == 'missing': altered['inputs'].pop()
        elif change == 'duplicate': altered['inputs'].append(altered['inputs'][0])
        elif change == 'escape': altered['inputs'][0]['path'] = '../outside'
        else: altered['schema_version'] = True
        (snapshot / 'closure.json').write_bytes(api.encoded(altered))
        reject(lambda: frozen.verify(snapshot, api.sha(api.encoded(altered))))
    (snapshot / 'closure.json').write_bytes(api.encoded(closure))
    source = original / 'controller.c'
    raw = source.read_bytes()
    source.write_bytes(raw + b' ')
    reject(lambda: frozen.freeze(COMPILER, roots, root / 'stale'), 'stale source')
    source.write_bytes(raw)
    real_read = api.read
    count = [0]
    def changed_read(path):
        data = real_read(path)
        if path == source:
            count[0] += 1
            if count[0] > 1: return data + b' '
        return data
    with patch.object(api, 'read', changed_read):
        reject(lambda: frozen.freeze(COMPILER, roots, root / 'capture-drift'), 'input changed')
    original.rename(root / 'original-offline')
    result = frozen.build(COMPILER, snapshot, digest, root / 'result')
    assert result['status'] == 'verified'
    result_digest = api.sha(api.encoded(result))
    frozen.verify_build(root / 'result', result_digest)
    for field in frozen.BUILD_EVIDENCE:
        altered = copy.deepcopy(result)
        altered['evidence'][field] = 'claimed_stronger_evidence'
        (root / 'result/build-receipt.json').write_bytes(api.encoded(altered))
        reject(lambda: frozen.verify_build(root / 'result', api.sha(api.encoded(altered))),
               'unsupported frozen build evidence')
    altered = copy.deepcopy(result)
    altered['evidence']['unknown_claim'] = True
    (root / 'result/build-receipt.json').write_bytes(api.encoded(altered))
    reject(lambda: frozen.verify_build(root / 'result', api.sha(api.encoded(altered))),
           'unsupported frozen build evidence')
    (root / 'result/build-receipt.json').write_bytes(api.encoded(result))
    shutil.copytree(root / 'result', root / 'relocated')
    frozen.verify_build(root / 'relocated', result_digest)
    shutil.copytree(snapshot, root / 'relocated-snapshot')
    with patch.dict(os.environ, {'FISICS_GRAPH_SOURCE_SNAPSHOT': str(root / 'absent')}):
        replayed = frozen.build(COMPILER, root / 'relocated-snapshot', digest, root / 'replayed')
    assert replayed['closure_sha256'] == result['closure_sha256']
    assert replayed['graphs'] != result['graphs']  # Embedded output paths are not normalized away.
    reject(lambda: frozen.build(COMPILER, snapshot, digest, root / 'result'), 'File exists')
    reject(lambda: frozen.build(ROOT / 'README.md', snapshot, digest, root / 'wrong-compiler'), 'compiler digest')
    real_run = subprocess.run
    def old_compiler_help(*args, **kwargs):
        if args[0] == [str(COMPILER), '--help']:
            return subprocess.CompletedProcess(args[0], 0, stdout='old compiler help', stderr='')
        return real_run(*args, **kwargs)
    with patch.object(subprocess, 'run', old_compiler_help):
        reject(lambda: frozen.build(COMPILER, snapshot, digest, root / 'old-capability'),
               'compiler lacks offline source snapshot support')
    assert not (root / 'old-capability').exists()
    commands = {}
    for kind in api.KINDS:
        commands[kind] = [str(COMPILER), '--' + kind + '-project-manifest',
                          str(root / 'result' / (kind + '.manifest.json')),
                          '--emit-' + kind + '-project-graph-json', str(root / ('negative-' + kind + '.json'))]
        for environment in ({'PATH': os.defpath},
                            {'PATH': os.defpath, 'FISICS_GRAPH_SOURCE_SNAPSHOT': str(root / 'absent')}):
            attempt = subprocess.run(commands[kind], env=environment, capture_output=True)
            assert attempt.returncode != 0, (kind, attempt.stderr)
            assert not (root / ('negative-' + kind + '.json')).exists()
    # An explicit snapshot must never fall back, even while original source bytes are available.
    (root / 'original-offline').rename(original)
    bad_roots = {'missing': str(root / 'absent'), 'empty': '', 'relative': 'snapshot', 'filesystem_root': '/'}
    (root / 'snapshot-link').symlink_to(root / 'result/snapshot/sources', target_is_directory=True)
    bad_roots['symlink'] = str(root / 'snapshot-link')
    for kind, command in commands.items():
        for label, source_root in bad_roots.items():
            attempt = subprocess.run(command, env={'PATH': os.defpath,
                                     'FISICS_GRAPH_SOURCE_SNAPSHOT': source_root}, capture_output=True)
            assert attempt.returncode != 0, (kind, label, attempt.stderr)
            assert b'invalid or missing frozen' in attempt.stderr, (kind, label, attempt.stderr)
            assert not (root / ('negative-' + kind + '.json')).exists()
    # The copied source is authoritative; changing only the disposable live original cannot rescue it.
    captured_source = root / 'result/snapshot/sources' / str(original / 'controller.c').lstrip('/')
    captured_bytes = captured_source.read_bytes()
    captured_source.write_bytes(captured_bytes + b' ')
    for kind, command in commands.items():
        attempt = subprocess.run(command, env={'PATH': os.defpath,
                                 'FISICS_GRAPH_SOURCE_SNAPSHOT': str(root / 'result/snapshot/sources')},
                                 capture_output=True)
        assert attempt.returncode != 0, (kind, attempt.stderr)
        assert not (root / ('negative-' + kind + '.json')).exists()
    captured_source.write_bytes(captured_bytes)
    original.rename(root / 'original-offline')
    for name in ('semantic.graph.json', 'physical.graph.json', 'hardware.graph.json',
                 'physical.manifest.json', 'handoff.json'):
        path = root / 'relocated' / name
        raw = path.read_bytes()
        path.write_bytes(raw + b' ')
        reject(lambda: frozen.verify_build(root / 'relocated', result_digest))
        path.write_bytes(raw)
    real_run = subprocess.run
    fired = [False]
    def mutate_after_run(*args, **kwargs):
        value = real_run(*args, **kwargs)
        if not fired[0] and '--semantic-project-manifest' in args[0]:
            fired[0] = True
            entry = next(e for e in closure['inputs'] if e['role'] == 'source')
            path = root / 'drift-result/snapshot' / entry['path']
            path.write_bytes(path.read_bytes() + b' ')
        return value
    with patch.object(subprocess, 'run', mutate_after_run):
        reject(lambda: frozen.build(COMPILER, snapshot, digest, root / 'drift-result'), 'digest/length')
    assert not (root / 'drift-result/build-receipt.json').exists()
print('FROZEN INPUT PASS: 14-input closure, detached real-compiler replay, repeated graph bytes,')
print('relocated readback, all input-class tampering, missing/duplicate/path/version/source/compiler rejection,')
print('capture/replay drift rejection, source-map fail-closed, and create-only completion boundary')
print('CLOSEOUT HARDENING PASS: early canonical rejection, CLI exit 2, and fixed evidence vocabulary')
print('SOURCE ISOLATION PASS: all three readers reject missing and invalid snapshot roots with live originals;')
print('corrupt captured source rejects without live-source fallback')
print('CLOSURE GUARDS PASS: role alias and lowered file/byte budgets reject before output;')
print('ambient snapshot setting is isolated and missing compiler capability rejects before output')
