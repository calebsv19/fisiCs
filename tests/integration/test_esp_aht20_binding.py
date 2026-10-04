"""Pure preflight and projection regressions; no ESP execution required."""
import copy
import os
import json
import tempfile
from pathlib import Path
import unittest
from unittest.mock import patch

import run_esp_aht20_binding as adapter
import run_esp_can_topology as topology


class BindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory(prefix='fisics-aht20-contract-')
        cls.addClassCleanup(cls.temp.cleanup)
        build = Path(cls.temp.name)
        compiler = Path(os.environ.get('FISICS_TEST_COMPILER', adapter.ROOT / 'fisics')).resolve()
        topology.emit_graphs(compiler, topology.DEFAULT_FIXTURE_ROOT, build)
        external = topology.load_contract(topology.DEFAULT_FIXTURE_ROOT / 'external.contract.json')
        topology.validate_graphs(build, external['topology_expectations'])
        cls.generated_graphs = {k: (build / f'{k}.graph.json').read_bytes()
                                for k in ('semantic', 'physical', 'hardware')}

    def setUp(self):
        self.binding = adapter.sim.load_json(adapter.FIXTURE)
        self.contract = adapter.sim.load_contract(adapter.sim.DEFAULT_FIXTURE_ROOT / 'contract.json')
        self.graphs = dict(self.generated_graphs)
        # Unit controls bind their freshly produced bytes. Historical external
        # execution pins remain unchanged in the authored integration contract.
        self.contract['static_graphs'] = {k: adapter.sim.sha256_bytes(raw)
                                          for k, raw in self.graphs.items()}
        self.profile = adapter.sim.canonical_bytes(dict(id='environment.aht20.v1',
                                                      state='admitted', adapter='family_v1'))
        self.binding['profile_sha256'] = adapter.sim.sha256_bytes(self.profile)

    def validate(self):
        return adapter.validate(self.binding, self.contract, self.graphs, self.profile)

    def test_valid(self):
        self.assertEqual(self.validate(), self.binding)

    def test_generic_verifier_is_required(self):
        with patch.object(adapter.aht20_declaration.handoff, 'verify_handoff',
                          side_effect=adapter.HandoffError('generic rejection')) as verify:
            with self.assertRaisesRegex(adapter.sim.SimulationError, 'generic rejection'):
                self.validate()
            verify.assert_called_once()

    def test_binding_rejections(self):
        for key, value in [('adapter', 'unknown'), ('device_id', 'unknown'),
                           ('profile_id', 'unknown'), ('schema_version', 2),
                           ('schema_version', True), ('scenario', 'faults'),
                           ('sensor', 'front_left'), ('instance', 11),
                           ('profile_sha256', '0' * 64)]:
            with self.subTest(key=key):
                original = copy.deepcopy(self.binding)
                self.binding[key] = value
                with self.assertRaises(adapter.sim.SimulationError):
                    self.validate()
                self.binding = original

    def test_port_rejections(self):
        for mutation in ('duplicate', 'missing', 'unit', 'subject_id', 'quantity_kind', 'source_field'):
            with self.subTest(mutation=mutation):
                original = copy.deepcopy(self.binding)
                if mutation == 'duplicate':
                    self.binding['ports'].append(copy.deepcopy(self.binding['ports'][0]))
                elif mutation == 'missing':
                    self.binding['ports'].pop()
                else:
                    self.binding['ports'][0][mutation] = 'wrong'
                with self.assertRaises(adapter.sim.SimulationError):
                    self.validate()
                self.binding = original

    def test_graph_drift(self):
        for kind in self.graphs:
            with self.subTest(kind=kind):
                old = self.graphs[kind]
                self.graphs[kind] += b' '
                with self.assertRaisesRegex(adapter.sim.SimulationError, 'digest mismatch'):
                    self.validate()
                self.graphs[kind] = old

    def test_missing_graph(self):
        del self.graphs['hardware']
        with self.assertRaisesRegex(adapter.sim.SimulationError, 'missing graph'):
            self.validate()

    def test_graph_semantic_checks_not_only_hashes(self):
        mutations = [
            ('hardware', lambda g: g['devices'][0]['model'].update(revision='unsupported')),
            ('hardware', lambda g: g['devices'].pop(0)),
            ('hardware', lambda g: g['signal_bindings'][0].update(result='incompatible')),
            ('physical', lambda g: g['physical_contracts'][0]['subject'].update(id='unknown')),
            ('physical', lambda g: g['physical_contracts'][0]['unit_policy'].update(unit='meter')),
            ('physical', lambda g: g['physical_contracts'][0]['value_form'].update(kind='vector')),
        ]
        for kind, mutate in mutations:
            with self.subTest(kind=kind, mutation=mutate):
                old = self.graphs[kind]
                old_digest = self.contract['static_graphs'][kind]
                value = json.loads(old)
                mutate(value)
                self.graphs[kind] = adapter.sim.canonical_bytes(value)
                self.contract['static_graphs'][kind] = adapter.sim.sha256_bytes(self.graphs[kind])
                with self.assertRaises(adapter.sim.SimulationError):
                    self.validate()
                self.graphs[kind] = old
                self.contract['static_graphs'][kind] = old_digest

    def test_samples_preserve_quality_and_convert_explicitly(self):
        sample = dict(raw=2000, sequence=4, validity=1, quality=0, session=2, received_ms=0)
        run = dict(family='aht20', records=[dict(at_us=1000, state=dict(signals=[
            dict(instance=12, sensor='rear_left', temperature=sample, humidity=dict(sample, raw=5000))]))])
        result = adapter.project(self.binding, run, self.contract)
        self.assertEqual(result[0]['sample'], sample)
        self.assertFalse(result[0]['usable'])
        self.assertEqual(result[0]['physical_value'], dict(numerator=5863, denominator=20, unit='kelvin'))
        self.assertEqual(result[1]['physical_value'], dict(numerator=1, denominator=2, unit='fraction'))
        run['records'][0]['state']['signals'] *= 2
        with self.assertRaisesRegex(adapter.sim.SimulationError, 'missing or duplicated'):
            adapter.project(self.binding, run, self.contract)

    def test_retained_readback_rejects_tampering(self):
        sample = dict(raw=2000, sequence=1, validity=0, quality=1, session=1, received_ms=0)
        run = dict(family='aht20', records=[dict(at_us=0, state=dict(signals=[
            dict(instance=12, sensor='rear_left', temperature=sample, humidity=sample)]))])
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            adapter.sim.canonical_write(root / 'binding.json', self.binding)
            adapter.sim.canonical_write(root / 'run.json', run)
            adapter.sim.canonical_write(root / 'projection-contract.json', self.contract)
            safe_name = self.binding['profile_id'].replace('.', '-')
            plan_bytes = adapter.sim.canonical_bytes(dict(profile_id=self.binding['profile_id'],
                                                          scenario='baseline'))
            for repeat in (1, 2):
                (root / f'{safe_name}.{repeat}.plan.json').write_bytes(plan_bytes)
                run_root = root / f'{safe_name}.{repeat}.run'
                run_root.mkdir()
                (run_root / 'plan.json').write_bytes(plan_bytes)
            for kind, data in self.graphs.items():
                (root / f'{kind}.graph.json').write_bytes(data)
            result = dict(schema='fisiCs.esp_bound_simulation', schema_version=1,
                          binding=self.binding,
                          binding_sha256=adapter.sim.sha256_path(root / 'binding.json'),
                          run_sha256=adapter.sim.sha256_path(root / 'run.json'),
                          profile_sha256=self.binding['profile_sha256'],
                          plan_sha256=adapter.sim.sha256_bytes(plan_bytes),
                          static_graphs=self.contract['static_graphs'],
                          records=adapter.project(self.binding, run, self.contract),
                          executions_completed=2, evidence='host_simulation',
                          physical_hardware=False, native_adoption='not_implemented',
                          retained_artifacts={str(p.relative_to(root)): adapter.sim.sha256_path(p)
                                              for p in root.rglob('*') if p.is_file()})
            adapter.sim.canonical_write(root / 'result.json', result)
            adapter.verify_result(root)
            claim_mutations = {'schema_version': (True, 2),
                               'executions_completed': (True, 200),
                               'evidence': ('physically_accepted',),
                               'physical_hardware': (1, True),
                               'native_adoption': ('installed_and_accepted',),
                               'profile_sha256': (True, '0' * 64),
                               'plan_sha256': (True, '0' * 64)}
            for field, values in claim_mutations.items():
                for value in values:
                    with self.subTest(field=field, value=value):
                        changed = copy.deepcopy(result)
                        changed[field] = value
                        adapter.sim.canonical_write(root / 'result.json', changed)
                        with self.assertRaises(adapter.sim.SimulationError):
                            adapter.verify_result(root)
                with self.subTest(field=field, value='missing'):
                    changed = copy.deepcopy(result)
                    del changed[field]
                    adapter.sim.canonical_write(root / 'result.json', changed)
                    with self.assertRaises(adapter.sim.SimulationError):
                        adapter.verify_result(root)
            adapter.sim.canonical_write(root / 'result.json', result)
            plan_copy = root / f'{safe_name}.2.run/plan.json'
            changed = copy.deepcopy(result)
            del changed['retained_artifacts'][str(plan_copy.relative_to(root))]
            adapter.sim.canonical_write(root / 'result.json', changed)
            with self.assertRaisesRegex(adapter.sim.SimulationError, 'plan digest mismatch'):
                adapter.verify_result(root)
            original_plan = plan_copy.read_bytes()
            plan_copy.write_bytes(original_plan + b' ')
            changed = copy.deepcopy(result)
            changed['retained_artifacts'][str(plan_copy.relative_to(root))] = adapter.sim.sha256_path(plan_copy)
            adapter.sim.canonical_write(root / 'result.json', changed)
            with self.assertRaisesRegex(adapter.sim.SimulationError, 'plan digest mismatch'):
                adapter.verify_result(root)
            plan_copy.write_bytes(original_plan)
            adapter.sim.canonical_write(root / 'result.json', result)
            original = (root / 'run.json').read_bytes()
            (root / 'run.json').write_bytes(original + b' ')
            with self.assertRaisesRegex(adapter.sim.SimulationError, 'artifact mismatch'):
                adapter.verify_result(root)
            (root / 'run.json').write_bytes(original)
            result['records'][0]['usable'] = False
            adapter.sim.canonical_write(root / 'result.json', result)
            with self.assertRaisesRegex(adapter.sim.SimulationError, 'projection mismatch'):
                adapter.verify_result(root)
            result['retained_artifacts']['../outside'] = '0' * 64
            adapter.sim.canonical_write(root / 'result.json', result)
            with self.assertRaisesRegex(adapter.sim.SimulationError, 'unsafe'):
                adapter.verify_result(root)


if __name__ == '__main__':
    unittest.main()
