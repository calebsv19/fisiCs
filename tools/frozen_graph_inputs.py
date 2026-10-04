#!/usr/bin/env python3
"""Freeze and replay the complete v1 offline graph data-input closure."""
import argparse
import copy
import hashlib
import os
from pathlib import Path
import subprocess
import sys

import attachment_handoff as api

SCHEMA = 'fisiCs.frozen_graph_inputs'
ROOTS = ('semantic', 'physical', 'hardware', 'attachments')
MAX_FILES = 1024
MAX_TOTAL = 256 * 1024 * 1024
BUILD_EVIDENCE = dict(input_closure='offline_graph_inputs', repeated_graph_bytes='identical',
                      translation_unit_regeneration='not_performed', authenticity='unsigned',
                      execution='not_performed', physical_acceptance='not_assessed')
FIELDS = {
    'semantic': ('schema', 'schema_version', 'project_id', 'translation_units', 'allowed_external_targets'),
    'physical': ('schema', 'schema_version', 'project_id', 'semantic_project_graph_path',
                 'units_metadata_paths', 'physical_catalog_paths', 'coverage_policy', 'source_evidence_policy'),
    'hardware': ('schema', 'schema_version', 'project_id', 'physical_project_graph_path',
                 'hardware_catalog_paths', 'hardware_topology_path', 'physical_evidence_policy', 'required_port_policy'),
}
LEAVES = {
    'physical_catalog': ('physical_contract_catalog', ('schema', 'schema_version', 'catalog_id', 'quantity_kinds', 'contracts')),
    'hardware_catalog': ('hardware_component_catalog', ('schema', 'schema_version', 'catalog_id', 'authority',
                                                       'bus_profiles', 'board_profiles', 'device_models', 'driver_contracts')),
    'topology': ('hardware_topology', ('schema', 'schema_version', 'project_id', 'nodes', 'buses',
                                     'devices', 'connections', 'signal_bindings')),
}


def binary_sha(path):
    digest = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest()


def snapshot_read(path):
    path = Path(path)
    api.need(not any(p.is_symlink() for p in [path, *path.parents]), 'snapshot symlink')
    return api.read(path)


def identity(value):
    api.need(isinstance(value, str) and value.startswith('/') and
             str(Path(value)) == value and '..' not in Path(value).parts,
             'expected canonical absolute input identity')
    return value


def resolve(owner, value):
    api.need(isinstance(value, str) and value and '\\' not in value,
             'invalid manifest path')
    # Lexical normalization only: detached verification never follows original paths.
    return identity(os.path.normpath(str(Path(owner).parent / value)))


def discover(roots, fetch):
    """Enumerate every v1 offline read, including source-receipt checks.

    Upstream graphs are regenerated, not supplied as opaque inputs. Catalogs and
    topology are terminal JSON documents: their v1 readers follow no file paths.
    """
    api.keys(roots, ROOTS)
    manifests = {}
    for kind in api.KINDS:
        raw = fetch(identity(roots[kind]), kind + '_manifest')
        obj = api.decode(raw)
        api.keys(obj, FIELDS[kind])
        api.need(obj['schema'] == 'fisiCs.' + kind + '_project_manifest' and
                 type(obj['schema_version']) is int and obj['schema_version'] == 1,
                 'unsupported frozen manifest')
        manifests[kind] = obj
    project = manifests['semantic']['project_id']
    api.name(project)
    api.need(all(m['project_id'] == project for m in manifests.values()), 'cross-project closure')
    units = manifests['semantic']['translation_units']
    api.need(isinstance(units, list) and 0 < len(units) <= MAX_FILES, 'invalid translation units')

    def metadata(owner, value, kind):
        path = resolve(owner, value)
        obj = api.decode(fetch(path, kind + '_metadata'))
        api.need(obj['schema'] == 'fisiCs.' + kind + '_metadata' and
                 type(obj['schema_version']) is int and obj['schema_version'] == 1 and
                 obj['complete'] is True, 'unsupported metadata input')
        source = obj['source']
        api.keys(source, ('path', 'length_bytes', 'hash_fnv1a64'))
        raw = fetch(identity(source['path']), 'source')
        api.need(type(source['length_bytes']) is int and source['length_bytes'] == len(raw) and
                 source['hash_fnv1a64'] == api.graph_fnv(raw), 'stale source receipt')

    for unit in units:
        api.keys(unit, ('metadata_path',))
        metadata(roots['semantic'], unit['metadata_path'], 'semantic')

    def leaf(owner, value, role):
        obj = api.decode(fetch(resolve(owner, value), role))
        schema, fields = LEAVES[role]
        api.keys(obj, fields)
        api.need(obj['schema'] == 'fisiCs.' + schema and type(obj['schema_version']) is int and
                 obj['schema_version'] == 1, 'unsupported frozen leaf schema')
        if role == 'topology':
            api.need(obj['project_id'] == project, 'cross-project topology')
    for field, role in (('units_metadata_paths', 'units'), ('physical_catalog_paths', 'physical_catalog')):
        values = manifests['physical'][field]
        api.need(isinstance(values, list) and 0 < len(values) <= MAX_FILES, 'invalid physical input list')
        for value in values:
            if role == 'units':
                metadata(roots['physical'], value, role)
            else:
                leaf(roots['physical'], value, role)
    values = manifests['hardware']['hardware_catalog_paths']
    api.need(isinstance(values, list) and 0 < len(values) <= MAX_FILES, 'invalid hardware input list')
    for value in values:
        leaf(roots['hardware'], value, 'hardware_catalog')
    leaf(roots['hardware'], manifests['hardware']['hardware_topology_path'], 'topology')
    # Attachments are an authored array, consumed only by the handoff verifier.
    fetch(identity(roots['attachments']), 'attachments')
    return project, manifests


def local_path(original, role):
    return ('sources' + original) if role == 'source' else ('data/' + api.sha(original.encode()) + '.json')


def freeze(compiler, roots, output):
    compiler = Path(compiler).resolve(strict=True)
    compiler_digest = binary_sha(compiler)
    roots = {k: str(Path(v).absolute()) for k, v in roots.items()}
    captured = {}
    total = 0

    def fetch(original, role):
        nonlocal total
        if original in captured:
            api.need(captured[original][0] == role, 'input aliases different roles')
            return captured[original][1]
        path = Path(original)
        api.need(str(path.resolve(strict=True)) == original, 'symlink or noncanonical input')
        raw = api.read(path)
        total += len(raw)
        api.need(len(captured) < MAX_FILES and total <= MAX_TOTAL, 'frozen input budget exceeded')
        captured[original] = (role, raw)
        return raw

    project, _ = discover(roots, fetch)
    for original, (_, raw) in captured.items():
        api.need(api.read(Path(original)) == raw, 'input changed during capture')
    api.need(binary_sha(compiler) == compiler_digest, 'compiler changed during capture')
    output = Path(output)
    output.mkdir(parents=True, exist_ok=False)
    entries = []
    for original, (role, raw) in sorted(captured.items()):
        relative = local_path(original, role)
        target = output / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(raw)
        entries.append(dict(identity=original, role=role, path=relative, sha256=api.sha(raw), length_bytes=len(raw)))
    receipt = dict(schema=SCHEMA, schema_version=1, project_id=project, compiler_sha256=compiler_digest,
                   roots=roots, inputs=entries)
    (output / 'closure.json').write_bytes(api.encoded(receipt))
    return receipt


def verify(snapshot, expected_sha256):
    snapshot = Path(snapshot)
    raw = snapshot_read(snapshot / 'closure.json')
    api.need(api.sha(raw) == expected_sha256, 'closure digest mismatch')
    receipt = api.decode(raw)
    api.need(raw == api.encoded(receipt), 'closure must use canonical encoding')
    api.keys(receipt, ('schema', 'schema_version', 'project_id', 'compiler_sha256', 'roots', 'inputs'))
    api.need(receipt['schema'] == SCHEMA and type(receipt['schema_version']) is int and
             receipt['schema_version'] == 1, 'unsupported frozen closure')
    api.need(isinstance(receipt['compiler_sha256'], str) and
             len(receipt['compiler_sha256']) == 64 and
             all(c in '0123456789abcdef' for c in receipt['compiler_sha256']), 'invalid compiler digest')
    api.need(isinstance(receipt['inputs'], list) and 0 < len(receipt['inputs']) <= MAX_FILES,
             'invalid frozen inventory')
    captured = {}
    total = 0
    for entry in receipt['inputs']:
        api.keys(entry, ('identity', 'role', 'path', 'sha256', 'length_bytes'))
        original = identity(entry['identity'])
        api.need(original not in captured and entry['path'] == local_path(original, entry['role']),
                 'duplicate identity or noncanonical snapshot path')
        # artifact() expects JSON; sources are arbitrary bytes, so use its path policy directly.
        target = snapshot / entry['path']
        data = snapshot_read(target)
        total += len(data)
        api.need(total <= MAX_TOTAL and type(entry['length_bytes']) is int and
                 entry['length_bytes'] == len(data) and api.sha(data) == entry['sha256'],
                 'frozen input digest/length mismatch')
        captured[original] = (entry['role'], data, entry['path'])
    used = set()

    def fetch(original, role):
        api.need(original in captured and captured[original][0] == role, 'missing closure dependency')
        used.add(original)
        return captured[original][1]

    project, manifests = discover(receipt['roots'], fetch)
    api.need(used == set(captured) and project == receipt['project_id'], 'extra or cross-project closure inputs')
    return receipt, captured, manifests


def build(compiler, snapshot, expected_sha256, output):
    receipt, captured, manifests = verify(snapshot, expected_sha256)
    compiler = Path(compiler).resolve(strict=True)
    api.need(binary_sha(compiler) == receipt['compiler_sha256'], 'frozen compiler digest mismatch')
    capability = subprocess.run([str(compiler), '--help'], env={'PATH': os.defpath, 'FISICS_MAX_PROCS': '0'},
                                capture_output=True, text=True)
    api.need(capability.returncode == 0 and 'FISICS_GRAPH_SOURCE_SNAPSHOT' in capability.stdout,
             'compiler lacks offline source snapshot support')
    output = Path(output).absolute()
    output.mkdir(parents=True, exist_ok=False)
    output = output.resolve()
    for _, data, relative in captured.values():
        target = output / 'snapshot' / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
    (output / 'snapshot/closure.json').write_bytes(api.encoded(receipt))
    api.need(api.sha(api.encoded(receipt)) == expected_sha256, 'closure must use canonical encoding')
    env = {'PATH': os.defpath, 'FISICS_MAX_PROCS': '0',
           'FISICS_GRAPH_SOURCE_SNAPSHOT': str(output / 'snapshot/sources')}

    def staged(owner, value):
        return str(output / 'snapshot' / captured[resolve(owner, value)][2])

    for kind in api.KINDS:
        effective = copy.deepcopy(manifests[kind])
        owner = receipt['roots'][kind]
        if kind == 'semantic':
            for unit in effective['translation_units']:
                unit['metadata_path'] = staged(owner, unit['metadata_path'])
        elif kind == 'physical':
            effective['semantic_project_graph_path'] = str(output / 'semantic.graph.json')
            for field in ('units_metadata_paths', 'physical_catalog_paths'):
                effective[field] = [staged(owner, value) for value in effective[field]]
        else:
            effective['physical_project_graph_path'] = str(output / 'physical.graph.json')
            effective['hardware_catalog_paths'] = [staged(owner, value) for value in effective['hardware_catalog_paths']]
            effective['hardware_topology_path'] = staged(owner, effective['hardware_topology_path'])
        manifest_path = output / (kind + '.manifest.json')
        manifest_path.write_bytes(api.encoded(effective))
        command = [str(compiler), '--' + kind + '-project-manifest', str(manifest_path),
                   '--emit-' + kind + '-project-graph-json', str(output / (kind + '.graph.json'))]
        first = None
        for _ in range(2):
            verify(output / 'snapshot', expected_sha256)
            result = subprocess.run(command, cwd=output, env=env, capture_output=True, text=True)
            api.need(result.returncode == 0, 'frozen ' + kind + ' graph rejected: ' + result.stderr)
            data = api.read(output / (kind + '.graph.json'))
            api.need(first is None or data == first, 'nondeterministic frozen graph')
            first = data
        api.need(manifest_path.read_bytes() == api.encoded(effective), 'effective manifest changed')
    verify(output / 'snapshot', expected_sha256)
    api.need(binary_sha(compiler) == receipt['compiler_sha256'], 'compiler changed during replay')
    # Reuse strict JSON duplicate/nonfinite parsing for the authored attachment array.
    attachments_raw = captured[receipt['roots']['attachments']][1]
    attachments = api.decode(b'{"attachments":' + attachments_raw + b'}')['attachments']
    handoff = dict(schema='fisiCs.attachment_handoff', schema_version=1, project_id=receipt['project_id'],
                   artifacts={kind: dict(path=kind + '.graph.json', sha256=api.sha(api.read(output / (kind + '.graph.json'))))
                              for kind in api.KINDS}, attachments=attachments)
    (output / 'handoff.json').write_bytes(api.encoded(handoff))
    verified = api.verify_handoff(output / 'handoff.json')
    result = dict(schema='fisiCs.frozen_graph_build', schema_version=1, status='verified',
                  closure_sha256=expected_sha256, compiler_sha256=receipt['compiler_sha256'],
                  graphs=verified['artifacts'], handoff_sha256=api.sha(api.encoded(handoff)),
                  effective_manifests={kind: api.sha(api.read(output / (kind + '.manifest.json')))
                                       for kind in api.KINDS},
                  evidence=dict(BUILD_EVIDENCE))
    # Completion marker last. Failures never emit a successful build receipt.
    (output / 'build-receipt.json').write_bytes(api.encoded(result))
    return result


def verify_build(output, expected_sha256):
    """Detached readback; no compiler or original source access, no execution."""
    output = Path(output)
    raw = snapshot_read(output / 'build-receipt.json')
    api.need(api.sha(raw) == expected_sha256, 'build receipt digest mismatch')
    result = api.decode(raw)
    api.keys(result, ('schema', 'schema_version', 'status', 'closure_sha256', 'compiler_sha256',
                      'graphs', 'handoff_sha256', 'effective_manifests', 'evidence'))
    api.need(result['schema'] == 'fisiCs.frozen_graph_build' and
             type(result['schema_version']) is int and result['schema_version'] == 1 and
             result['status'] == 'verified', 'unsupported frozen build')
    api.need(api.encoded(result['evidence']) == api.encoded(BUILD_EVIDENCE),
             'unsupported frozen build evidence')
    receipt, _, _ = verify(output / 'snapshot', result['closure_sha256'])
    api.need(receipt['compiler_sha256'] == result['compiler_sha256'], 'compiler association mismatch')
    api.keys(result['effective_manifests'], api.KINDS)
    for kind, digest in result['effective_manifests'].items():
        api.need(api.sha(snapshot_read(output / (kind + '.manifest.json'))) == digest, 'effective manifest digest mismatch')
    api.need(api.sha(snapshot_read(output / 'handoff.json')) == result['handoff_sha256'], 'handoff digest mismatch')
    handoff = api.verify_handoff(output / 'handoff.json')
    api.need(handoff['artifacts'] == result['graphs'] and handoff['project_id'] == receipt['project_id'],
             'frozen graph association mismatch')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='operation', required=True)
    capture = sub.add_parser('freeze')
    capture.add_argument('--compiler', required=True)
    for key in ROOTS:
        capture.add_argument('--' + key, required=True)
    capture.add_argument('--out', required=True)
    check = sub.add_parser('verify')
    replay = sub.add_parser('build')
    readback = sub.add_parser('verify-build')
    readback.add_argument('--out', required=True)
    readback.add_argument('--expected-sha256', required=True)
    for p in (check, replay):
        p.add_argument('--snapshot', required=True)
        p.add_argument('--expected-sha256', required=True)
    replay.add_argument('--compiler', required=True)
    replay.add_argument('--out', required=True)
    args = parser.parse_args()
    try:
        if args.operation == 'freeze':
            result = freeze(args.compiler, {key: getattr(args, key) for key in ROOTS}, args.out)
        elif args.operation == 'verify':
            result = verify(args.snapshot, args.expected_sha256)[0]
        elif args.operation == 'build':
            result = build(args.compiler, args.snapshot, args.expected_sha256, args.out)
        else:
            result = verify_build(args.out, args.expected_sha256)
    except (OSError, ValueError, KeyError, TypeError, RecursionError) as exc:
        print('Frozen graph input rejected: ' + str(exc), file=sys.stderr)
        return 2
    sys.stdout.buffer.write(api.encoded(result))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
