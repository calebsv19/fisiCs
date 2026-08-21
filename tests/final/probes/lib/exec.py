import os
import signal
import subprocess


def _timeout_text(value):
    if value is None:
        return ""
    if isinstance(value, bytes):
        return value.decode("utf-8", errors="replace")
    return str(value)


def _merge_timeout_output(exc):
    return _timeout_text(exc.stdout) + _timeout_text(exc.stderr)


def _terminate_process_group(proc):
    """Stop and reap a timed-out command together with its compiler children."""
    try:
        os.killpg(proc.pid, signal.SIGTERM)
    except ProcessLookupError:
        pass

    try:
        return proc.communicate(timeout=1)
    except subprocess.TimeoutExpired:
        try:
            os.killpg(proc.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
        return proc.communicate()


def _run_process(cmd, timeout_sec, env, stdout, stderr, input_text=None):
    proc = subprocess.Popen(
        cmd,
        stdin=subprocess.PIPE if input_text is not None else None,
        stdout=stdout,
        stderr=stderr,
        text=True,
        encoding="utf-8",
        errors="replace",
        env=env,
        start_new_session=True,
    )
    try:
        stdout_text, stderr_text = proc.communicate(
            input=input_text, timeout=timeout_sec
        )
        return proc.returncode, stdout_text, stderr_text, False
    except subprocess.TimeoutExpired as exc:
        stdout_text, stderr_text = _terminate_process_group(proc)
        return (
            124,
            stdout_text if stdout_text is not None else exc.stdout,
            stderr_text if stderr_text is not None else exc.stderr,
            True,
        )


def run_cmd(cmd, timeout_sec, env=None):
    exit_code, stdout, stderr, timed_out = _run_process(
        cmd, timeout_sec, env, subprocess.PIPE, subprocess.STDOUT
    )
    if timed_out:
        return exit_code, _timeout_text(stdout) + _timeout_text(stderr), True
    return exit_code, stdout, False


def run_binary(path, timeout_sec, env=None):
    exit_code, stdout, stderr, timed_out = _run_process(
        [str(path)], timeout_sec, env, subprocess.PIPE, subprocess.PIPE
    )
    return exit_code, stdout, stderr, timed_out
