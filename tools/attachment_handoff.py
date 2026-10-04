#!/usr/bin/env python3
"""Verify a declared device attachment handoff; never import or execute a consumer."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import sys

LIMIT = 16 * 1024 * 1024
KINDS = ('semantic', 'physical', 'hardware')


class HandoffError(ValueError):
    """An unsupported, inconsistent or unsafe handoff."""


def need(condition, message):
    if not condition:
        raise HandoffError(message)


def pairs(items):
    result = {}
    for key, value in items:
        need(key not in result, 'duplicate JSON key: ' + key)
        result[key] = value
    return result


def decode(raw):
    need(len(raw) <= LIMIT, 'JSON exceeds 16 MiB')
    try:
        value = json.loads(raw, object_pairs_hook=pairs,
                           parse_constant=lambda value: (_ for _ in ()).throw(HandoffError('nonfinite JSON')))
    except (UnicodeError, json.JSONDecodeError, RecursionError) as exc:
        raise HandoffError('invalid JSON') from exc
    need(isinstance(value, dict), 'JSON root must be an object')
    return value


def read(path):
    need(path.is_file(), 'input is not a regular file')
    with path.open('rb') as stream:
        raw = stream.read(LIMIT + 1)
    need(len(raw) <= LIMIT, 'artifact exceeds 16 MiB')
    return raw


def encoded(value):
    return (json.dumps(value, sort_keys=True, indent=2, allow_nan=False) + '\n').encode()


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def graph_fnv(raw):
    value = 1469598103934665603  # Existing compiler receipt basis, not a new hash contract.
    for byte in raw:
        value = ((value ^ byte) * 1099511628211) & ((1 << 64) - 1)
    return f'{value:016x}'


def keys(value, expected):
    need(isinstance(value, dict) and set(value) == set(expected), 'unexpected object fields')


def name(value):
    need(isinstance(value, str) and 0 < len(value) <= 256 and not any(ord(c) < 32 for c in value),
         'invalid identity string')
    return value


def index(items, key):
    need(isinstance(items, list), 'expected record array')
    result = {}
    for item in items:
        identity = name(item[key])
        need(identity not in result, 'duplicate identity: ' + identity)
        result[identity] = item
    return result


def artifact(root, descriptor):
    keys(descriptor, ('path', 'sha256'))
    path = descriptor['path']
    need(isinstance(path, str) and '\\' not in path, 'invalid artifact path')
    relative = PurePosixPath(path)
    need(path == str(relative) and not relative.is_absolute() and
         relative.parts and all(p not in ('.', '..') for p in relative.parts), 'unsafe artifact path')
    target = root
    for part in relative.parts:
        target = target / part
        need(not target.is_symlink(), 'symlink artifact path')
    need(target.resolve().is_relative_to(root.resolve()), 'artifact escapes bundle')
    need(isinstance(descriptor['sha256'], str) and re.fullmatch('[0-9a-f]{64}', descriptor['sha256']),
         'invalid SHA-256')
    raw = read(target)
    need(sha(raw) == descriptor['sha256'], 'artifact digest mismatch: ' + path)
    return raw, decode(raw)


def _verify_handoff(manifest_path):
    """Return a deterministic v1/v2 receipt for integrity and selected reference joins.

    This is not a replacement for compiler graph generation/full validation.
    Embedded source paths are provenance only and are never opened.
    """
    manifest_path = Path(manifest_path)
    raw = read(manifest_path)
    manifest = decode(raw)
    version = manifest.get('schema_version')
    need(type(version) is int and version in (1, 2), 'unsupported handoff schema/version')
    fields = ('schema', 'schema_version', 'project_id', 'artifacts', 'attachments')
    keys(manifest, fields + (('provenance',) if version == 2 else ()))
    need(manifest['schema'] == 'fisiCs.attachment_handoff' and
         type(manifest['schema_version']) is int and manifest['schema_version'] in (1, 2),
         'unsupported handoff schema/version')
    project = name(manifest['project_id'])
    keys(manifest['artifacts'], KINDS)
    graphs = {}
    graph_bytes = {}
    for kind in KINDS:
        data, graph = artifact(manifest_path.parent, manifest['artifacts'][kind])
        need(graph['schema'] == f'fisiCs.{kind}_project_graph' and
             type(graph['schema_version']) is int and graph['schema_version'] == 1 and
             graph['complete'] is True and graph['project']['id'] == project,
             'unsupported, incomplete or cross-project graph: ' + kind)
        graphs[kind] = graph
        graph_bytes[kind] = data
    if version == 2:
        _, provenance = artifact(manifest_path.parent, manifest['provenance'])
        keys(provenance, ('schema', 'schema_version', 'producer', 'project_id', 'manifest_sha256', 'semantic', 'physical'))
        need(provenance['schema'] == 'fisiCs.physical_graph_provenance' and
             type(provenance['schema_version']) is int and provenance['schema_version'] == 1 and
             provenance['project_id'] == project, 'unsupported producer provenance')
        keys(provenance['producer'], ('compiler_sha256', 'operation'))
        need(provenance['producer']['operation'] == 'physical_project_graph', 'unsupported producer operation')
        for digest in (provenance['manifest_sha256'], provenance['producer']['compiler_sha256']):
            need(isinstance(digest, str) and re.fullmatch('[0-9a-f]{64}', digest), 'invalid producer digest')
        for kind in ('semantic', 'physical'):
            descriptor = provenance[kind]
            keys(descriptor, ('sha256', 'length_bytes'))
            need(type(descriptor['length_bytes']) is int and descriptor['length_bytes'] == len(graph_bytes[kind])
                 and descriptor['sha256'] == sha(graph_bytes[kind]), 'producer parent/output digest mismatch')
    need(graphs['physical']['semantic_graph_receipt']['project_id'] == project,
         'semantic project receipt mismatch')
    parent = graphs['hardware']['physical_graph_receipt']
    need(parent['project_id'] == project and
         type(parent['length_bytes']) is int and parent['length_bytes'] == len(graph_bytes['physical']) and
         parent['hash_fnv1a64'] == graph_fnv(graph_bytes['physical']), 'physical graph receipt mismatch')
    records = index(graphs['semantic']['records'], 'source_identity')
    physical = index(graphs['physical']['physical_contracts'], 'contract_id')
    quantities = index(graphs['physical']['quantity_kinds'], 'id')
    hardware = graphs['hardware']
    devices = index(hardware['devices'], 'device_id')
    models = {(name(m['model_id']), name(m['revision'])): m for m in hardware['device_models']}
    need(len(models) == len(hardware['device_models']), 'duplicate device model revision')
    drivers = {(name(d['driver_id']), name(d['revision'])) for d in hardware['driver_contracts']}
    need(len(drivers) == len(hardware['driver_contracts']), 'duplicate driver revision')
    joins = index(hardware['signal_bindings'], 'binding_id')
    attachments = index(manifest['attachments'], 'attachment_id')
    need(0 < len(attachments) <= 256, 'expected 1..256 attachments')
    used_devices = set()
    output = []
    for identity, attachment in sorted(attachments.items()):
        keys(attachment, ('attachment_id', 'device_id', 'adapter', 'ports'))
        device_id = name(attachment['device_id'])
        need(device_id in devices and device_id not in used_devices, 'unknown or duplicate attached device')
        used_devices.add(device_id)
        adapter = attachment['adapter']
        if adapter is not None:
            keys(adapter, ('id', 'revision'))
            name(adapter['id']); name(adapter['revision'])
        device = devices[device_id]
        if device['driver'] is not None:
            need((device['driver']['driver_id'], device['driver']['revision']) in drivers,
                 'device driver revision missing')
        model_key = (device['model']['model_id'], device['model']['revision'])
        need(model_key in models, 'device model revision missing')
        model_ports = index(models[model_key]['physical_ports'], 'port_id')
        ports = index(attachment['ports'], 'port_id')
        required = {p for p, value in model_ports.items() if value['required'] is True}
        need(required <= set(ports), 'required physical port missing')
        projected = []
        for port_id, port in sorted(ports.items()):
            keys(port, ('port_id', 'subject_id', 'physical_contract'))
            need(port_id in model_ports, 'unknown physical port')
            matches = [j for j in joins.values() if j['device_id'] == device_id and
                       j['physical_port_id'] == port_id]
            need(len(matches) == 1 and matches[0]['result'] == 'compatible', 'ambiguous or incompatible signal join')
            contract = physical[matches[0]['physical_contract_id']]
            subject = name(port['subject_id'])
            need(contract['subject'] == {'kind': 'semantic_id', 'id': subject} and subject in records,
                 'unresolved source subject')
            need(contract['state'] == 'source_compatible', 'physical source compatibility missing')
            need(type(contract['record_index']) is int and
                 contract['record_index'] == records[subject]['record_index'], 'source record join mismatch')
            fields = ('quantity_kind', 'unit_policy', 'value_form', 'reference_semantics', 'frame_id')
            expected = {key: contract[key] for key in fields}
            need(contract['quantity_kind'] in quantities, 'quantity definition missing')
            # Compare JSON types as well as values (true must not equal 1).
            need(encoded(port['physical_contract']) == encoded(expected), 'physical contract mismatch')
            projected.append(dict(port_id=port_id, subject_id=subject,
                                  direction=model_ports[port_id]['direction'],
                                  physical_contract_id=contract['contract_id'], physical_contract=expected,
                                  conversion=contract['conversion'],
                                  quantity_definition=quantities[contract['quantity_kind']],
                                  source=records[subject]))
        output.append(dict(attachment_id=identity, device_id=device_id,
                           model=device['model'], driver=device['driver'], adapter=adapter,
                           state='unbound' if adapter is None else 'declared', ports=projected))
    result = dict(schema='fisiCs.attachment_handoff_receipt', schema_version=version,
                status='verified', project_id=project, manifest_sha256=sha(raw),
                artifacts={k: manifest['artifacts'][k]['sha256'] for k in KINDS}, attachments=output,
                evidence=dict(artifact_integrity='verified', selected_reference_joins='verified',
                              authenticity='unsigned', full_graph_revalidation='not_performed',
                              live_source_freshness='not_checked', consumer_support='not_assessed',
                              execution='not_performed', physical_acceptance='not_assessed'))
    if version == 2:
        result['provenance_sha256'] = manifest['provenance']['sha256']
        result['evidence']['semantic_parent'] = 'producer_receipt_verified'
    return result


def verify_handoff(manifest_path):
    """Stable Python entry point. All rejected inputs raise HandoffError."""
    try:
        return _verify_handoff(manifest_path)
    except HandoffError:
        raise
    except (OSError, KeyError, TypeError, ValueError, RecursionError) as exc:
        raise HandoffError('invalid handoff: ' + str(exc)) from exc


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('manifest', type=Path)
    args = parser.parse_args()
    try:
        result = verify_handoff(args.manifest)
    except (HandoffError, OSError, KeyError, TypeError, ValueError, RecursionError) as exc:
        result = dict(schema='fisiCs.attachment_handoff_receipt', schema_version=1,
                      status='rejected', error=str(exc))
        sys.stdout.buffer.write(encoded(result))
        return 2
    sys.stdout.buffer.write(encoded(result))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
