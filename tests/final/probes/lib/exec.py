import subprocess


def _timeout_text(value):
    if value is None:
        return ""
    if isinstance(value, bytes):
        return value.decode("utf-8", errors="replace")
    return str(value)


def _merge_timeout_output(exc):
    return _timeout_text(exc.stdout) + _timeout_text(exc.stderr)


def run_cmd(cmd, timeout_sec, env=None):
    try:
        proc = subprocess.run(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=timeout_sec,
            env=env,
        )
    except subprocess.TimeoutExpired as exc:
        out = _merge_timeout_output(exc)
        return 124, out, True
    return proc.returncode, proc.stdout, False


def run_binary(path, timeout_sec, env=None):
    try:
        proc = subprocess.run(
            [str(path)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=timeout_sec,
            env=env,
        )
    except subprocess.TimeoutExpired as exc:
        out = _timeout_text(exc.stdout)
        err = _timeout_text(exc.stderr)
        return 124, out, err, True
    return proc.returncode, proc.stdout, proc.stderr, False
