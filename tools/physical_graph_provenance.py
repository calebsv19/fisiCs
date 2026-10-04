#!/usr/bin/env python3
"""Produce a physical graph and an opt-in semantic-parent receipt; preserve graph v1."""
import argparse
from pathlib import Path
import subprocess
import sys
import attachment_handoff as api


def produce(compiler, manifest_path, output):
    compiler, manifest_path = Path(compiler).resolve(), Path(manifest_path).resolve()
    manifest_raw = api.read(manifest_path)
    manifest = api.decode(manifest_raw)
    semantic_path = (manifest_path.parent / manifest['semantic_project_graph_path']).resolve()
    semantic_raw = api.read(semantic_path)
    compiler_hash = api.sha(compiler.read_bytes())
    output = Path(output)
    output.mkdir(parents=True, exist_ok=False)
    physical_path = output / 'physical.graph.json'
    command = [str(compiler), '--physical-project-manifest', str(manifest_path),
               '--emit-physical-project-graph-json', str(physical_path)]
    result = subprocess.run(command, capture_output=True, text=True)
    api.need(result.returncode == 0, 'physical compiler invocation failed: ' + result.stderr)
    api.need(api.read(semantic_path) == semantic_raw and api.read(manifest_path) == manifest_raw
             and api.sha(compiler.read_bytes()) == compiler_hash, 'producer inputs changed during invocation')
    physical_raw = api.read(physical_path)
    physical = api.decode(physical_raw)
    semantic = api.decode(semantic_raw)
    for kind, graph in (('physical', physical), ('semantic', semantic)):
        api.need(graph['schema'] == 'fisiCs.' + kind + '_project_graph' and
                 type(graph['schema_version']) is int and graph['schema_version'] == 1 and
                 graph['complete'] is True and graph['project']['id'] == manifest['project_id'],
                 'producer graph identity mismatch')
    api.need(physical['semantic_graph_receipt']['project_id'] == semantic['project']['id'] and
             Path(physical['semantic_graph_receipt']['path']).resolve() == semantic_path,
             'producer semantic parent mismatch')
    receipt = dict(schema='fisiCs.physical_graph_provenance', schema_version=1,
                   producer=dict(compiler_sha256=compiler_hash, operation='physical_project_graph'),
                   project_id=physical['project']['id'], manifest_sha256=api.sha(manifest_raw),
                   semantic=dict(sha256=api.sha(semantic_raw), length_bytes=len(semantic_raw)),
                   physical=dict(sha256=api.sha(physical_raw), length_bytes=len(physical_raw)))
    (output / 'semantic.graph.json').write_bytes(semantic_raw)
    (output / 'provenance.json').write_bytes(api.encoded(receipt))
    return receipt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', type=Path, required=True)
    parser.add_argument('--manifest', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    try:
        result = produce(args.compiler, args.manifest, args.out)
    except (OSError, ValueError, KeyError, TypeError) as exc:
        print('Physical provenance rejected: ' + str(exc), file=sys.stderr)
        return 2
    sys.stdout.buffer.write(api.encoded(result))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
