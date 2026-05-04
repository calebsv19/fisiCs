import difflib
import json
import os
import shlex
import shutil
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parent
REPO_ROOT = ROOT.parent.parent
META_DIR = ROOT / "meta"
META_INDEX_PATH = META_DIR / "index.json"
ARTIFACT_ROOT = REPO_ROOT / "build" / "tests" / "binary"

ALLOWED_ENV_KEYS = {
    "PATH",
    "HOME",
    "TMPDIR",
    "TMP",
    "TEMP",
    "LANG",
    "LC_ALL",
    "LC_CTYPE",
    "TERM",
    "USER",
    "LOGNAME",
    "SHELL",
}

RESOURCE_PROFILES = {
    "smoke": {"cpu_seconds": 2, "file_size_mb": 8, "open_files": 64},
    "default": {"cpu_seconds": 5, "file_size_mb": 32, "open_files": 128},
    "heavy": {"cpu_seconds": 15, "file_size_mb": 128, "open_files": 256},
}

LINK_FAILURE_MARKERS = (
    "undefined symbol",
    "undefined symbols",
    "undefined reference",
    "multiple definition",
    "duplicate symbol",
    "ld:",
    "linker command failed",
    "collect2: error",
)


def parse_csv_env(name):
    raw = os.environ.get(name, "").strip()
    if not raw:
        return []
    return [token.strip() for token in raw.split(",") if token.strip()]


def load_json(path):
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def iter_manifest_files():
    if META_INDEX_PATH.exists():
        meta_index = load_json(META_INDEX_PATH)
        manifests = meta_index.get("manifests")
        if isinstance(manifests, list) and manifests:
            for rel_path in manifests:
                yield META_DIR / rel_path
            return
    for path in sorted(META_DIR.glob("*.json")):
        if path.name == "index.json":
            continue
        data = load_json(path)
        if isinstance(data, dict) and "tests" in data:
            yield path


def load_meta():
    merged = {"version": 1, "suite": "binary", "tests": []}
    seen_ids = set()
    for manifest_path in iter_manifest_files():
        manifest = load_json(manifest_path)
        tests = manifest.get("tests", [])
        for test in tests:
            row = dict(test)
            test_id = str(row.get("id", ""))
            if not test_id:
                raise ValueError(f"missing test id in {manifest_path}")
            if test_id in seen_ids:
                raise ValueError(f"duplicate binary test id '{test_id}' in {manifest_path}")
            seen_ids.add(test_id)
            row["__manifest"] = manifest_path.name
            merged["tests"].append(row)
    return merged


def resolve_rel_path(rel_path):
    return ROOT / str(rel_path)


def diff_text(expected, actual, label):
    exp_lines = expected.splitlines(keepends=True)
    act_lines = actual.splitlines(keepends=True)
    return "".join(
        difflib.unified_diff(exp_lines, act_lines, fromfile=str(label), tofile="actual")
    )


def build_env(extra_overrides):
    env = {}
    for key in ALLOWED_ENV_KEYS:
        value = os.environ.get(key)
        if value is not None:
            env[key] = value
    if "PATH" not in env:
        env["PATH"] = "/usr/bin:/bin"
    if "LANG" not in env:
        env["LANG"] = "C"
    if "LC_ALL" not in env:
        env["LC_ALL"] = "C"
    for key, value in extra_overrides.items():
        env[str(key)] = str(value)
    return env


def _preexec_limits(profile_name):
    if os.name != "posix":
        return None
    profile = RESOURCE_PROFILES.get(profile_name, RESOURCE_PROFILES["default"])
    cpu_seconds = int(profile.get("cpu_seconds", 5))
    file_size_mb = int(profile.get("file_size_mb", 32))
    open_files = int(profile.get("open_files", 128))

    def _apply_limit(resource_module, limit_kind, soft_value):
        try:
            current_soft, current_hard = resource_module.getrlimit(limit_kind)
            if current_hard == resource_module.RLIM_INFINITY:
                target_soft = soft_value
            else:
                target_soft = min(soft_value, current_hard)
            target_soft = max(1, int(target_soft))
            resource_module.setrlimit(limit_kind, (target_soft, current_hard))
        except Exception:
            pass

    def _runner():
        try:
            import resource
        except Exception:
            return
        _apply_limit(resource, resource.RLIMIT_CPU, cpu_seconds)
        _apply_limit(resource, resource.RLIMIT_FSIZE, file_size_mb * 1024 * 1024)
        _apply_limit(resource, resource.RLIMIT_NOFILE, open_files)

    return _runner


def run_compile(cmd, cwd, env, timeout_sec, profile_name):
    try:
        proc = subprocess.run(
            cmd,
            cwd=cwd,
            env=env,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=timeout_sec,
            preexec_fn=_preexec_limits(profile_name),
        )
        return {
            "timed_out": False,
            "exit_code": proc.returncode,
            "output": proc.stdout,
        }
    except subprocess.TimeoutExpired as exc:
        return {
            "timed_out": True,
            "exit_code": None,
            "output": (exc.stdout or "") + (exc.stderr or ""),
        }


def run_program(cmd, cwd, env, stdin_text, timeout_sec, profile_name):
    try:
        proc = subprocess.run(
            cmd,
            cwd=cwd,
            env=env,
            input=stdin_text,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=timeout_sec,
            preexec_fn=_preexec_limits(profile_name),
        )
        return {
            "timed_out": False,
            "exit_code": proc.returncode,
            "stdout": proc.stdout,
            "stderr": proc.stderr,
        }
    except subprocess.TimeoutExpired as exc:
        return {
            "timed_out": True,
            "exit_code": None,
            "stdout": exc.stdout or "",
            "stderr": exc.stderr or "",
        }


def should_skip(test):
    skip_cfg = test.get("skip_if", {})
    if not isinstance(skip_cfg, dict):
        skip_cfg = {}
    missing_env = []
    for env_name in skip_cfg.get("missing_env", []):
        key = str(env_name)
        if not os.environ.get(key, "").strip():
            missing_env.append(key)
    missing_tools = []
    for tool in skip_cfg.get("missing_tools", []):
        if shutil.which(str(tool)) is None:
            missing_tools.append(str(tool))
    missing_pkg_modules = []
    pkg_modules = [str(m) for m in skip_cfg.get("missing_pkg_config_modules", [])]
    if pkg_modules:
        pkg_config = shutil.which("pkg-config")
        if pkg_config is None:
            if "pkg-config" not in missing_tools:
                missing_tools.append("pkg-config")
        else:
            for module in pkg_modules:
                probe = subprocess.run(
                    [pkg_config, "--exists", module],
                    stdout=subprocess.DEVNULL,
                    stderr=subprocess.DEVNULL,
                    text=True,
                )
                if probe.returncode != 0:
                    missing_pkg_modules.append(module)
    differential_with = str(test.get("differential_with", "")).strip()
    if differential_with == "clang":
        clang_bin = str(test.get("differential_compiler", "clang")).strip() or "clang"
        if shutil.which(clang_bin) is None and clang_bin not in missing_tools:
            missing_tools.append(clang_bin)
    if missing_env:
        return f"missing env: {', '.join(missing_env)}"
    if missing_tools:
        return f"missing tools: {', '.join(missing_tools)}"
    if missing_pkg_modules:
        return f"missing pkg-config modules: {', '.join(missing_pkg_modules)}"
    return None


def resolve_pkg_config_flags(test, env_overrides):
    modules = [str(module) for module in test.get("pkg_config_modules", [])]
    if not modules:
        return []
    pkg_config = shutil.which("pkg-config")
    if pkg_config is None:
        raise RuntimeError("pkg_config_modules requires pkg-config in PATH")
    probe = subprocess.run(
        [pkg_config, "--cflags", "--libs"] + modules,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        env=build_env(env_overrides),
    )
    if probe.returncode != 0:
        stderr = (probe.stderr or "").strip()
        stdout = (probe.stdout or "").strip()
        detail = stderr or stdout or "pkg-config failed"
        raise RuntimeError(f"pkg-config --cflags --libs {' '.join(modules)} failed: {detail}")
    return shlex.split(probe.stdout)


def classify_compile_failure(output_text):
    lowered = (output_text or "").lower()
    for marker in LINK_FAILURE_MARKERS:
        if marker in lowered:
            return "link_fail"
    return "compile_fail"


def classify_binary_trust_layer(category, input_count, level, differential_with):
    if category == "runtime" or differential_with:
        return "Layer E"
    if category in ("link_fail", "link_only") or input_count > 1 or level == "link":
        return "Layer D"
    return "Layer C"


def classify_binary_owner_lane(category, input_count, level):
    if category == "runtime":
        return "binary-runtime"
    if category in ("link_fail", "link_only") or input_count > 1 or level == "link":
        return "binary-link"
    return "binary-compile"


def emit_binary_failure(
    message,
    *,
    failure_kind,
    severity,
    raw_status,
    test_id=None,
    category="",
    input_count=1,
    level="",
    differential_with="",
):
    trust_layer = classify_binary_trust_layer(
        category, input_count, level, differential_with
    )
    owner_lane = (
        classify_binary_owner_lane(category, input_count, level)
        if (category or input_count > 1 or level)
        else "binary-harness"
    )
    prefix = "FAIL"
    if test_id:
        prefix += f" {test_id}"
    print(
        f"{prefix} [failure_kind={failure_kind} severity={severity} "
        f"source_lane=binary trust_layer={trust_layer} owner_lane={owner_lane} "
        f"raw_status={raw_status}]: {message}"
    )
