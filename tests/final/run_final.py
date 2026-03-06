#!/usr/bin/env python3
import difflib
import json
import os
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
        only_diag = all(p.suffix == ".diag" for p in expects)
        has_tokens = any(p.suffix == ".tokens" for p in expects)
        has_ir = any(p.suffix == ".ir" for p in expects)
        capture_frontend_diag = test.get("capture_frontend_diag", False)
        allow_nonzero_exit = test.get("allow_nonzero_exit", False)

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

        exit_code, output = run_cmd(cmd, env=cmd_env)

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

        if not test_failed and not update:
            print(f"PASS {test_id}")

    if failures:
        print(f"\n{failures} failing, {skipped} skipped")
        return 1
    if skipped:
        print(f"\n0 failing, {skipped} skipped")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
