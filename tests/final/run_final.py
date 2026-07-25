#!/usr/bin/env python3
import difflib
import fnmatch
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

from bin_resolver import stage_bin_copy

ROOT = Path(__file__).resolve().parent
REPO_ROOT = ROOT.parent.parent
META_DIR = ROOT / "meta"
META_INDEX_PATH = META_DIR / "index.json"
SUPPORTED_EXPECTATION_EXTENSIONS = frozenset(
    {".ast", ".diag", ".diagjson", ".ir", ".pdiag", ".sema", ".tokens"}
)
RUNTIME_EXPECTATION_FIELDS = {
    "expected_stdout": ".stdout",
    "expected_stderr": ".stderr",
}
SUPPORTED_TEST_FIELDS = frozenset(
    {
        "__manifest",
        "allow_empty_diag",
        "allow_empty_diag_json",
        "allow_nonzero_exit",
        "args",
        "bucket",
        "capture_frontend_diag",
        "clang_args",
        "differential",
        "differential_compiler",
        "env",
        "expect_exit",
        "expected_stderr",
        "expected_stdout",
        "expected_stdout_variants",
        "expects",
        "expect_compile_exit",
        "id",
        "impl_defined",
        "input",
        "inputs",
        "ir_contains",
        "ir_forbids",
        "link",
        "mixed_clang_args",
        "mixed_clang_compiler",
        "mixed_clang_inputs",
        "reference_args",
        "requires",
        "run",
        "run_args",
        "run_env",
        "run_stdin",
        "standard",
        "status",
        "tags",
        "ub",
    }
)
SUPPORTED_REQUIREMENTS = frozenset({"token-dump"})
FINAL_SUITE_ABSOLUTE_PATH_RE = re.compile(
    r"(?:/[^\s:/\"']+)+/tests/final/"
)
FINAL_COMPILE_TIMEOUT_SEC = int(os.environ.get("FISICS_FINAL_COMPILE_TIMEOUT_SEC", "120"))
FINAL_RUNTIME_TIMEOUT_SEC = int(os.environ.get("FISICS_FINAL_RUNTIME_TIMEOUT_SEC", "30"))
FINAL_TIMEOUT_EXIT = 124
FINAL_TIMEOUT_MARKER = "[fisics-final-harness-timeout]"


def configure_stdio():
    for stream_name in ("stdout", "stderr"):
        stream = getattr(sys, stream_name, None)
        reconfigure = getattr(stream, "reconfigure", None)
        if callable(reconfigure):
            reconfigure(line_buffering=True, write_through=True)


def mixed_object_path(directory, source, ordinal):
    """Return a per-input object path even when source basenames collide."""
    return Path(directory) / f"{ordinal:03d}_{Path(source).stem}.clang.o"


def should_update_expectation(update_ir_only, extension):
    return not update_ir_only or extension == ".ir"


def stage_expectation_update(pending_updates, path, actual, test_id):
    key = path.resolve()
    previous = pending_updates.get(key)
    if previous is not None and previous[0] != actual:
        return previous[1]
    if previous is None:
        pending_updates[key] = (actual, test_id)
    return None


def expected_diag_frontend_capture(expects):
    saw_existing_diag = False
    for path in expects:
        if path.suffix != ".diag" or not path.exists():
            continue
        saw_existing_diag = True
        for line in path.read_text(encoding="utf-8").splitlines():
            if (line.startswith("Error:") or line.startswith("Warning:") or
                    ": error:" in line or ": warning:" in line):
                return True
    return False if saw_existing_diag else None


def expected_compile_exit_from_diagnostics(expects):
    for path in expects:
        if path.suffix not in (".diag", ".diagjson", ".pdiag") or not path.exists():
            continue
        if path.suffix == ".pdiag":
            continue
        text = path.read_text(encoding="utf-8")
        if path.suffix == ".diag":
            if any(line.startswith("Error at") for line in text.splitlines()):
                return 1
        elif path.suffix == ".diagjson":
            try:
                payload = json.loads(text)
            except json.JSONDecodeError:
                continue
            diagnostics = payload.get("diagnostics", [])
            if any(
                item.get("severity_name") == "error"
                and item.get("category_name") != "parser"
                for item in diagnostics
                if isinstance(item, dict)
            ):
                return 1
    return None


def should_capture_frontend_diagnostics(
    explicit_capture,
    allow_empty_diag,
    has_diag_text,
    only_frontend,
    exit_code,
    expected_capture=None,
):
    if explicit_capture:
        return True
    if expected_capture is not None:
        return expected_capture
    return explicit_capture or (
        not allow_empty_diag
        and has_diag_text
        and only_frontend
        and exit_code != 0
    )


def normalize_final_suite_paths(text):
    repo_prefix = str(REPO_ROOT.resolve()) + os.sep
    text = text.replace(repo_prefix, "")
    return FINAL_SUITE_ABSOLUTE_PATH_RE.sub("tests/final/", text)


def is_meaningful_ir_expectation(text):
    lines = [line.strip() for line in text.splitlines() if line.strip()]
    if not lines or lines[0] != "IR:":
        return False
    return any(
        line.startswith("; ModuleID =")
        or line == "Skipping LLVM code generation due to semantic errors."
        for line in lines[1:]
    )


def test_oracle_extensions(test):
    extensions = {
        Path(path).suffix
        for path in test.get("expects", [])
        if isinstance(path, str)
    }
    for field in RUNTIME_EXPECTATION_FIELDS:
        path = test.get(field)
        if isinstance(path, str) and path:
            extensions.add(Path(path).suffix)
    for path in test.get("expected_stdout_variants", []):
        if isinstance(path, str) and path:
            extensions.add(Path(path).suffix)
    return extensions


def is_confined_relative_path(path, resolution_base, boundary):
    if not isinstance(path, str) or not path:
        return False
    relative = Path(path)
    if relative.is_absolute():
        return False
    try:
        (resolution_base / relative).resolve().relative_to(boundary.resolve())
    except ValueError:
        return False
    return True


def validate_test_definition(test):
    errors = []
    unknown_fields = sorted(set(test) - SUPPORTED_TEST_FIELDS)
    if unknown_fields:
        errors.append("unknown manifest field(s): " + ", ".join(unknown_fields))

    if "status" in test and test["status"] != "ok":
        errors.append("registered stable tests must use status=ok")

    requires = test.get("requires", [])
    if not isinstance(requires, list) or any(
        not isinstance(requirement, str) or not requirement
        for requirement in requires
    ):
        errors.append("requires must be a list of non-empty strings")
    else:
        unknown_requirements = sorted(set(requires) - SUPPORTED_REQUIREMENTS)
        if unknown_requirements:
            errors.append(
                "unknown requirement(s): " + ", ".join(unknown_requirements)
            )

    expects = test.get("expects", [])
    if not isinstance(expects, list):
        return ["expects must be a list"]

    unsupported = sorted(
        {
            Path(path).suffix
            for path in expects
            if isinstance(path, str)
            and Path(path).suffix not in SUPPORTED_EXPECTATION_EXTENSIONS
        }
    )
    if any(not isinstance(path, str) or not path for path in expects):
        errors.append("expects entries must be non-empty strings")
    elif any(
        not is_confined_relative_path(path, ROOT, ROOT) for path in expects
    ):
        errors.append("expects entries must remain inside the final suite")
    if unsupported:
        errors.append(
            "unsupported expectation extension(s): " + ", ".join(unsupported)
        )

    stdout_variants = test.get("expected_stdout_variants")
    if stdout_variants is not None:
        if (
            not isinstance(stdout_variants, list)
            or not stdout_variants
            or any(not isinstance(path, str) or not path for path in stdout_variants)
        ):
            errors.append(
                "expected_stdout_variants must be a non-empty list of paths"
            )
        else:
            if any(Path(path).suffix != ".stdout" for path in stdout_variants):
                errors.append(
                    "expected_stdout_variants entries must use the .stdout extension"
                )
            if any(
                not is_confined_relative_path(path, ROOT, ROOT)
                for path in stdout_variants
            ):
                errors.append(
                    "expected_stdout_variants entries must remain inside the final suite"
                )
            if len(stdout_variants) != len(set(stdout_variants)):
                errors.append("expected_stdout_variants entries must be unique")
        if "expected_stdout" in test:
            errors.append(
                "expected_stdout cannot be combined with expected_stdout_variants"
            )

    has_ir_markers = bool(test.get("ir_contains") or test.get("ir_forbids"))
    has_compile_exit_oracle = "expect_compile_exit" in test
    if (
        not expects
        and not has_ir_markers
        and not bool(test.get("run", False))
        and not has_compile_exit_oracle
    ):
        errors.append("test has no file, IR-marker, compile-exit, or runtime oracle")

    run_enabled = bool(test.get("run", False))
    ignored_runtime_fields = [
        field
        for field in (
            "expect_exit",
            "expected_stdout",
            "expected_stdout_variants",
            "expected_stderr",
            "run_args",
            "run_env",
            "run_stdin",
        )
        if field in test and not run_enabled
    ]
    if ignored_runtime_fields:
        errors.append(
            "runtime field(s) require run=true: " + ", ".join(ignored_runtime_fields)
        )
    if test.get("differential", False) and not run_enabled:
        errors.append("differential=true requires run=true")

    link_enabled = bool(test.get("link", False))
    if link_enabled and run_enabled:
        errors.append("link=true cannot be combined with run=true")
    if link_enabled and not has_compile_exit_oracle:
        errors.append("link=true requires expect_compile_exit")
    if has_compile_exit_oracle:
        try:
            int(test["expect_compile_exit"])
        except (TypeError, ValueError):
            errors.append("expect_compile_exit must be an integer")
        if test.get("allow_nonzero_exit", False):
            errors.append(
                "expect_compile_exit cannot be combined with allow_nonzero_exit"
            )

    primary_input = test.get("input")
    if primary_input is not None:
        if not isinstance(primary_input, str) or not primary_input:
            errors.append("input must be a non-empty string")
        elif not is_confined_relative_path(
            primary_input, ROOT, REPO_ROOT
        ):
            errors.append("input must remain inside the repository")

    for field in ("inputs", "mixed_clang_inputs"):
        paths = test.get(field)
        if paths is None:
            continue
        if not isinstance(paths, list) or any(
            not isinstance(path, str) or not path for path in paths
        ):
            errors.append(f"{field} must be a list of non-empty strings")
            continue
        if len(paths) != len(set(paths)):
            errors.append(f"{field} entries must be unique")
        if any(
            not is_confined_relative_path(path, ROOT, REPO_ROOT)
            for path in paths
        ):
            errors.append(f"{field} entries must remain inside the repository")

    inputs = test.get("inputs")
    if (
        isinstance(inputs, list)
        and isinstance(primary_input, str)
        and primary_input
        and primary_input not in inputs
    ):
        errors.append("input must be present in the complete inputs list")

    for field, required_extension in RUNTIME_EXPECTATION_FIELDS.items():
        path = test.get(field)
        if path is None:
            continue
        if not isinstance(path, str) or not path:
            errors.append(f"{field} must be a non-empty string")
        elif Path(path).suffix != required_extension:
            errors.append(f"{field} must use the {required_extension} extension")
        elif not is_confined_relative_path(path, ROOT, ROOT):
            errors.append(f"{field} must remain inside the final suite")

    legacy_json_capture_fields = [
        field
        for field in ("capture_diag_json", "capture_frontend_diag_json")
        if field in test
    ]
    if legacy_json_capture_fields:
        errors.append(
            "obsolete diagnostic JSON capture field(s): "
            + ", ".join(legacy_json_capture_fields)
            + "; use a .diagjson expectation"
        )

    if test.get("allow_empty_diag_json", False) and not any(
        isinstance(path, str) and path.endswith(".diagjson") for path in expects
    ):
        errors.append("allow_empty_diag_json requires a .diagjson expectation")

    return errors


def extract_sections(text, capture_frontend_diag=False):
    lines = text.splitlines()
    section = None
    ast_lines = []
    diag_lines = []
    token_lines = []
    sema_lines = []
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
            section = "sema"
            sema_lines.append("Sema:")
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
        elif section == "sema":
            sema_lines.append(line)
        elif section == "ir":
            ir_lines.append(line)

    ast_text = "\n".join(ast_lines).rstrip()
    diag_text = "\n".join(diag_lines).rstrip()
    token_text = "\n".join(token_lines).rstrip()
    sema_text = "\n".join(sema_lines).rstrip()
    ir_text = "\n".join(ir_lines).rstrip()
    if ast_text:
        ast_text += "\n"
    if diag_text:
        diag_text += "\n"
    if token_text:
        token_text += "\n"
    if sema_text:
        sema_text += "\n"
    if ir_text:
        ir_text += "\n"
    return ast_text, diag_text, token_text, sema_text, ir_text


def timeout_output(exc):
    output = exc.stdout or ""
    if isinstance(output, bytes):
        output = output.decode("utf-8", errors="replace")
    return f"{output}{FINAL_TIMEOUT_MARKER} after {exc.timeout}s\n"


def command_timed_out(output):
    return FINAL_TIMEOUT_MARKER in (output or "")


def run_cmd(cmd, env=None, timeout_sec=FINAL_COMPILE_TIMEOUT_SEC):
    try:
        proc = subprocess.run(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            encoding="utf-8",
            errors="replace",
            env=env,
            timeout=timeout_sec,
        )
    except subprocess.TimeoutExpired as exc:
        return FINAL_TIMEOUT_EXIT, timeout_output(exc)
    return proc.returncode, proc.stdout


def run_program(cmd, env=None, stdin_text=None, timeout_sec=FINAL_RUNTIME_TIMEOUT_SEC):
    try:
        proc = subprocess.run(
            cmd,
            input=stdin_text,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            encoding="utf-8",
            errors="replace",
            env=env,
            timeout=timeout_sec,
        )
    except subprocess.TimeoutExpired as exc:
        return FINAL_TIMEOUT_EXIT, "", timeout_output(exc)
    return proc.returncode, proc.stdout, proc.stderr


def load_json(path):
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def iter_manifest_files():
    explicit_manifests = []
    for token in parse_csv_env("FINAL_MANIFEST"):
        if not token.endswith(".json"):
            continue
        path = META_DIR / token
        if path.exists():
            explicit_manifests.append(path)

    if META_INDEX_PATH.exists():
        meta_index = load_json(META_INDEX_PATH)
        if isinstance(meta_index, dict):
            manifests = meta_index.get("manifests")
            if manifests:
                yielded = set()
                for rel_path in manifests:
                    path = META_DIR / rel_path
                    yielded.add(path.resolve())
                    yield path
                for path in explicit_manifests:
                    if path.resolve() not in yielded:
                        yield path
                return
            if "tests" in meta_index:
                yield META_INDEX_PATH
                for path in explicit_manifests:
                    if path.resolve() != META_INDEX_PATH.resolve():
                        yield path
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
    try:
        staged_bin = stage_bin_copy(
            sys.argv[1] if len(sys.argv) > 1 else "./fisics",
            REPO_ROOT,
            prefix="final-fisics-",
        )
    except FileNotFoundError as exc:
        print(str(exc))
        return 1
    bin_path = str(staged_bin.staged_path)
    memcheck_runtime_lib = REPO_ROOT / "build" / "unsanitized" / "libfisics_memcheck_runtime.a"
    memcheck_runtime_tmp = None
    if memcheck_runtime_lib.exists():
        memcheck_runtime_tmp = tempfile.TemporaryDirectory(prefix="final-memcheck-runtime-")
        staged_memcheck_runtime_lib = (
            Path(memcheck_runtime_tmp.name) / memcheck_runtime_lib.name
        )
        shutil.copy2(memcheck_runtime_lib, staged_memcheck_runtime_lib)
        memcheck_runtime_lib = staged_memcheck_runtime_lib
    update_ir_only = os.environ.get("UPDATE_FINAL_IR_ONLY", "0") == "1"
    update = (
        os.environ.get("UPDATE_FINAL", "0") == "1" or
        update_ir_only or
        "--update" in sys.argv
    )
    filt = os.environ.get("FINAL_FILTER", "").strip()
    prefix_filters = parse_csv_env("FINAL_PREFIX")
    glob_filters = parse_csv_env("FINAL_GLOB")
    bucket_filters = parse_csv_env("FINAL_BUCKET")
    tag_filters = parse_csv_env("FINAL_TAG")
    manifest_filters = parse_csv_env("FINAL_MANIFEST")
    manifest_glob_filters = parse_csv_env("FINAL_MANIFEST_GLOB")
    expectation_extension_filters = {
        token if token.startswith(".") else f".{token}"
        for token in parse_csv_env("FINAL_EXPECT_EXT")
    }
    has_selector = any(
        [
            filt,
            prefix_filters,
            glob_filters,
            bucket_filters,
            tag_filters,
            manifest_filters,
            manifest_glob_filters,
            expectation_extension_filters,
        ]
    )
    enable_token_dump = os.environ.get("ENABLE_TOKEN_DUMP", "1") == "1"

    try:
        meta = load_meta()
        tests = meta.get("tests", [])

        failures = 0
        skipped = 0
        selected = 0
        pending_expectation_updates = {}

        for test in tests:
            test_id = test["id"]
            test_manifest = str(test.get("__manifest", ""))
            test_bucket = str(test.get("bucket", ""))
            tags = {str(tag) for tag in test.get("tags", [])}
            expectation_extensions = test_oracle_extensions(test)

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
            if expectation_extension_filters and not (
                expectation_extensions & expectation_extension_filters
            ):
                continue

            selected += 1

            definition_errors = validate_test_definition(test)
            if definition_errors:
                for definition_error in definition_errors:
                    emit_final_failure(
                        definition_error,
                        failure_kind="harness_error",
                        severity="medium",
                        test_id=test_id,
                        test_bucket=test_bucket,
                        input_count=len(test.get("inputs", [])) or 1,
                        run_enabled=bool(test.get("run", False)),
                        differential=bool(test.get("differential", False)),
                    )
                    failures += 1
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
            mixed_clang_inputs = test.get("mixed_clang_inputs", [])
            mixed_clang_input_paths = [ROOT / p for p in mixed_clang_inputs]
            expects = [ROOT / p for p in test.get("expects", [])]

            has_ast = any(p.suffix == ".ast" for p in expects)
            has_diag_text = any(p.suffix == ".diag" for p in expects)
            has_diag_json = any(p.suffix == ".diagjson" for p in expects)
            has_parser_diag = any(p.suffix == ".pdiag" for p in expects)
            has_sema = any(p.suffix == ".sema" for p in expects)
            only_frontend = all(p.suffix in (".diag", ".diagjson", ".pdiag", ".sema") for p in expects)
            has_tokens = any(p.suffix == ".tokens" for p in expects)
            ir_contains = [str(marker) for marker in test.get("ir_contains", [])]
            ir_forbids = [str(marker) for marker in test.get("ir_forbids", [])]
            has_ir = (
                any(p.suffix == ".ir" for p in expects) or
                bool(ir_contains) or
                bool(ir_forbids)
            )
            capture_frontend_diag = test.get("capture_frontend_diag", False)
            allow_empty_diag = test.get("allow_empty_diag", False)
            allow_nonzero_exit = test.get("allow_nonzero_exit", False)
            run_enabled = bool(test.get("run", False))
            link_enabled = bool(test.get("link", False))
            expected_compile_exit = (
                int(test["expect_compile_exit"])
                if "expect_compile_exit" in test
                else None
            )
            if expected_compile_exit is None and not allow_nonzero_exit:
                expected_compile_exit = expected_compile_exit_from_diagnostics(expects)
            differential = bool(test.get("differential", False))
            ub = bool(test.get("ub", False))
            impl_defined = bool(test.get("impl_defined", False))
            standard = str(test.get("standard", "c99"))

            expected_stdout_rel = test.get("expected_stdout")
            expected_stdout_variant_rels = test.get("expected_stdout_variants", [])
            expected_stderr_rel = test.get("expected_stderr")
            expected_stdout_path = ROOT / expected_stdout_rel if expected_stdout_rel else None
            expected_stdout_variant_paths = [
                ROOT / relative_path for relative_path in expected_stdout_variant_rels
            ]
            expected_stderr_path = ROOT / expected_stderr_rel if expected_stderr_rel else None
            run_args = [str(a) for a in test.get("run_args", [])]
            run_stdin = test.get("run_stdin")
            run_env_overrides = {str(k): str(v) for k, v in test.get("run_env", {}).items()}
            input_count = len(input_paths) + len(mixed_clang_input_paths)

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
            cmd = [bin_path, f"-std={standard}"] + [str(a) for a in extra_args] + [
                str(p) for p in input_paths
            ]
            if has_tokens:
                if not enable_token_dump:
                    print(f"SKIP {test_id}: requires token-dump")
                    skipped += 1
                    continue
                cmd.append("--dump-tokens")
            if has_sema:
                cmd.append("--dump-sema")
            if has_ir:
                cmd.append("--dump-ir")
            frontend_only_diag = (
                only_frontend
                and input_count == 1
                and not run_enabled
                and not link_enabled
                and not has_ir
            )
            cmd_env = os.environ.copy()
            # The final suite spawns the compiler many times from one parent process.
            # Default the process guard off here unless a caller explicitly overrides it.
            cmd_env.setdefault("FISICS_MAX_PROCS", "0")
            cmd_env.setdefault("FISICS_MEMCHECK_RUNTIME_LIB", str(memcheck_runtime_lib))
            for key, value in test.get("env", {}).items():
                cmd_env[str(key)] = str(value)
            if frontend_only_diag:
                cmd_env["DISABLE_CODEGEN"] = "1"

            mixed_clang_tmp = None
            mixed_clang_object_paths = []
            runtime_tmp = None
            runtime_exec_path = None
            link_tmp = None
            diag_json_tmp = None
            diag_json_path = None
            diag_json_raw = None
            mixed_clang_failed = False
            if mixed_clang_input_paths:
                if not run_enabled:
                    emit_final_failure(
                        "mixed_clang_inputs requires run=true",
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
                mixed_compiler_name = str(
                    test.get("mixed_clang_compiler", test.get("differential_compiler", "clang"))
                )
                mixed_compiler = shutil.which(mixed_compiler_name)
                if not mixed_compiler:
                    print(f"SKIP {test_id}: mixed_clang_inputs requested but {mixed_compiler_name} not found")
                    skipped += 1
                    continue
                mixed_clang_tmp = tempfile.TemporaryDirectory(prefix=f"final-mixed-{test_id}-")
                mixed_clang_args = [
                    str(a)
                    for a in test.get("mixed_clang_args", test.get("reference_args", test.get("clang_args", [])))
                ]
                for mixed_index, mixed_input in enumerate(mixed_clang_input_paths):
                    mixed_object = mixed_object_path(
                        Path(mixed_clang_tmp.name), mixed_input, mixed_index
                    )
                    mixed_cmd = [
                        mixed_compiler,
                        f"-std={standard}",
                        "-O0",
                    ] + mixed_clang_args + [
                        "-c",
                        str(mixed_input),
                        "-o",
                        str(mixed_object),
                    ]
                    mixed_exit, mixed_output = run_cmd(mixed_cmd)
                    if command_timed_out(mixed_output):
                        emit_final_failure(
                            "mixed compiler input compile timed out",
                            failure_kind="harness_error",
                            severity="high",
                            test_id=test_id,
                            test_bucket=test_bucket,
                            input_count=input_count,
                            run_enabled=run_enabled,
                            differential=differential,
                        )
                        print(mixed_output)
                        failures += 1
                        mixed_clang_failed = True
                        break
                    if mixed_exit != 0:
                        emit_final_failure(
                            f"mixed clang input compile exited {mixed_exit}",
                            failure_kind="harness_error",
                            severity="medium",
                            test_id=test_id,
                            test_bucket=test_bucket,
                            input_count=input_count,
                            run_enabled=run_enabled,
                            differential=differential,
                        )
                        print(mixed_output)
                        failures += 1
                        mixed_clang_failed = True
                        break
                    mixed_clang_object_paths.append(mixed_object)
                if mixed_clang_failed:
                    if mixed_clang_tmp is not None:
                        mixed_clang_tmp.cleanup()
                    continue
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
                cmd = cmd + [str(p) for p in mixed_clang_object_paths] + ["-o", str(runtime_exec_path)]
            elif link_enabled:
                link_tmp = tempfile.TemporaryDirectory(prefix=f"final-link-{test_id}-")
                link_output_path = Path(link_tmp.name) / "a.out"
                cmd = cmd + ["-o", str(link_output_path)]
            if has_diag_json or has_parser_diag:
                diag_json_tmp = tempfile.TemporaryDirectory(prefix=f"final-diag-{test_id}-")
                diag_json_path = Path(diag_json_tmp.name) / "diagnostics.json"
                cmd = cmd + ["--emit-diags-json", str(diag_json_path)]

            test_failed = False
            try:
                exit_code, output = run_cmd(cmd, env=cmd_env)

                if command_timed_out(output):
                    emit_final_failure(
                        "compiler timed out",
                        failure_kind="harness_error",
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
                    if (
                        expected_compile_exit is not None
                        and exit_code != expected_compile_exit
                    ):
                        emit_final_failure(
                            f"compiler exit mismatch (expected {expected_compile_exit}, "
                            f"got {exit_code})",
                            failure_kind="harness_error",
                            severity="high",
                            test_id=test_id,
                            test_bucket=test_bucket,
                            input_count=input_count,
                            run_enabled=run_enabled,
                            differential=differential,
                        )
                        print(output)
                        failures += 1
                        test_failed = True
                    elif expected_compile_exit is not None:
                        pass
                    elif has_ast and exit_code != 0:
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
                    if (expected_compile_exit is None and
                            not has_ast and not only_frontend and
                            exit_code != 0 and not allow_nonzero_exit):
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

                effective_frontend_diag_capture = should_capture_frontend_diagnostics(
                    capture_frontend_diag,
                    allow_empty_diag,
                    has_diag_text,
                    only_frontend,
                    exit_code,
                    expected_diag_frontend_capture(expects),
                )
                ast_text, diag_text, token_text, sema_text, ir_text = extract_sections(
                    output,
                    capture_frontend_diag=effective_frontend_diag_capture,
                )
                diag_text = normalize_final_suite_paths(diag_text)
                token_text = normalize_final_suite_paths(token_text)
                if diag_json_path is not None and diag_json_path.exists():
                    diag_json_raw = diag_json_path.read_text(encoding="utf-8")

                for marker in ir_contains:
                    if marker not in output:
                        emit_final_failure(
                            f"missing required IR marker {marker!r}",
                            failure_kind="ir_or_codegen_fail",
                            severity="high",
                            test_id=test_id,
                            test_bucket=test_bucket,
                            input_count=input_count,
                            run_enabled=run_enabled,
                            differential=differential,
                        )
                        failures += 1
                        test_failed = True
                for marker in ir_forbids:
                    if marker in output:
                        emit_final_failure(
                            f"forbidden IR marker present {marker!r}",
                            failure_kind="ir_or_codegen_fail",
                            severity="high",
                            test_id=test_id,
                            test_bucket=test_bucket,
                            input_count=input_count,
                            run_enabled=run_enabled,
                            differential=differential,
                        )
                        failures += 1
                        test_failed = True
                for expect_path in expects:
                    ext = expect_path.suffix
                    if ext == ".ast":
                        actual = ast_text
                    elif ext == ".diag":
                        actual = diag_text
                    elif ext == ".tokens":
                        actual = token_text
                    elif ext == ".sema":
                        actual = sema_text
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
                            actual = normalize_final_suite_paths(
                                normalize_diag_json_text(diag_json_raw)
                            )
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
                        if not should_update_expectation(update_ir_only, ext):
                            continue
                        conflicting_test_id = stage_expectation_update(
                            pending_expectation_updates,
                            expect_path,
                            actual,
                            test_id,
                        )
                        if conflicting_test_id is not None:
                            emit_final_failure(
                                f"shared expectation {expect_path} differs from "
                                f"{conflicting_test_id} during update",
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

                    if command_timed_out(run_stderr):
                        emit_final_failure(
                            "runtime timed out",
                            failure_kind="harness_error",
                            severity="high",
                            test_id=test_id,
                            test_bucket=test_bucket,
                            input_count=input_count,
                            run_enabled=run_enabled,
                            differential=differential,
                        )
                        failures += 1
                        test_failed = True

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
                        if update and should_update_expectation(
                            update_ir_only, expected_stdout_path.suffix
                        ):
                            conflicting_test_id = stage_expectation_update(
                                pending_expectation_updates,
                                expected_stdout_path,
                                run_stdout,
                                test_id,
                            )
                            if conflicting_test_id is not None:
                                emit_final_failure(
                                    f"shared expectation {expected_stdout_path} differs from "
                                    f"{conflicting_test_id} during update",
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

                    if expected_stdout_variant_paths:
                        missing_variant_paths = [
                            path
                            for path in expected_stdout_variant_paths
                            if not path.exists()
                        ]
                        if missing_variant_paths:
                            emit_final_failure(
                                "missing expectation variant(s): "
                                + ", ".join(str(path) for path in missing_variant_paths),
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
                            expected_stdout_variants = [
                                path.read_text(encoding="utf-8")
                                for path in expected_stdout_variant_paths
                            ]
                            if run_stdout not in expected_stdout_variants:
                                emit_final_failure(
                                    "stdout did not match any enumerated expectation variant",
                                    failure_kind="runtime_mismatch",
                                    severity="critical",
                                    test_id=test_id,
                                    test_bucket=test_bucket,
                                    input_count=input_count,
                                    run_enabled=run_enabled,
                                    differential=differential,
                                )
                                for path, expected_stdout in zip(
                                    expected_stdout_variant_paths,
                                    expected_stdout_variants,
                                ):
                                    print(diff_text(expected_stdout, run_stdout, path))
                                failures += 1
                                test_failed = True

                    if expected_stderr_path:
                        if update and should_update_expectation(
                            update_ir_only, expected_stderr_path.suffix
                        ):
                            conflicting_test_id = stage_expectation_update(
                                pending_expectation_updates,
                                expected_stderr_path,
                                run_stderr,
                                test_id,
                            )
                            if conflicting_test_id is not None:
                                emit_final_failure(
                                    f"shared expectation {expected_stderr_path} differs from "
                                    f"{conflicting_test_id} during update",
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
                                    ] + reference_args + [str(p) for p in input_paths + mixed_clang_input_paths] + [
                                        "-o",
                                        str(reference_exec),
                                    ]
                                    reference_compile_exit, reference_compile_output = run_cmd(reference_cmd)
                                    if command_timed_out(reference_compile_output):
                                        emit_final_failure(
                                            f"{reference_compiler_name} compile timed out",
                                            failure_kind="harness_error",
                                            severity="high",
                                            test_id=test_id,
                                            test_bucket=test_bucket,
                                            input_count=input_count,
                                            run_enabled=run_enabled,
                                            differential=differential,
                                        )
                                        print(reference_compile_output)
                                        failures += 1
                                        test_failed = True
                                    elif reference_compile_exit != 0:
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
                                        if command_timed_out(reference_stderr):
                                            emit_final_failure(
                                                f"{reference_compiler_name} runtime timed out",
                                                failure_kind="harness_error",
                                                severity="high",
                                                test_id=test_id,
                                                test_bucket=test_bucket,
                                                input_count=input_count,
                                                run_enabled=run_enabled,
                                                differential=differential,
                                            )
                                            failures += 1
                                            test_failed = True
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
                if link_tmp is not None:
                    link_tmp.cleanup()
                if mixed_clang_tmp is not None:
                    mixed_clang_tmp.cleanup()
                if diag_json_tmp is not None:
                    diag_json_tmp.cleanup()

        if failures:
            print(f"\n{failures} failing, {skipped} skipped")
            return 1
        if has_selector and selected == 0:
            emit_final_failure(
                "selector matched 0 tests "
                "(FINAL_FILTER / FINAL_PREFIX / FINAL_GLOB / FINAL_BUCKET / "
                "FINAL_TAG / FINAL_MANIFEST / FINAL_MANIFEST_GLOB / "
                "FINAL_EXPECT_EXT)",
                failure_kind="harness_error",
                severity="medium",
            )
            return 1
        if update:
            for path, (actual, _) in pending_expectation_updates.items():
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(actual, encoding="utf-8")
        if skipped:
            print(f"\n0 failing, {skipped} skipped")
        return 0
    finally:
        if memcheck_runtime_tmp is not None:
            memcheck_runtime_tmp.cleanup()
        staged_bin.cleanup()


if __name__ == "__main__":
    raise SystemExit(main())
