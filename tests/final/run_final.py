#!/usr/bin/env python3
import difflib
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
            test_id = test.get("id")
            if test_id in seen_ids:
                raise ValueError(f"duplicate final test id '{test_id}' in {manifest_path}")
            seen_ids.add(test_id)
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


def main():
    bin_path = sys.argv[1] if len(sys.argv) > 1 else "./fisics"
    update = os.environ.get("UPDATE_FINAL", "0") == "1" or "--update" in sys.argv
    filt = os.environ.get("FINAL_FILTER", "")
    enable_token_dump = os.environ.get("ENABLE_TOKEN_DUMP", "1") == "1"

    meta = load_meta()
    tests = meta.get("tests", [])

    failures = 0
    skipped = 0

    for test in tests:
        test_id = test["id"]
        if filt and test_id != filt:
            continue

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

        if differential and not run_enabled:
            print(f"FAIL {test_id}: differential=true requires run=true")
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
        cmd_env = os.environ.copy()
        for key, value in test.get("env", {}).items():
            cmd_env[str(key)] = str(value)

        runtime_tmp = None
        runtime_exec_path = None
        diag_json_tmp = None
        diag_json_path = None
        diag_json_raw = None
        if run_enabled:
            if has_tokens:
                print(f"FAIL {test_id}: run=true tests do not support token expectations")
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
                    print(f"FAIL {test_id}: compiler exited {exit_code}")
                    print(output)
                    failures += 1
                    continue
            else:
                if has_ast and exit_code != 0:
                    print(f"FAIL {test_id}: compiler exited {exit_code}")
                    print(output)
                    failures += 1
                    continue
                if not has_ast and not only_diag and exit_code != 0 and not allow_nonzero_exit:
                    print(f"FAIL {test_id}: compiler exited {exit_code}")
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
                        print(f"FAIL {test_id}: missing diagnostics JSON export")
                        print(output)
                        failures += 1
                        test_failed = True
                        continue
                    try:
                        actual = normalize_diag_json_text(diag_json_raw)
                    except json.JSONDecodeError as exc:
                        print(f"FAIL {test_id}: invalid diagnostics JSON ({exc})")
                        print(diag_json_raw)
                        failures += 1
                        test_failed = True
                        continue
                elif ext == ".pdiag":
                    if diag_json_raw is None:
                        print(f"FAIL {test_id}: missing diagnostics JSON export")
                        print(output)
                        failures += 1
                        test_failed = True
                        continue
                    try:
                        actual = render_parser_diag_text(diag_json_raw)
                    except json.JSONDecodeError as exc:
                        print(f"FAIL {test_id}: invalid diagnostics JSON ({exc})")
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
                    print(f"FAIL {test_id}: missing expectation {expect_path}")
                    failures += 1
                    test_failed = True
                    continue

                expected = expect_path.read_text(encoding="utf-8")
                if expected != actual:
                    print(f"FAIL {test_id}: mismatch in {expect_path.name}")
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
                    print(f"FAIL {test_id}: expect_exit must be an integer")
                    failures += 1
                    test_failed = True
                    expect_exit = None

                if expect_exit is not None and run_exit != expect_exit:
                    print(f"FAIL {test_id}: runtime exit mismatch (expected {expect_exit}, got {run_exit})")
                    failures += 1
                    test_failed = True

                if expected_stdout_path:
                    if update:
                        expected_stdout_path.parent.mkdir(parents=True, exist_ok=True)
                        expected_stdout_path.write_text(run_stdout, encoding="utf-8")
                    else:
                        if not expected_stdout_path.exists():
                            print(f"FAIL {test_id}: missing expectation {expected_stdout_path}")
                            failures += 1
                            test_failed = True
                        else:
                            expected_stdout = expected_stdout_path.read_text(encoding="utf-8")
                            if expected_stdout != run_stdout:
                                print(f"FAIL {test_id}: mismatch in {expected_stdout_path.name}")
                                print(diff_text(expected_stdout, run_stdout, expected_stdout_path))
                                failures += 1
                                test_failed = True

                if expected_stderr_path:
                    if update:
                        expected_stderr_path.parent.mkdir(parents=True, exist_ok=True)
                        expected_stderr_path.write_text(run_stderr, encoding="utf-8")
                    else:
                        if not expected_stderr_path.exists():
                            print(f"FAIL {test_id}: missing expectation {expected_stderr_path}")
                            failures += 1
                            test_failed = True
                        else:
                            expected_stderr = expected_stderr_path.read_text(encoding="utf-8")
                            if expected_stderr != run_stderr:
                                print(f"FAIL {test_id}: mismatch in {expected_stderr_path.name}")
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
                        clang = shutil.which(test.get("differential_compiler", "clang"))
                        if not clang:
                            print(f"SKIP {test_id}: differential requested but clang not found")
                            skipped += 1
                        else:
                            with tempfile.TemporaryDirectory(prefix=f"final-diff-{test_id}-") as clang_tmp:
                                clang_exec = Path(clang_tmp) / "clang.out"
                                clang_args = [str(a) for a in test.get("clang_args", [])]
                                clang_cmd = [
                                    clang,
                                    f"-std={standard}",
                                    "-O0",
                                ] + clang_args + [str(p) for p in input_paths] + ["-o", str(clang_exec)]
                                clang_compile_exit, clang_compile_output = run_cmd(clang_cmd)
                                if clang_compile_exit != 0:
                                    print(f"FAIL {test_id}: clang compile failed ({clang_compile_exit})")
                                    print(clang_compile_output)
                                    failures += 1
                                    test_failed = True
                                else:
                                    clang_exit, clang_stdout, clang_stderr = run_program(
                                        [str(clang_exec)] + run_args,
                                        env=runtime_env,
                                        stdin_text=run_stdin,
                                    )
                                    if clang_exit != run_exit:
                                        print(
                                            f"FAIL {test_id}: differential exit mismatch "
                                            f"(fisics={run_exit}, clang={clang_exit})"
                                        )
                                        failures += 1
                                        test_failed = True
                                    if clang_stdout != run_stdout:
                                        print(f"FAIL {test_id}: differential stdout mismatch")
                                        print(
                                            diff_text(
                                                clang_stdout,
                                                run_stdout,
                                                Path(f"{test_id}.stdout"),
                                            )
                                        )
                                        failures += 1
                                        test_failed = True
                                    if clang_stderr != run_stderr:
                                        print(f"FAIL {test_id}: differential stderr mismatch")
                                        print(
                                            diff_text(
                                                clang_stderr,
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
    if skipped:
        print(f"\n0 failing, {skipped} skipped")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
