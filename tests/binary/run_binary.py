#!/usr/bin/env python3
import difflib
import fnmatch
import json
import os
import shlex
import shutil
import subprocess
import sys
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


def main():
    raw_bin_path = sys.argv[1] if len(sys.argv) > 1 else "./fisics"
    bin_path = str(Path(raw_bin_path).resolve())
    if not Path(bin_path).exists():
        print(f"FAIL: compiler binary not found at {bin_path}")
        return 1
    update = os.environ.get("UPDATE_BINARY", "0") == "1" or "--update" in sys.argv

    filt = os.environ.get("BINARY_FILTER", "").strip()
    prefixes = parse_csv_env("BINARY_PREFIX")
    levels = parse_csv_env("BINARY_LEVEL")
    manifests = parse_csv_env("BINARY_MANIFEST")
    has_selector = bool(filt or prefixes or levels or manifests)

    if update and not has_selector:
        print("FAIL: UPDATE_BINARY requires selector (BINARY_FILTER/BINARY_PREFIX/BINARY_LEVEL/BINARY_MANIFEST)")
        return 1

    meta = load_meta()
    tests = meta.get("tests", [])

    failures = 0
    skipped = 0
    selected = 0

    for test in tests:
        test_id = str(test.get("id"))
        test_level = str(test.get("level", ""))
        test_manifest = str(test.get("__manifest", ""))

        if filt and test_id != filt:
            continue
        if prefixes and not any(test_id.startswith(prefix) for prefix in prefixes):
            continue
        if levels and test_level not in levels:
            continue
        if manifests and not any(
            token == test_manifest or token in test_manifest or test_manifest.endswith(token)
            for token in manifests
        ):
            continue

        selected += 1

        skip_reason = should_skip(test)
        if skip_reason:
            print(f"SKIP {test_id}: {skip_reason}")
            skipped += 1
            continue

        category = str(test.get("category", "runtime"))
        args = [str(a) for a in test.get("args", [])]
        env_overrides = test.get("env", {})
        run_env_overrides = test.get("run_env", {})
        profile_name = str(test.get("resource_profile", "default"))

        inputs = test.get("inputs")
        if inputs:
            input_paths = [resolve_rel_path(p) for p in inputs]
        else:
            input_paths = [resolve_rel_path(test["input"])]

        run_dir = ARTIFACT_ROOT / test_id
        if run_dir.exists():
            shutil.rmtree(run_dir)
        run_dir.mkdir(parents=True, exist_ok=True)

        compile_timeout = int(test.get("compile_timeout_sec", test.get("timeout_sec", 10)))
        try:
            pkg_config_flags = resolve_pkg_config_flags(test, env_overrides)
        except RuntimeError as exc:
            print(f"FAIL {test_id} [harness_error]: {exc}")
            failures += 1
            continue

        if category == "compile_only" or category == "compile_fail":
            if len(input_paths) != 1:
                print(
                    f"FAIL {test_id} [harness_error]: "
                    f"{category} requires exactly one input"
                )
                failures += 1
                continue
            output_path = run_dir / "artifact.o"
            compile_cmd = (
                [bin_path]
                + args
                + [str(input_paths[0]), "-c"]
                + pkg_config_flags
                + ["-o", str(output_path)]
            )
        elif category == "runtime" or category == "link_fail" or category == "link_only":
            output_path = run_dir / "a.out"
            compile_cmd = (
                [bin_path]
                + args
                + [str(p) for p in input_paths]
                + pkg_config_flags
                + ["-o", str(output_path)]
            )
        else:
            print(f"FAIL {test_id} [harness_error]: unsupported category '{category}'")
            failures += 1
            continue

        compile_result = run_compile(
            compile_cmd,
            cwd=run_dir,
            env=build_env(env_overrides),
            timeout_sec=compile_timeout,
            profile_name=profile_name,
        )
        expect_compile_timeout = bool(test.get("expect_compile_timeout", False))
        if compile_result["timed_out"]:
            if expect_compile_timeout:
                print(f"PASS {test_id}")
            else:
                print(f"FAIL {test_id} [compile_timeout]: compiler timed out after {compile_timeout}s")
                failures += 1
            continue
        if expect_compile_timeout:
            print(
                f"FAIL {test_id} [compile_timeout_unexpected]: "
                f"expected compiler timeout after {compile_timeout}s"
            )
            failures += 1
            continue

        if category == "link_fail":
            if compile_result["exit_code"] == 0:
                print(f"FAIL {test_id} [compile_succeeded_unexpected]: expected link failure")
                failures += 1
                continue
            failure_kind = classify_compile_failure(compile_result["output"])
            if failure_kind != "link_fail":
                print(
                    f"FAIL {test_id} [compile_fail_unexpected]: expected link-stage failure, "
                    f"got compile-stage failure"
                )
                if compile_result["output"]:
                    print(compile_result["output"])
                failures += 1
                continue
            contains = [str(x) for x in test.get("expect_output_contains", [])]
            missing_contains = []
            lowered_output = (compile_result["output"] or "").lower()
            for fragment in contains:
                if fragment.lower() not in lowered_output:
                    missing_contains.append(fragment)
            if missing_contains:
                print(
                    f"FAIL {test_id} [link_fail_unexpected]: missing expected output fragments: "
                    f"{', '.join(missing_contains)}"
                )
                if compile_result["output"]:
                    print(compile_result["output"])
                failures += 1
                continue
            print(f"PASS {test_id}")
            continue

        if category == "compile_fail":
            if compile_result["exit_code"] == 0:
                print(f"FAIL {test_id} [compile_succeeded_unexpected]: expected compile failure")
                failures += 1
                continue
            failure_kind = classify_compile_failure(compile_result["output"])
            if failure_kind != "compile_fail":
                print(
                    f"FAIL {test_id} [link_fail_unexpected]: expected compile-stage failure, "
                    f"got link-stage failure"
                )
                if compile_result["output"]:
                    print(compile_result["output"])
                failures += 1
                continue
            contains = [str(x) for x in test.get("expect_output_contains", [])]
            missing_contains = []
            lowered_output = (compile_result["output"] or "").lower()
            for fragment in contains:
                if fragment.lower() not in lowered_output:
                    missing_contains.append(fragment)
            if missing_contains:
                print(
                    f"FAIL {test_id} [compile_fail_unexpected]: missing expected output fragments: "
                    f"{', '.join(missing_contains)}"
                )
                if compile_result["output"]:
                    print(compile_result["output"])
                failures += 1
                continue
            print(f"PASS {test_id}")
            continue

        if compile_result["exit_code"] != 0:
            failure_kind = classify_compile_failure(compile_result["output"])
            fail_code = "link_fail_unexpected" if failure_kind == "link_fail" else "compile_fail_unexpected"
            print(f"FAIL {test_id} [{fail_code}]: compiler exited {compile_result['exit_code']}")
            if compile_result["output"]:
                print(compile_result["output"])
            failures += 1
            continue
        if not output_path.exists():
            print(f"FAIL {test_id} [harness_error]: expected output missing: {output_path}")
            failures += 1
            continue

        if category == "compile_only" or category == "link_only":
            print(f"PASS {test_id}")
            continue

        run_cmdline = [str(output_path)] + [str(a) for a in test.get("run_args", [])]
        run_timeout = int(test.get("run_timeout_sec", test.get("timeout_sec", 5)))
        run_result = run_program(
            run_cmdline,
            cwd=run_dir,
            env=build_env(run_env_overrides),
            stdin_text=test.get("run_stdin"),
            timeout_sec=run_timeout,
            profile_name=profile_name,
        )
        if run_result["timed_out"]:
            print(f"FAIL {test_id} [runtime_timeout]: program timed out after {run_timeout}s")
            failures += 1
            continue

        test_failed = False
        try:
            expect_exit = int(test.get("expect_exit", 0))
        except (TypeError, ValueError):
            print(f"FAIL {test_id} [harness_error]: expect_exit must be an integer")
            failures += 1
            continue
        if run_result["exit_code"] != expect_exit:
            print(
                f"FAIL {test_id} [runtime_exit_mismatch]: expected {expect_exit}, got {run_result['exit_code']}"
            )
            failures += 1
            test_failed = True

        def check_stream(kind, actual):
            nonlocal failures, test_failed
            rel = test.get(f"expected_{kind}")
            if rel:
                path = resolve_rel_path(rel)
                if update:
                    path.parent.mkdir(parents=True, exist_ok=True)
                    path.write_text(actual, encoding="utf-8")
                    return
                if not path.exists():
                    print(f"FAIL {test_id} [harness_error]: missing expectation {path}")
                    failures += 1
                    test_failed = True
                    return
                expected = path.read_text(encoding="utf-8")
                if expected != actual:
                    print(f"FAIL {test_id} [runtime_{kind}_mismatch]: mismatch in {path.name}")
                    print(diff_text(expected, actual, path))
                    failures += 1
                    test_failed = True
            else:
                if actual != "":
                    print(f"FAIL {test_id} [runtime_{kind}_mismatch]: expected empty {kind}")
                    failures += 1
                    test_failed = True

        check_stream("stdout", run_result["stdout"])
        check_stream("stderr", run_result["stderr"])

        expected_files = test.get("expected_files", [])
        if expected_files:
            for file_spec in expected_files:
                if not isinstance(file_spec, dict) or "path" not in file_spec:
                    print(f"FAIL {test_id} [harness_error]: expected_files entries require 'path'")
                    failures += 1
                    test_failed = True
                    continue
                rel_path = str(file_spec["path"])
                full_path = run_dir / rel_path
                if not full_path.exists():
                    print(
                        f"FAIL {test_id} [harness_error]: expected runtime file missing: {full_path}"
                    )
                    failures += 1
                    test_failed = True
                    continue
                if "content" in file_spec:
                    actual_content = full_path.read_text(encoding="utf-8")
                    expected_content = str(file_spec["content"])
                    if actual_content != expected_content:
                        print(
                            f"FAIL {test_id} [harness_error]: runtime file content mismatch for {rel_path}"
                        )
                        print(diff_text(expected_content, actual_content, full_path))
                        failures += 1
                        test_failed = True

        differential_with = str(test.get("differential_with", "")).strip()
        if not test_failed and differential_with:
            if differential_with != "clang":
                print(
                    f"FAIL {test_id} [harness_error]: unsupported differential_with='{differential_with}'"
                )
                failures += 1
                test_failed = True
            else:
                clang_bin = str(test.get("differential_compiler", "clang")).strip() or "clang"
                clang_path = shutil.which(clang_bin)
                if not clang_path:
                    print(
                        f"SKIP {test_id}: missing tools: {clang_bin}"
                    )
                    skipped += 1
                    continue

                clang_args = [str(a) for a in test.get("clang_args", args)]
                clang_env_overrides = test.get("clang_env", {})
                clang_run_env_overrides = test.get("clang_run_env", run_env_overrides)
                clang_compile_timeout = int(
                    test.get(
                        "clang_compile_timeout_sec",
                        test.get("differential_compile_timeout_sec", compile_timeout),
                    )
                )
                clang_run_timeout = int(
                    test.get(
                        "clang_run_timeout_sec",
                        test.get("differential_run_timeout_sec", run_timeout),
                    )
                )

                clang_output = run_dir / "clang.out"
                clang_compile_cmd = (
                    [clang_path]
                    + clang_args
                    + [str(p) for p in input_paths]
                    + pkg_config_flags
                    + ["-o", str(clang_output)]
                )
                clang_compile = run_compile(
                    clang_compile_cmd,
                    cwd=run_dir,
                    env=build_env(clang_env_overrides),
                    timeout_sec=clang_compile_timeout,
                    profile_name=profile_name,
                )

                if clang_compile["timed_out"]:
                    print(
                        f"FAIL {test_id} [differential_compile_timeout]: "
                        f"clang timed out after {clang_compile_timeout}s"
                    )
                    failures += 1
                    test_failed = True
                elif clang_compile["exit_code"] != 0:
                    print(
                        f"FAIL {test_id} [differential_compile_fail]: "
                        f"clang exited {clang_compile['exit_code']}"
                    )
                    if clang_compile["output"]:
                        print(clang_compile["output"])
                    failures += 1
                    test_failed = True
                elif not clang_output.exists():
                    print(
                        f"FAIL {test_id} [differential_compile_fail]: clang output missing: {clang_output}"
                    )
                    failures += 1
                    test_failed = True
                else:
                    clang_run = run_program(
                        [str(clang_output)] + [str(a) for a in test.get("run_args", [])],
                        cwd=run_dir,
                        env=build_env(clang_run_env_overrides),
                        stdin_text=test.get("run_stdin"),
                        timeout_sec=clang_run_timeout,
                        profile_name=profile_name,
                    )
                    if clang_run["timed_out"]:
                        print(
                            f"FAIL {test_id} [differential_runtime_timeout]: "
                            f"clang binary timed out after {clang_run_timeout}s"
                        )
                        failures += 1
                        test_failed = True
                    else:
                        if run_result["exit_code"] != clang_run["exit_code"]:
                            print(
                                f"FAIL {test_id} [differential_mismatch]: "
                                f"exit mismatch fisics={run_result['exit_code']} clang={clang_run['exit_code']}"
                            )
                            failures += 1
                            test_failed = True
                        if run_result["stdout"] != clang_run["stdout"]:
                            print(
                                f"FAIL {test_id} [differential_mismatch]: stdout mismatch vs clang"
                            )
                            print(diff_text(clang_run["stdout"], run_result["stdout"], "clang.stdout"))
                            failures += 1
                            test_failed = True
                        if run_result["stderr"] != clang_run["stderr"]:
                            print(
                                f"FAIL {test_id} [differential_mismatch]: stderr mismatch vs clang"
                            )
                            print(diff_text(clang_run["stderr"], run_result["stderr"], "clang.stderr"))
                            failures += 1
                            test_failed = True

        if not test_failed:
            print(f"PASS {test_id}")

    if failures:
        print(f"\n{failures} failing, {skipped} skipped")
        return 1
    if has_selector and selected == 0:
        print("FAIL: selector matched 0 binary tests")
        return 1
    if skipped:
        print(f"\n0 failing, {skipped} skipped")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
