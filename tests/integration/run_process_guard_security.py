#!/usr/bin/env python3
"""Exercise secure admission with real compiler processes and isolated state."""

from concurrent.futures import ThreadPoolExecutor
import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys
import tempfile
import threading
import time


def private(path):
    path.mkdir(mode=0o700)
    path.chmod(0o700)
    return path


def environment(runtime, guard, limit, timeout=0):
    return dict(os.environ, TMPDIR=str(runtime), FISICS_PROC_GUARD_DIR=str(guard),
                FISICS_MAX_PROCS=str(limit), FISICS_TIMEOUT_SEC=str(timeout))


def invoke(compiler, runtime, guard, limit):
    return subprocess.run([str(compiler), '--not-a-real-flag'],
                          env=environment(runtime, guard, limit),
                          capture_output=True, text=True, timeout=10)


def cli_reached(result):
    assert result.returncode == 1, (result.returncode, result.stderr)
    assert "unknown option '--not-a-real-flag'" in result.stderr, result.stderr
    assert 'refusing to start fisics' not in result.stderr, result.stderr


def refused(result):
    assert result.returncode == 1, (result.returncode, result.stderr)
    assert 'refusing to start fisics' in result.stderr, result.stderr
    assert 'unknown option' not in result.stderr, result.stderr


def entry(directory, pid, mode=0o600):
    path = directory / f'pid-{pid}'
    path.write_text(f'{pid}\n')
    path.chmod(mode)
    return path


def absent_pid():
    for pid in [2147483647, 1073741823, 536870911]:
        try:
            os.kill(pid, 0)
        except ProcessLookupError:
            return pid
        except (PermissionError, OverflowError):
            continue
    raise AssertionError('No absent PID available')


def wait_until(predicate, message, seconds=10):
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        if predicate():
            return
        time.sleep(0.01)
    raise AssertionError(message)


def stop(process):
    if process.poll() is None:
        try:
            os.killpg(process.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
    process.communicate(timeout=10)


def security_cases(compiler, root):
    runtime = private(root / 'runtime')
    for kind in ['symlink', 'mode', 'regular-file']:
        case = private(root / kind)
        target = case / 'guard'
        if kind == 'symlink':
            decoy = private(case / 'decoy')
            entry(decoy, os.getpid())
            target.symlink_to(decoy, target_is_directory=True)
        elif kind == 'mode':
            private(target).chmod(0o777)
        else:
            target.write_text('untouched')
        result = invoke(compiler, runtime, target, 1)
        cli_reached(result)
        assert 'no secure per-user process-guard directory' in result.stderr
        if kind == 'regular-file':
            assert target.read_text() == 'untouched'

    guard = private(root / 'invalid-entries')
    (guard / 'pid-not-numeric').write_text('ignored')
    entry(guard, os.getpid(), 0o644)
    for alias in [f'pid-+{os.getpid()}', f'pid-0{os.getpid()}']:
        (guard / alias).write_text('ignored')
        (guard / alias).chmod(0o600)
    decoy = root / 'decoy-file'
    decoy.write_text('untouched')
    decoy.chmod(0o600)
    (guard / f'pid-{os.getppid()}').symlink_to(decoy)
    (guard / f'pid-{absent_pid()}').mkdir(mode=0o700)
    cli_reached(invoke(compiler, runtime, guard, 1))
    assert decoy.read_text() == 'untouched'

    for kind in ['symlink-lock', 'insecure-lock']:
        guard = private(root / kind)
        if kind == 'symlink-lock':
            (guard / '.lock').symlink_to(decoy)
        else:
            (guard / '.lock').write_text('')
            (guard / '.lock').chmod(0o644)
        result = invoke(compiler, runtime, guard, 1)
        cli_reached(result)
        assert 'failed to lock secure process-guard directory' in result.stderr
        assert decoy.read_text() == 'untouched'

    guard = private(root / 'live')
    live = entry(guard, os.getpid())
    refused(invoke(compiler, runtime, guard, 1))
    other_runtime = private(root / 'different-runtime')
    refused(invoke(compiler, other_runtime, guard, 1))
    cli_reached(invoke(compiler, other_runtime, guard, 2))
    cli_reached(invoke(compiler, other_runtime, guard, 0))
    assert list(guard.glob('pid-*')) == [live]
    disabled_guard = root / 'disabled-does-not-create-state'
    cli_reached(invoke(compiler, runtime, disabled_guard, 0))
    assert not disabled_guard.exists()

    guard = private(root / 'stale')
    stale = entry(guard, absent_pid())
    cli_reached(invoke(compiler, runtime, guard, 1))
    assert not stale.exists()
    assert not list(guard.glob('pid-*'))


def linker_fixture(root):
    clang = shutil.which('clang')
    assert clang, 'clang is needed for a real successful link'
    linker = root / 'held-linker'
    linker.write_text(f'''#!{sys.executable}
import os
from pathlib import Path
import sys
import time
ready = Path(os.environ['FISICS_TEST_GUARD_READY'])
(ready / str(os.getppid())).write_text(str(os.getpid()))
deadline = time.monotonic() + 15
while not Path(os.environ['FISICS_TEST_GUARD_RELEASE']).exists():
    if time.monotonic() > deadline:
        raise SystemExit('held linker timed out')
    time.sleep(0.01)
os.execv({clang!r}, [{clang!r}, *sys.argv[1:]])
''')
    linker.chmod(0o700)
    source = root / 'program.c'
    source.write_text('int main(void) { return 0; }\n')
    return linker, source


def launch(compiler, case, runtime, guard, limit, linker, source, label, timeout=0):
    env = environment(runtime, guard, limit, timeout)
    env.update(FISICS_TEST_GUARD_READY=str(case / 'ready'),
               FISICS_TEST_GUARD_RELEASE=str(case / 'release'))
    return subprocess.Popen([str(compiler), f'--linker={linker}', str(source),
                             '-o', str(case / f'program-{label}')],
                            env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                            text=True, start_new_session=True)


def concurrent_case(compiler, root, limit, round_index=0, count=6):
    case = private(root / f'concurrent-{limit}-{round_index}')
    private(case / 'ready')
    guard = case / 'guard'
    runtimes = [private(case / f'runtime-{i}') for i in range(count)]
    linker, source = linker_fixture(case)
    barrier = threading.Barrier(count)

    def start(i):
        barrier.wait(timeout=10)
        return launch(compiler, case, runtimes[i], guard, limit, linker, source, i)

    with ThreadPoolExecutor(max_workers=count) as executor:
        processes = list(executor.map(start, range(count)))
    try:
        def settled():
            admitted = list((case / 'ready').iterdir())
            assert len(admitted) <= limit, ('over-admission', limit, len(admitted))
            return len(admitted) == limit and sum(p.poll() is not None for p in processes) == count - limit
        wait_until(settled, 'simultaneous compiler starts did not settle')
        probe_runtime = private(case / 'probe-runtime')
        refused(invoke(compiler, probe_runtime, guard, limit))
        cli_reached(invoke(compiler, probe_runtime, guard, limit + 1))
        (case / 'release').touch()
        accepted = 0
        rejected = 0
        for i, process in enumerate(processes):
            stdout, stderr = process.communicate(timeout=10)
            if process.returncode == 0:
                accepted += 1
                subprocess.run([str(case / f'program-{i}')], check=True, timeout=5)
            else:
                rejected += 1
                assert process.returncode == 1 and 'refusing to start fisics' in stderr, (stdout, stderr)
        assert (accepted, rejected) == (limit, count - limit)
        assert not list(guard.glob('pid-*'))
        return {'limit': limit, 'started_together': count, 'compiled_and_ran': accepted, 'refused': rejected}
    finally:
        (case / 'release').touch(exist_ok=True)
        for process in processes:
            stop(process)


def termination_case(compiler, root, watchdog):
    name = 'watchdog' if watchdog else 'cancelled'
    case = private(root / name)
    private(case / 'ready')
    runtime = private(case / 'runtime')
    guard = case / 'guard'
    linker, source = linker_fixture(case)
    process = launch(compiler, case, runtime, guard, 1, linker, source, name,
                     timeout=1 if watchdog else 0)
    try:
        if not watchdog:
            wait_until(lambda: bool(list((case / 'ready').iterdir())), 'compiler did not reach held linker')
            os.killpg(process.pid, signal.SIGTERM)
        # A short watchdog may fire during compilation before the held linker
        # starts. Its diagnostic and released guard are the required contract.
        stdout, stderr = process.communicate(timeout=8)
        assert process.returncode != 0, (stdout, stderr)
        if watchdog:
            assert 'watchdog timed out' in stderr, stderr
            assert not list(guard.glob('pid-*'))
        # Cancellation may leave a PID entry. A subsequent start must reclaim it.
        cli_reached(invoke(compiler, runtime, guard, 1))
        assert not list(guard.glob('pid-*'))
    finally:
        (case / 'release').touch(exist_ok=True)
        stop(process)


def check(compiler, root):
    compiler = Path(compiler).resolve()
    security_cases(compiler, private(root / 'security'))
    # Every round starts without a guard directory or lock file, exercising
    # exclusive lock initialization as well as serialized PID admission.
    waves = [concurrent_case(compiler, root, limit, round_index)
             for round_index in range(4) for limit in [1, 2]]
    termination_case(compiler, root, watchdog=False)
    termination_case(compiler, root, watchdog=True)
    print(json.dumps({'security': 'passed', 'cross_tmpdir': 'passed',
                      'concurrency': waves, 'normal_cleanup': 'passed',
                      'cancellation_recovery': 'passed', 'watchdog_cleanup': 'passed'}))


if __name__ == '__main__':
    if len(sys.argv) != 2:
        raise SystemExit('usage: run_process_guard_security.py /path/to/fisics')
    directory = tempfile.mkdtemp(prefix='fisics-process-guard-')
    try:
        check(sys.argv[1], Path(directory))
    except BaseException:
        print(f'Retained failed process-guard fixture: {directory}', file=sys.stderr)
        raise
    else:
        shutil.rmtree(directory)
    print('process guard security/concurrency contract passed')
