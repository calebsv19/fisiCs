#!/usr/bin/env python3
import difflib
import fnmatch
import json
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent
META_DIR = ROOT / "meta"
META_INDEX_PATH = META_DIR / "index.json"


def configure_stdio():
    for stream_name in ("stdout", "stderr"):
        stream = getattr(sys, stream_name, None)
        reconfigure = getattr(stream, "reconfigure", None)
        if callable(reconfigure):
            reconfigure(line_buffering=True, write_through=True)


def extract_sections(text, capture_frontend_diag=False):
    lines = text.splitlines()
    section = None
    ast_lines = []
    diag_lines = []
    token_lines = []
    ir_lines = []

    def append_diag(line):
        if not diag_lines:
            diag_lines.append("Diagnostics:")
        diag_lines.append(line)

    for line in lines:
        stripped = line.lstrip()
        if (
            capture_frontend_diag
            and section != "diag"
            and (
                stripped.startswith("Error")
                or stripped.startswith("Warning")
                or ": error:" in stripped
                or ": warning:" in stripped
            )
        ):
            append_diag(line)
            continue
        if stripped.startswith("Token Stream:"):
            section = "tokens"
            token_lines.append("Tokens:")
            continue
        if stripped.startswith("AST Output:"):
            section = "ast"
            ast_lines.append("AST:")
            continue
        if stripped.startswith("Semantic Analysis:"):
            section = "diag"
            if not diag_lines:
                diag_lines.append("Diagnostics:")
            continue
        if stripped.startswith("Semantic Model Dump:"):
            section = None
            continue
        if "LLVM Code Generation:" in line:
            section = "ir"
            ir_lines.append("IR:")
            continue
        if section == "tokens":
            if stripped == "":
                section = None
                continue
            token_lines.append(line)
        elif section == "ast":
            ast_lines.append(line)
        elif section == "diag":
            if (
                capture_frontend_diag
                and stripped == "Semantic analysis: no issues found."
                and len(diag_lines) > 1
            ):
                continue
            diag_lines.append(line)
        elif section == "ir":
            ir_lines.append(line)

    ast_text = "\n".join(ast_lines).rstrip()
    diag_text = "\n".join(diag_lines).rstrip()
    token_text = "\n".join(token_lines).rstrip()
    ir_text = "\n".join(ir_lines).rstrip()
    if ast_text:
        ast_text += "\n"
    if diag_text:
        diag_text += "\n"
    if token_text:
        token_text += "\n"
    if ir_text:
        ir_text += "\n"
    return ast_text, diag_text, token_text, ir_text


def run_cmd(cmd, env=None):
    proc = subprocess.run(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        encoding="utf-8",
        errors="replace",
        env=env,
    )
    return proc.returncode, proc.stdout


def run_program(cmd, env=None, stdin_text=None):
    proc = subprocess.run(
        cmd,
        input=stdin_text,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        encoding="utf-8",
        errors="replace",
        env=env,
    )
    return proc.returncode, proc.stdout, proc.stderr


def load_json(path):
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def iter_manifest_files():
    if META_INDEX_PATH.exists():
        meta_index = load_json(META_INDEX_PATH)
        if isinstance(meta_index, dict):
            manifests = meta_index.get("manifests")
            if manifests:
                for rel_path in manifests:
                    yield META_DIR / rel_path
                return
            if "tests" in meta_index:
                yield META_INDEX_PATH
                return

    for path in sorted(META_DIR.glob("*.json")):
        if path.name == "feature_map.json":
            continue
        data = load_json(path)
        if isinstance(data, dict) and "tests" in data:
            yield path


def load_meta():
    merged = {
        "version": 1,
        "suite": "final",
        "tests": [],
    }
    seen_ids = set()

    for manifest_path in iter_manifest_files():
        manifest = load_json(manifest_path)
        tests = manifest.get("tests", [])
        for test in tests:
            test = dict(test)
            test_id = test.get("id")
            if test_id in seen_ids:
                raise ValueError(f"duplicate final test id '{test_id}' in {manifest_path}")
            seen_ids.add(test_id)
            test["__manifest"] = manifest_path.name
            merged["tests"].append(test)

    return merged


def diff_text(expected, actual, path):
    exp_lines = expected.splitlines(keepends=True)
    act_lines = actual.splitlines(keepends=True)
    return "".join(difflib.unified_diff(exp_lines, act_lines, fromfile=str(path), tofile="actual"))


def normalize_diag_json_text(raw_text):
    data = json.loads(raw_text)
    return json.dumps(data, indent=2, sort_keys=True) + "\n"


def render_parser_diag_text(raw_text):
    data = json.loads(raw_text)
    diagnostics = data.get("diagnostics", [])
    lines = ["ParserDiagnostics:"]
    for item in diagnostics:
        code = int(item.get("code", 0))
        if code < 1000 or code >= 2000:
            continue
        line = int(item.get("line", 0))
        column = int(item.get("column", 0))
        length = int(item.get("length", 0))
        kind = int(item.get("kind", 0))
        lines.append(
            f"code={code} line={line} column={column} length={length} kind={kind}"
        )
    if len(lines) == 1:
        lines.append("<none>")
    return "\n".join(lines) + "\n"


def parse_csv_env(name):
    raw = os.environ.get(name, "").strip()
    if not raw:
        return []
    return [value.strip() for value in raw.split(",") if value.strip()]


def classify_final_trust_layer(test_bucket, input_count, run_enabled, differential):
    if run_enabled or differential or test_bucket == "runtime-surface":
        return "Layer E"
    if input_count > 1 or test_bucket == "scopes-linkage":
        return "Layer D"
    return "Layer B"


def emit_final_failure(
    message,
    *,
    failure_kind,
    severity,
    test_id=None,
    test_bucket="",
    input_count=1,
    run_enabled=False,
    differential=False,
):
    trust_layer = classify_final_trust_layer(
        test_bucket, input_count, run_enabled, differential
    )
    owner_lane = test_bucket or "final-harness"
    prefix = "FAIL"
    if test_id:
        prefix += f" {test_id}"
    print(
        f"{prefix} [failure_kind={failure_kind} severity={severity} "
        f"source_lane=final trust_layer={trust_layer} owner_lane={owner_lane}]: "
        f"{message}"
    )


def main():
    configure_stdio()
    bin_path = sys.argv[1] if len(sys.argv) > 1 else "./fisics"
    update = os.environ.get("UPDATE_FINAL", "0") == "1" or "--update" in sys.argv
    filt = os.environ.get("FINAL_FILTER", "").strip()
    prefix_filters = parse_csv_env("FINAL_PREFIX")
    glob_filters = parse_csv_env("FINAL_GLOB")
    bucket_filters = parse_csv_env("FINAL_BUCKET")
    tag_filters = parse_csv_env("FINAL_TAG")
    manifest_filters = parse_csv_env("FINAL_MANIFEST")
    manifest_glob_filters = parse_csv_env("FINAL_MANIFEST_GLOB")
    has_selector = any(
        [
            filt,
            prefix_filters,
            glob_filters,
            bucket_filters,
            tag_filters,
            manifest_filters,
            manifest_glob_filters,
        ]
    )
    enable_token_dump = os.environ.get("ENABLE_TOKEN_DUMP", "1") == "1"

    meta = load_meta()
    tests = meta.get("tests", [])

    failures = 0
    skipped = 0
    selected = 0

    for test in tests:
        test_id = test["id"]
        test_manifest = str(test.get("__manifest", ""))
        test_bucket = str(test.get("bucket", ""))
        tags = {str(tag) for tag in test.get("tags", [])}

        if filt and test_id != filt:
            continue
        if prefix_filters and not any(test_id.startswith(prefix) for prefix in prefix_filters):
            continue
        if glob_filters and not any(fnmatch.fnmatch(test_id, pattern) for pattern in glob_filters):
            continue
        if bucket_filters and test_bucket not in bucket_filters:
            continue
        if tag_filters and not all(tag in tags for tag in tag_filters):
            continue
        if manifest_filters and not any(
            test_manifest == token or test_manifest.endswith(token) or token in test_manifest
            for token in manifest_filters
        ):
            continue
        if manifest_glob_filters and not any(
            fnmatch.fnmatch(test_manifest, pattern) for pattern in manifest_glob_filters
        ):
            continue

        selected += 1

        requires = set(test.get("requires", []))
        if "token-dump" in requires and not enable_token_dump:
            print(f"SKIP {test_id}: requires token-dump")
            skipped += 1
            continue

        inputs = test.get("inputs")
        if inputs:
            input_paths = [ROOT / p for p in inputs]
        else:
            input_paths = [ROOT / test["input"]]
        expects = [ROOT / p for p in test.get("expects", [])]

        has_ast = any(p.suffix == ".ast" for p in expects)
        has_diag_json = any(p.suffix == ".diagjson" for p in expects)
        has_parser_diag = any(p.suffix == ".pdiag" for p in expects)
        only_diag = all(p.suffix in (".diag", ".diagjson", ".pdiag") for p in expects)
        has_tokens = any(p.suffix == ".tokens" for p in expects)
        has_ir = any(p.suffix == ".ir" for p in expects)
        capture_frontend_diag = test.get("capture_frontend_diag", False)
        allow_nonzero_exit = test.get("allow_nonzero_exit", False)
        run_enabled = bool(test.get("run", False))
        differential = bool(test.get("differential", False))
        ub = bool(test.get("ub", False))
        impl_defined = bool(test.get("impl_defined", False))
        standard = str(test.get("standard", "c99"))

        expected_stdout_rel = test.get("expected_stdout")
        expected_stderr_rel = test.get("expected_stderr")
        expected_stdout_path = ROOT / expected_stdout_rel if expected_stdout_rel else None
        expected_stderr_path = ROOT / expected_stderr_rel if expected_stderr_rel else None
        run_args = [str(a) for a in test.get("run_args", [])]
        run_stdin = test.get("run_stdin")
        run_env_overrides = {str(k): str(v) for k, v in test.get("run_env", {}).items()}
        input_count = len(input_paths)

        if differential and not run_enabled:
            emit_final_failure(
                "differential=true requires run=true",
                failure_kind="harness_error",
                severity="medium",
                test_id=test_id,
                test_bucket=test_bucket,
                input_count=input_count,
                run_enabled=run_enabled,
                differential=differential,
            )
            failures += 1
            continue

        extra_args = test.get("args", [])
        cmd = [bin_path] + [str(a) for a in extra_args] + [str(p) for p in input_paths]
        if has_tokens:
            if not enable_token_dump:
                print(f"SKIP {test_id}: requires token-dump")
                skipped += 1
                continue
            cmd.append("--dump-tokens")
        if has_ir:
            cmd.append("--dump-ir")
        frontend_only_diag = only_diag and input_count == 1 and not run_enabled and not has_ir
        cmd_env = os.environ.copy()
        for key, value in test.get("env", {}).items():
            cmd_env[str(key)] = str(value)
        if frontend_only_diag:
            cmd_env["DISABLE_CODEGEN"] = "1"

        runtime_tmp = None
        runtime_exec_path = None
        diag_json_tmp = None
        diag_json_path = None
        diag_json_raw = None
        if run_enabled:
            if has_tokens:
                emit_final_failure(
                    "run=true tests do not support token expectations",
                    failure_kind="harness_error",
                    severity="medium",
                    test_id=test_id,
                    test_bucket=test_bucket,
                    input_count=input_count,
                    run_enabled=run_enabled,
                    differential=differential,
                )
                failures += 1
                continue
            runtime_tmp = tempfile.TemporaryDirectory(prefix=f"final-{test_id}-")
            runtime_exec_path = Path(runtime_tmp.name) / "a.out"
            cmd = cmd + ["-o", str(runtime_exec_path)]
        if has_diag_json or has_parser_diag:
            diag_json_tmp = tempfile.TemporaryDirectory(prefix=f"final-diag-{test_id}-")
            diag_json_path = Path(diag_json_tmp.name) / "diagnostics.json"
            cmd = cmd + ["--emit-diags-json", str(diag_json_path)]

        try:
            exit_code, output = run_cmd(cmd, env=cmd_env)

            if run_enabled:
                if exit_code != 0:
                    emit_final_failure(
                        f"compiler exited {exit_code}",
                        failure_kind="ir_or_codegen_fail",
                        severity="high",
                        test_id=test_id,
                        test_bucket=test_bucket,
                        input_count=input_count,
                        run_enabled=run_enabled,
                        differential=differential,
                    )
                    print(output)
                    failures += 1
                    continue
            else:
                if has_ast and exit_code != 0:
                    emit_final_failure(
                        f"compiler exited {exit_code}",
                        failure_kind="parser_fail",
                        severity="high",
                        test_id=test_id,
                        test_bucket=test_bucket,
                        input_count=input_count,
                        run_enabled=run_enabled,
                        differential=differential,
                    )
                    print(output)
                    failures += 1
                    continue
                if not has_ast and not only_diag and exit_code != 0 and not allow_nonzero_exit:
                    emit_final_failure(
                        f"compiler exited {exit_code}",
                        failure_kind="ir_or_codegen_fail",
                        severity="high",
                        test_id=test_id,
                        test_bucket=test_bucket,
                        input_count=input_count,
                        run_enabled=run_enabled,
                        differential=differential,
                    )
                    print(output)
                    failures += 1
                    continue

            ast_text, diag_text, token_text, ir_text = extract_sections(
                output, capture_frontend_diag=capture_frontend_diag
            )
            if diag_json_path is not None and diag_json_path.exists():
                diag_json_raw = diag_json_path.read_text(encoding="utf-8")

            test_failed = False
            for expect_path in expects:
                ext = expect_path.suffix
                if ext == ".ast":
                    actual = ast_text
                elif ext == ".diag":
                    actual = diag_text
                elif ext == ".tokens":
                    actual = token_text
                elif ext == ".ir":
                    actual = ir_text
                elif ext == ".diagjson":
                    if diag_json_raw is None:
                        emit_final_failure(
                            "missing diagnostics JSON export",
                            failure_kind="harness_error",
                            severity="medium",
                            test_id=test_id,
                            test_bucket=test_bucket,
                            input_count=input_count,
                            run_enabled=run_enabled,
                            differential=differential,
                        )
                        print(output)
                        failures += 1
                        test_failed = True
                        continue
                    try:
                        actual = normalize_diag_json_text(diag_json_raw)
                    except json.JSONDecodeError as exc:
                        emit_final_failure(
                            f"invalid diagnostics JSON ({exc})",
                            failure_kind="harness_error",
                            severity="medium",
                            test_id=test_id,
                            test_bucket=test_bucket,
                            input_count=input_count,
                            run_enabled=run_enabled,
                            differential=differential,
                        )
                        print(diag_json_raw)
                        failures += 1
                        test_failed = True
                        continue
                elif ext == ".pdiag":
                    if diag_json_raw is None:
                        emit_final_failure(
                            "missing diagnostics JSON export",
                            failure_kind="harness_error",
                            severity="medium",
                            test_id=test_id,
                            test_bucket=test_bucket,
                            input_count=input_count,
                            run_enabled=run_enabled,
                            differential=differential,
                        )
                        print(output)
                        failures += 1
                        test_failed = True
                        continue
                    try:
                        actual = render_parser_diag_text(diag_json_raw)
                    except json.JSONDecodeError as exc:
                        emit_final_failure(
                            f"invalid diagnostics JSON ({exc})",
                            failure_kind="harness_error",
                            severity="medium",
                            test_id=test_id,
                            test_bucket=test_bucket,
                            input_count=input_count,
                            run_enabled=run_enabled,
                            differential=differential,
                        )
                        print(diag_json_raw)
                        failures += 1
                        test_failed = True
                        continue
                else:
                    print(f"SKIP {test_id}: unsupported expectation {expect_path.name}")
                    skipped += 1
                    continue

                if update:
                    expect_path.parent.mkdir(parents=True, exist_ok=True)
                    with open(expect_path, "w", encoding="utf-8") as f:
                        f.write(actual)
                    continue

                if not expect_path.exists():
                    emit_final_failure(
                        f"missing expectation {expect_path}",
                        failure_kind="harness_error",
                        severity="medium",
                        test_id=test_id,
                        test_bucket=test_bucket,
                        input_count=input_count,
                        run_enabled=run_enabled,
                        differential=differential,
                    )
                    failures += 1
                    test_failed = True
                    continue

                expected = expect_path.read_text(encoding="utf-8")
                if expected != actual:
                    failure_kind = "ir_or_codegen_fail"
                    severity = "high"
                    if ext in (".ast", ".tokens"):
                        failure_kind = "parser_fail"
                    elif ext in (".diag", ".diagjson", ".pdiag"):
                        failure_kind = "wrong_diagnostics"
                        severity = "medium"
                    emit_final_failure(
                        f"mismatch in {expect_path.name}",
                        failure_kind=failure_kind,
                        severity=severity,
                        test_id=test_id,
                        test_bucket=test_bucket,
                        input_count=input_count,
                        run_enabled=run_enabled,
                        differential=differential,
                    )
                    print(diff_text(expected, actual, expect_path))
                    failures += 1
                    test_failed = True

            if run_enabled and not test_failed:
                run_cmdline = [str(runtime_exec_path)] + run_args
                runtime_env = os.environ.copy()
                runtime_env.update(run_env_overrides)
                run_exit, run_stdout, run_stderr = run_program(
                    run_cmdline,
                    env=runtime_env,
                    stdin_text=run_stdin,
                )

                expect_exit = test.get("expect_exit", 0)
                try:
                    expect_exit = int(expect_exit)
                except (TypeError, ValueError):
                    emit_final_failure(
                        "expect_exit must be an integer",
                        failure_kind="harness_error",
                        severity="medium",
                        test_id=test_id,
                        test_bucket=test_bucket,
                        input_count=input_count,
                        run_enabled=run_enabled,
                        differential=differential,
                    )
                    failures += 1
                    test_failed = True
                    expect_exit = None

                if expect_exit is not None and run_exit != expect_exit:
                    emit_final_failure(
                        f"runtime exit mismatch (expected {expect_exit}, got {run_exit})",
                        failure_kind="runtime_mismatch",
                        severity="critical",
                        test_id=test_id,
                        test_bucket=test_bucket,
                        input_count=input_count,
                        run_enabled=run_enabled,
                        differential=differential,
                    )
                    failures += 1
                    test_failed = True

                if expected_stdout_path:
                    if update:
                        expected_stdout_path.parent.mkdir(parents=True, exist_ok=True)
                        expected_stdout_path.write_text(run_stdout, encoding="utf-8")
                    else:
                        if not expected_stdout_path.exists():
                            emit_final_failure(
                                f"missing expectation {expected_stdout_path}",
                                failure_kind="harness_error",
                                severity="medium",
                                test_id=test_id,
                                test_bucket=test_bucket,
                                input_count=input_count,
                                run_enabled=run_enabled,
                                differential=differential,
                            )
                            failures += 1
                            test_failed = True
                        else:
                            expected_stdout = expected_stdout_path.read_text(encoding="utf-8")
                            if expected_stdout != run_stdout:
                                emit_final_failure(
                                    f"mismatch in {expected_stdout_path.name}",
                                    failure_kind="runtime_mismatch",
                                    severity="critical",
                                    test_id=test_id,
                                    test_bucket=test_bucket,
                                    input_count=input_count,
                                    run_enabled=run_enabled,
                                    differential=differential,
                                )
                                print(diff_text(expected_stdout, run_stdout, expected_stdout_path))
                                failures += 1
                                test_failed = True

                if expected_stderr_path:
                    if update:
                        expected_stderr_path.parent.mkdir(parents=True, exist_ok=True)
                        expected_stderr_path.write_text(run_stderr, encoding="utf-8")
                    else:
                        if not expected_stderr_path.exists():
                            emit_final_failure(
                                f"missing expectation {expected_stderr_path}",
                                failure_kind="harness_error",
                                severity="medium",
                                test_id=test_id,
                                test_bucket=test_bucket,
                                input_count=input_count,
                                run_enabled=run_enabled,
                                differential=differential,
                            )
                            failures += 1
                            test_failed = True
                        else:
                            expected_stderr = expected_stderr_path.read_text(encoding="utf-8")
                            if expected_stderr != run_stderr:
                                emit_final_failure(
                                    f"mismatch in {expected_stderr_path.name}",
                                    failure_kind="runtime_mismatch",
                                    severity="critical",
                                    test_id=test_id,
                                    test_bucket=test_bucket,
                                    input_count=input_count,
                                    run_enabled=run_enabled,
                                    differential=differential,
                                )
                                print(diff_text(expected_stderr, run_stderr, expected_stderr_path))
                                failures += 1
                                test_failed = True

                if differential:
                    if ub:
                        print(f"SKIP {test_id}: differential disabled for ub=true")
                        skipped += 1
                    elif impl_defined:
                        print(f"SKIP {test_id}: differential disabled for impl_defined=true")
                        skipped += 1
                    else:
                        reference_compiler_name = str(test.get("differential_compiler", "clang"))
                        reference_compiler = shutil.which(reference_compiler_name)
                        if not reference_compiler:
                            print(
                                f"SKIP {test_id}: differential requested but {reference_compiler_name} not found"
                            )
                            skipped += 1
                        else:
                            with tempfile.TemporaryDirectory(
                                prefix=f"final-diff-{test_id}-{reference_compiler_name}-"
                            ) as reference_tmp:
                                reference_exec = Path(reference_tmp) / f"{reference_compiler_name}.out"
                                reference_args = [
                                    str(a)
                                    for a in test.get("reference_args", test.get("clang_args", []))
                                ]
                                reference_cmd = [
                                    reference_compiler,
                                    f"-std={standard}",
                                    "-O0",
                                ] + reference_args + [str(p) for p in input_paths] + [
                                    "-o",
                                    str(reference_exec),
                                ]
                                reference_compile_exit, reference_compile_output = run_cmd(reference_cmd)
                                if reference_compile_exit != 0:
                                    emit_final_failure(
                                        f"{reference_compiler_name} compile failed "
                                        f"({reference_compile_exit})",
                                        failure_kind="harness_error",
                                        severity="medium",
                                        test_id=test_id,
                                        test_bucket=test_bucket,
                                        input_count=input_count,
                                        run_enabled=run_enabled,
                                        differential=differential,
                                    )
                                    print(reference_compile_output)
                                    failures += 1
                                    test_failed = True
                                else:
                                    reference_exit, reference_stdout, reference_stderr = run_program(
                                        [str(reference_exec)] + run_args,
                                        env=runtime_env,
                                        stdin_text=run_stdin,
                                    )
                                    if reference_exit != run_exit:
                                        emit_final_failure(
                                            f"differential exit mismatch "
                                            f"(fisics={run_exit}, {reference_compiler_name}={reference_exit})",
                                            failure_kind="runtime_mismatch",
                                            severity="critical",
                                            test_id=test_id,
                                            test_bucket=test_bucket,
                                            input_count=input_count,
                                            run_enabled=run_enabled,
                                            differential=differential,
                                        )
                                        failures += 1
                                        test_failed = True
                                    if reference_stdout != run_stdout:
                                        emit_final_failure(
                                            "differential stdout mismatch",
                                            failure_kind="runtime_mismatch",
                                            severity="critical",
                                            test_id=test_id,
                                            test_bucket=test_bucket,
                                            input_count=input_count,
                                            run_enabled=run_enabled,
                                            differential=differential,
                                        )
                                        print(
                                            diff_text(
                                                reference_stdout,
                                                run_stdout,
                                                Path(f"{test_id}.stdout"),
                                            )
                                        )
                                        failures += 1
                                        test_failed = True
                                    if reference_stderr != run_stderr:
                                        emit_final_failure(
                                            "differential stderr mismatch",
                                            failure_kind="runtime_mismatch",
                                            severity="critical",
                                            test_id=test_id,
                                            test_bucket=test_bucket,
                                            input_count=input_count,
                                            run_enabled=run_enabled,
                                            differential=differential,
                                        )
                                        print(
                                            diff_text(
                                                reference_stderr,
                                                run_stderr,
                                                Path(f"{test_id}.stderr"),
                                            )
                                        )
                                        failures += 1
                                        test_failed = True

            if not test_failed and not update:
                print(f"PASS {test_id}")
        finally:
            if runtime_tmp is not None:
                runtime_tmp.cleanup()
            if diag_json_tmp is not None:
                diag_json_tmp.cleanup()

    if failures:
        print(f"\n{failures} failing, {skipped} skipped")
        return 1
    if has_selector and selected == 0:
        emit_final_failure(
            "selector matched 0 tests "
            "(FINAL_FILTER / FINAL_PREFIX / FINAL_GLOB / FINAL_BUCKET / "
            "FINAL_TAG / FINAL_MANIFEST / FINAL_MANIFEST_GLOB)",
            failure_kind="harness_error",
            severity="medium",
        )
        return 1
    if skipped:
        print(f"\n0 failing, {skipped} skipped")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
