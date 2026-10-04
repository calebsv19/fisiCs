#!/usr/bin/env python3
"""Stable handoff tests over freshly compiler-produced public example graphs."""
import copy
import os
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
COMPILER = Path(os.environ.get('FISICS_TEST_COMPILER', ROOT / 'fisics')).resolve()
EXAMPLES_BUILD = Path(os.environ.get('FISICS_EXAMPLES_BUILD_DIR', ROOT / 'build/examples')).resolve()
sys.path.insert(0, str(ROOT / 'tools'))
import attachment_handoff as api


class HandoffTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='fisics-handoff-test-')
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        source = EXAMPLES_BUILD / 'semantic_metadata/hardware_project'
        self.graphs = {k: (source / f'{k}.graph.json').read_bytes() for k in api.KINDS}
        for k, raw in self.graphs.items():
            (self.root / f'{k}.graph.json').write_bytes(raw)
        self.manifest = dict(schema='fisiCs.attachment_handoff', schema_version=1,
                             project_id=api.decode(self.graphs['semantic'])['project']['id'],
                             artifacts={k: dict(path=f'{k}.graph.json', sha256=api.sha(raw))
                                        for k, raw in self.graphs.items()},
                             attachments=json.loads((ROOT / 'examples/semantic_metadata/hardware_project/attachments.json').read_text()))
        self.path = self.root / 'handoff.json'

    def verify(self):
        self.path.write_bytes(api.encoded(self.manifest))
        return api.verify_handoff(self.path)

    def rejected(self):
        with self.assertRaises(api.HandoffError):
            self.verify()

    def test_two_physical_semantics_and_states(self):
        result = self.verify()
        self.assertEqual([a['state'] for a in result['attachments']], ['unbound', 'declared'])
        self.assertEqual([a['ports'][0]['physical_contract']['reference_semantics']['kind']
                          for a in result['attachments']], ['difference', 'absolute'])
        self.assertEqual(result['evidence']['consumer_support'], 'not_assessed')
        self.assertEqual(api.encoded(result), api.encoded(self.verify()))

    def test_schema_versions_and_unknown_fields(self):
        for key, value in [('schema_version', True), ('schema_version', 2), ('schema', 'other'),
                           ('project_id', 'other'), ('unknown', 1)]:
            with self.subTest(key=key, value=value):
                original = copy.deepcopy(self.manifest)
                self.manifest[key] = value
                self.rejected()
                self.manifest = original

    def test_port_physical_contract_mismatches(self):
        port = self.manifest['attachments'][0]['ports'][0]
        for field, value in [('quantity_kind', 'different'), ('unit_policy', {'mode': 'dimension_only', 'unit': None}),
                             ('value_form', {'kind': 'vector'}), ('frame_id', 'unknown'),
                             ('reference_semantics', {'kind': 'difference'})]:
            with self.subTest(field=field):
                old = copy.deepcopy(port['physical_contract'])
                port['physical_contract'][field] = value
                self.rejected()
                port['physical_contract'] = old

    def test_subject_device_and_port_identity(self):
        for key in ('subject_id', 'port_id'):
            port = self.manifest['attachments'][0]['ports'][0]
            old = port[key]; port[key] = 'missing'; self.rejected(); port[key] = old
        self.manifest['attachments'][0]['device_id'] = 'missing'
        self.rejected()

    def test_duplicate_and_missing_assignments(self):
        original = copy.deepcopy(self.manifest)
        for kind in ('attachment', 'device', 'port', 'required', 'empty'):
            with self.subTest(kind=kind):
                self.manifest = copy.deepcopy(original)
                items = self.manifest['attachments']
                if kind in ('attachment', 'device'):
                    item = copy.deepcopy(items[0])
                    if kind == 'device': item['attachment_id'] = 'another'
                    items.append(item)
                elif kind == 'port': items[0]['ports'].append(copy.deepcopy(items[0]['ports'][0]))
                elif kind == 'required': items[0]['ports'] = []
                else: self.manifest['attachments'] = []
                self.rejected()

    def test_paths_and_digests(self):
        descriptor = self.manifest['artifacts']['semantic']
        for value in ('../semantic.graph.json', '/tmp/graph.json', './semantic.graph.json', 'a\\b'):
            descriptor['path'] = value; self.rejected()
        descriptor['path'] = 'semantic.graph.json'
        descriptor['sha256'] = '0' * 64; self.rejected()

    def test_symlink_and_corrupt_bytes(self):
        path = self.root / 'semantic.graph.json'
        path.write_bytes(self.graphs['semantic'] + b' '); self.rejected()
        path.unlink(); path.symlink_to(ROOT / 'README.md'); self.rejected()

    def test_duplicate_json_keys_and_nonfinite(self):
        for raw in (b'{"a":1,"a":2}', b'{"a":NaN}'):
            self.path.write_bytes(raw)
            with self.assertRaises(api.HandoffError): api.verify_handoff(self.path)

    def test_manifest_and_artifact_size_limits(self):
        # A known valid manifest and graph are controls for both independent read limits.
        self.verify()
        with self.path.open('wb') as stream:
            stream.truncate(16 * 1024 * 1024 + 1)
        with self.assertRaisesRegex(api.HandoffError, '16 MiB'):
            api.verify_handoff(self.path)
        graph_path = self.root / 'semantic.graph.json'
        with graph_path.open('wb') as stream:
            stream.truncate(16 * 1024 * 1024 + 1)
        self.path.write_bytes(api.encoded(self.manifest))
        with self.assertRaisesRegex(api.HandoffError, '16 MiB'):
            api.verify_handoff(self.path)

    def test_descriptor_type_and_nested_symlink_rejection(self):
        self.verify()
        descriptor = self.manifest['artifacts']['semantic']
        for value in (True, 1, None, [], {}):
            with self.subTest(value=value):
                descriptor['sha256'] = value
                self.rejected()
        descriptor['sha256'] = api.sha(self.graphs['semantic'])
        nested = self.root / 'nested'
        nested.symlink_to(self.root, target_is_directory=True)
        descriptor['path'] = 'nested/semantic.graph.json'
        self.rejected()

    def test_graph_schema_and_receipt_join(self):
        for kind, field, value in [('hardware', 'schema_version', True), ('physical', 'complete', False)]:
            original = self.graphs[kind]
            data = api.decode(original); data[field] = value
            raw = api.encoded(data)
            (self.root / f'{kind}.graph.json').write_bytes(raw)
            self.manifest['artifacts'][kind]['sha256'] = api.sha(raw)
            self.rejected()
            (self.root / f'{kind}.graph.json').write_bytes(original)
            self.manifest['artifacts'][kind]['sha256'] = api.sha(original)
        data = api.decode(self.graphs['hardware'])
        data['physical_graph_receipt']['hash_fnv1a64'] = '0' * 16
        raw = api.encoded(data)
        (self.root / 'hardware.graph.json').write_bytes(raw)
        self.manifest['artifacts']['hardware']['sha256'] = api.sha(raw)
        self.rejected()

    def test_cli_relocation_and_no_source_dependency(self):
        self.verify()
        moved = self.root / 'moved'
        moved.mkdir()
        for p in list(self.root.glob('*.json')): shutil.copy2(p, moved / p.name)
        helper = self.root / 'attachment_handoff.py'
        shutil.copy2(ROOT / 'tools/attachment_handoff.py', helper)
        command = [sys.executable, '-B', str(helper)]
        first = subprocess.run(command + [str(self.path)], cwd='/', capture_output=True)
        second = subprocess.run(command + [str(moved / 'handoff.json')], cwd='/', capture_output=True)
        self.assertEqual(first.returncode, 0, first.stderr)
        self.assertEqual(first.stdout, second.stdout)
        rejected = subprocess.run(command + [str(moved / 'missing.json')], capture_output=True)
        self.assertEqual(rejected.returncode, 2)
        self.assertEqual(json.loads(rejected.stdout)['status'], 'rejected')

    def test_embedded_source_paths_are_provenance_only(self):
        graph = api.decode(self.graphs['semantic'])
        for record in graph['records']:
            record['source']['file'] = '/nonexistent/historical/source.c'
        raw = api.encoded(graph)
        (self.root / 'semantic.graph.json').write_bytes(raw)
        self.manifest['artifacts']['semantic']['sha256'] = api.sha(raw)
        result = self.verify()
        self.assertEqual(result['evidence']['live_source_freshness'], 'not_checked')
        self.assertEqual(result['evidence']['full_graph_revalidation'], 'not_performed')


if __name__ == '__main__':
    unittest.main()
