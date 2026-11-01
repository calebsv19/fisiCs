"""Release-copy isolation, input identity, output occupancy and goal boundaries."""
import hashlib
import importlib.util
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('producer', ROOT / 'scripts/prepare_release_producer.py')
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)


class PreparationTests(unittest.TestCase):
    def seed(self, path):
        path.mkdir()
        subprocess.run(['git', 'init', '-q', str(path)], check=True)
        (path / '.gitignore').write_text('build/\npersonal/\n')
        (path / 'source.c').write_text('int seed;\n')
        subprocess.run(['git', '-C', str(path), 'add', '.'], check=True)
        subprocess.run(['git', '-C', str(path), '-c', 'user.name=Fixture', '-c',
                        'user.email=fixture@example.invalid', 'commit', '-qm', 'seed'], check=True)
        return path

    def test_copy_preserves_dirty_input_identity_and_omits_ignored_outputs(self):
        with tempfile.TemporaryDirectory() as temp:
            t = Path(temp).resolve(); source = self.seed(t / 'source')
            (source / 'source.c').write_text('int changed;\n')
            (source / 'new.c').write_text('int untracked;\n')
            for name in ('build', 'personal'):
                (source / name).mkdir(); (source / name / 'sentinel').write_text('protected')
            before = {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in source.rglob('*') if p.is_file()}
            dest = m.prepare(source, t / 'owned')
            self.assertEqual((dest / 'source.c').read_bytes(), (source / 'source.c').read_bytes())
            self.assertTrue((dest / 'new.c').exists())
            self.assertFalse((dest / 'build').exists()); self.assertFalse((dest / 'personal').exists())
            self.assertEqual(subprocess.check_output(['git', '-C', str(source), 'rev-parse', 'HEAD']), subprocess.check_output(['git', '-C', str(dest), 'rev-parse', 'HEAD']))
            self.assertEqual(subprocess.check_output(['git', '-C', str(dest), 'remote']), b'')
            with self.assertRaisesRegex(ValueError, 'already exists'): m.prepare(source, t / 'owned')
            self.assertEqual(before, {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in source.rglob('*') if p.is_file()})

    def test_overlap_relative_and_symlink_outputs_refuse(self):
        with tempfile.TemporaryDirectory() as temp:
            t = Path(temp).resolve(); source = self.seed(t / 'source')
            for output in (source / 'build/new', t, Path('relative/output')):
                with self.assertRaises(ValueError): m.checked_output(output, source)
            (t / 'link').symlink_to(t / 'absent')
            with self.assertRaisesRegex(ValueError, 'symlink'): m.checked_output(t / 'link/out', source)

    def test_source_symlink_and_nonregular_inputs_refuse(self):
        with tempfile.TemporaryDirectory() as temp:
            t = Path(temp).resolve(); source = self.seed(t / 'source')
            (source / 'link').symlink_to(source / 'source.c')
            with self.assertRaisesRegex(ValueError, 'ordinary'): m.inputs(source)
            (source / 'link').unlink(); (source / 'source.c').unlink(); os.mkfifo(source / 'source.c')
            with self.assertRaisesRegex(ValueError, 'ordinary'): m.inputs(source)

    def test_export_exact_and_occupied_output_preserved(self):
        with tempfile.TemporaryDirectory() as temp:
            t = Path(temp).resolve(); source = t / 'package'; source.mkdir()
            (source / 'artifact.zip').write_bytes(b'archive fixture')
            output = t / 'export'; m.export_package(source, output)
            self.assertEqual((output / 'artifact.zip').read_bytes(), b'archive fixture')
            with self.assertRaisesRegex(ValueError, 'already exists'): m.export_package(source, output)
            self.assertEqual((output / 'artifact.zip').read_bytes(), b'archive fixture')

    def test_symlink_package_refuses_before_export(self):
        with tempfile.TemporaryDirectory() as temp:
            t = Path(temp).resolve(); source = t / 'package'; source.mkdir()
            (source / 'link').symlink_to(t / 'outside')
            with self.assertRaisesRegex(ValueError, 'non-ordinary'): m.export_package(source, t / 'export')
            self.assertFalse((t / 'export').exists())

    def test_authentication_keeps_owned_root_and_inherited_owner_context(self):
        with tempfile.TemporaryDirectory() as temp:
            t = Path(temp).resolve(); dest = t / 'copy'; dest.mkdir()
            argv = ['producer', '--root', str(t / 'build'), '--make', '/usr/bin/make',
                    '--compiler', '/usr/bin/clang', '--release-root', str(t / 'authenticated'),
                    '--', 'release-notarize']
            with patch('sys.argv', argv), patch.object(m, 'prepare', return_value=dest), patch.object(m.subprocess, 'call', return_value=0) as run:
                self.assertEqual(m.main(), 0)
            command = run.call_args.args[0]
            self.assertIn('PREPARATION_ROOT=', command)
            self.assertIn('RELEASE_ROOT=' + str(t / 'authenticated'), command)
            self.assertEqual(command[-1], 'release-notarize')
            self.assertIn('MAKEFLAGS', run.call_args.kwargs['env']) if 'MAKEFLAGS' in os.environ else None
            q = subprocess.run(['make', '-C', str(ROOT), 'PREPARATION_ROOT=' + str(t / 'absent'), 'release-notarize', 'dev-smoke'], capture_output=True, text=True)
            self.assertNotEqual(q.returncode, 0)
            self.assertIn('cannot mix', q.stderr)
            self.assertFalse((t / 'absent').exists())

    def test_forbidden_goals_refuse_before_output(self):
        with tempfile.TemporaryDirectory() as temp:
            output = Path(temp).resolve() / 'absent'
            for goal in ('clean', 'release-sign', 'release-publish', 'all'):
                q = subprocess.run(['make', '-C', str(ROOT), 'PREPARATION_ROOT=' + str(output), goal], capture_output=True, text=True)
                self.assertNotEqual(q.returncode, 0)
                self.assertIn('Unsupported contained preparation goal', q.stderr)
                self.assertFalse(output.exists())


if __name__ == '__main__':
    unittest.main()
