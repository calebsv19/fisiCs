#!/usr/bin/env python3
"""Create a new portable handoff from the already-generated public hardware example."""
import argparse
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
from attachment_handoff import KINDS, encoded, read, decode, sha, verify_handoff


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--graphs', type=Path, default=ROOT / 'build/examples/semantic_metadata/hardware_project')
    parser.add_argument('--out', required=True, type=Path)
    args = parser.parse_args()
    graphs = {k: read(args.graphs / f'{k}.graph.json') for k in KINDS}
    attachments = json.loads(read(ROOT / 'examples/semantic_metadata/hardware_project/attachments.json'))
    manifest = dict(schema='fisiCs.attachment_handoff', schema_version=1,
                    project_id=decode(graphs['semantic'])['project']['id'],
                    artifacts={k: dict(path=f'{k}.graph.json', sha256=sha(v)) for k, v in graphs.items()},
                    attachments=attachments)
    args.out.mkdir(parents=True, exist_ok=False)
    for kind, raw in graphs.items():
        (args.out / f'{kind}.graph.json').write_bytes(raw)
    path = args.out / 'handoff.json'
    path.write_bytes(encoded(manifest))
    receipt = verify_handoff(path)
    (args.out / 'receipt.json').write_bytes(encoded(receipt))
    print(path)


if __name__ == '__main__':
    main()
