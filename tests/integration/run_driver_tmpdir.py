#!/usr/bin/env python3
"""Compile/link with observed object custody and cleanup under standard TMPDIR.

Run: python3 tests/integration/run_driver_tmpdir.py /absolute/path/to/fisics
On macOS, the positive configured-directory case runs in a real process sandbox
that denies writes outside the private fixture, including global /tmp objects.
"""
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


def check(compiler, root):
    compiler = Path(compiler).resolve()
    clang = shutil.which('clang')
    if not clang:
        raise RuntimeError('clang linker is required')
    env = dict(os.environ)
    env.update(FISICS_MAX_PROCS='0', XCRUN_CACHE_DISABLE='1')
    if sys.platform == 'darwin':
        env['SDKROOT'] = subprocess.check_output(
            ['/usr/bin/xcrun', '--show-sdk-path'], text=True).strip()
    rows = []
    cases = [('spaces', 'private objects'), ('trailing', 'private objects/'),
             ('long', '/'.join(['long'] + ['x' * 100] * 5)),
             ('unset', None), ('empty', ''), ('missing', 'absent'),
             ('not_directory', 'regular-file'), ('link_failure', 'link objects')]
    for name, value in cases:
        case = root / name
        case.mkdir()
        (case / 'main.c').write_text('int helper(void); int main(void) { return helper(); }\n')
        (case / 'helper.c').write_text('int helper(void) { return 0; }\n')
        trace = case / 'linker.json'
        wrapper = case / 'linker'
        wrapper.write_text('#!' + sys.executable + '\n' +
            'import json, os, sys\nfrom pathlib import Path\n' +
            'objects = [x for x in sys.argv[1:] if x.endswith(".o")]\n' +
            'Path(' + repr(str(trace)) + ').write_text(json.dumps({"objects":objects,"existed":[Path(x).is_file() for x in objects]}))\n' +
            ('raise SystemExit(23)\n' if name == 'link_failure' else
             'os.execv(' + repr(clang) + ', [' + repr(clang) + ', *sys.argv[1:]])\n'))
        wrapper.chmod(0o755)
        selected = dict(env)
        if value is None:
            selected.pop('TMPDIR', None)
        elif value == '':
            selected['TMPDIR'] = ''
        else:
            target = case / value
            if name == 'not_directory':
                target.write_text('not a directory')
            elif name != 'missing':
                target.mkdir(parents=True, exist_ok=True)
            selected['TMPDIR'] = str(target) + ('/' if name == 'trailing' else '')
        argv = [str(compiler), 'main.c', 'helper.c', '--linker=' + str(wrapper), '-o', 'program']
        sandboxed = sys.platform == 'darwin' and name == 'spaces'
        if sandboxed:
            profile = ('(version 1)(allow default)(deny network*)(deny file-write*)'
                       '(allow file-write* (subpath ' + json.dumps(str(root)) + '))'
                       '(allow file-write* (literal "/dev/null"))')
            argv = ['/usr/bin/sandbox-exec', '-p', profile, *argv]
        result = subprocess.run(argv, cwd=case, env=selected, capture_output=True, text=True)
        invalid = name in ('missing', 'not_directory')
        assert (result.returncode == 0) == (not invalid and name != 'link_failure'), (name, result.returncode, result.stderr)
        assert (case / 'program').exists() == (not invalid and name != 'link_failure'), name
        observed = None
        if invalid:
            assert 'mkstemp' in result.stderr and not trace.exists(), (name, result.stderr)
        else:
            observed = json.loads(trace.read_text())
            assert len(observed['objects']) == 2 and all(observed['existed']), (name, observed)
            expected = Path(selected.get('TMPDIR') or '/tmp').resolve()
            assert all(Path(x).parent.resolve() == expected for x in observed['objects']), (name, observed)
            assert all(not Path(x).exists() for x in observed['objects']), (name, 'temporary object leaked')
            if name != 'link_failure':
                assert subprocess.run([str(case / 'program')], cwd=case).returncode == 0, name
        rows.append({'case': name, 'pass': True, 'returncode': result.returncode,
                     'sandboxed': sandboxed, 'temporary_objects': observed})
    return rows


if __name__ == '__main__':
    with tempfile.TemporaryDirectory(prefix='fisics-driver-tmpdir-') as directory:
        rows = check(sys.argv[1], Path(directory).resolve())
    print(json.dumps({'passed': True, 'tests_run': len(rows), 'cases': rows}, indent=2))
