#!/usr/bin/env python3
import os
import shutil
import sys
from pathlib import Path

from run_binary_support import (
    ARTIFACT_ROOT,
    build_env,
    classify_compile_failure,
    diff_text,
    emit_binary_failure,
    load_meta,
    parse_csv_env,
    resolve_pkg_config_flags,
    resolve_rel_path,
    run_compile,
    run_program,
    should_skip,
)


def main():
    raw_bin_path = sys.argv[1] if len(sys.argv) > 1 else "./fisics"
    bin_path = str(Path(raw_bin_path).resolve())
    if not Path(bin_path).exists():
        emit_binary_failure(
            f"compiler binary not found at {bin_path}",
            failure_kind="harness_error",
            severity="medium",
            raw_status="missing_compiler_binary",
        )
        return 1
    update = os.environ.get("UPDATE_BINARY", "0") == "1" or "--update" in sys.argv

    filt = os.environ.get("BINARY_FILTER", "").strip()
    prefixes = parse_csv_env("BINARY_PREFIX")
    levels = parse_csv_env("BINARY_LEVEL")
    manifests = parse_csv_env("BINARY_MANIFEST")
    has_selector = bool(filt or prefixes or levels or manifests)

    if update and not has_selector:
        emit_binary_failure(
            "UPDATE_BINARY requires selector "
            "(BINARY_FILTER/BINARY_PREFIX/BINARY_LEVEL/BINARY_MANIFEST)",
            failure_kind="harness_error",
            severity="medium",
            raw_status="update_requires_selector",
        )
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
        differential_with = str(test.get("differential_with", "")).strip()

        inputs = test.get("inputs")
        if inputs:
            input_paths = [resolve_rel_path(p) for p in inputs]
        else:
            input_paths = [resolve_rel_path(test["input"])]
        input_count = len(input_paths)

        run_dir = ARTIFACT_ROOT / test_id
        if run_dir.exists():
            shutil.rmtree(run_dir)
        run_dir.mkdir(parents=True, exist_ok=True)

        compile_timeout = int(test.get("compile_timeout_sec", test.get("timeout_sec", 10)))
        try:
            pkg_config_flags = resolve_pkg_config_flags(test, env_overrides)
        except RuntimeError as exc:
            emit_binary_failure(
                str(exc),
                failure_kind="harness_error",
                severity="medium",
                raw_status="harness_error",
                test_id=test_id,
                category=category,
                input_count=input_count,
                level=test_level,
                differential_with=differential_with,
            )
            failures += 1
            continue

        if category == "compile_only" or category == "compile_fail":
            if len(input_paths) != 1:
                emit_binary_failure(
                    f"{category} requires exactly one input",
                    failure_kind="harness_error",
                    severity="medium",
                    raw_status="harness_error",
                    test_id=test_id,
                    category=category,
                    input_count=input_count,
                    level=test_level,
                    differential_with=differential_with,
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
            emit_binary_failure(
                f"unsupported category '{category}'",
                failure_kind="harness_error",
                severity="medium",
                raw_status="harness_error",
                test_id=test_id,
                category=category,
                input_count=input_count,
                level=test_level,
                differential_with=differential_with,
            )
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
                emit_binary_failure(
                    f"compiler timed out after {compile_timeout}s",
                    failure_kind="ir_or_codegen_fail",
                    severity="high",
                    raw_status="compile_timeout",
                    test_id=test_id,
                    category=category,
                    input_count=input_count,
                    level=test_level,
                    differential_with=differential_with,
                )
                failures += 1
            continue
        if expect_compile_timeout:
            emit_binary_failure(
                f"expected compiler timeout after {compile_timeout}s",
                failure_kind="harness_error",
                severity="medium",
                raw_status="compile_timeout_unexpected",
                test_id=test_id,
                category=category,
                input_count=input_count,
                level=test_level,
                differential_with=differential_with,
            )
            failures += 1
            continue

        forbidden_compile_fragments = [
            str(x) for x in test.get("compile_output_must_not_contain", [])
        ]
        if forbidden_compile_fragments:
            lowered_output = (compile_result["output"] or "").lower()
            found_fragments = []
            for fragment in forbidden_compile_fragments:
                if fragment.lower() in lowered_output:
                    found_fragments.append(fragment)
            if found_fragments:
                emit_binary_failure(
                    "found forbidden compile output fragments: "
                    f"{', '.join(found_fragments)}",
                    failure_kind="wrong_diagnostics",
                    severity="medium",
                    raw_status="compile_output_forbidden",
                    test_id=test_id,
                    category=category,
                    input_count=input_count,
                    level=test_level,
                    differential_with=differential_with,
                )
                if compile_result["output"]:
                    print(compile_result["output"])
                failures += 1
                continue

        if category == "link_fail":
            if compile_result["exit_code"] == 0:
                emit_binary_failure(
                    "expected link failure",
                    failure_kind="link_fail",
                    severity="high",
                    raw_status="compile_succeeded_unexpected",
                    test_id=test_id,
                    category=category,
                    input_count=input_count,
                    level=test_level,
                    differential_with=differential_with,
                )
                failures += 1
                continue
            failure_kind = classify_compile_failure(compile_result["output"])
            if failure_kind != "link_fail":
                emit_binary_failure(
                    "expected link-stage failure, got compile-stage failure",
                    failure_kind="ir_or_codegen_fail",
                    severity="high",
                    raw_status="compile_fail_unexpected",
                    test_id=test_id,
                    category=category,
                    input_count=input_count,
                    level=test_level,
                    differential_with=differential_with,
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
                emit_binary_failure(
                    "missing expected output fragments: "
                    f"{', '.join(missing_contains)}",
                    failure_kind="wrong_diagnostics",
                    severity="medium",
                    raw_status="link_fail_unexpected",
                    test_id=test_id,
                    category=category,
                    input_count=input_count,
                    level=test_level,
                    differential_with=differential_with,
                )
                if compile_result["output"]:
                    print(compile_result["output"])
                failures += 1
                continue
            print(f"PASS {test_id}")
            continue

        if category == "compile_fail":
            if compile_result["exit_code"] == 0:
                emit_binary_failure(
                    "expected compile failure",
                    failure_kind="wrong_diagnostics",
                    severity="high",
                    raw_status="compile_succeeded_unexpected",
                    test_id=test_id,
                    category=category,
                    input_count=input_count,
                    level=test_level,
                    differential_with=differential_with,
                )
                failures += 1
                continue
            failure_kind = classify_compile_failure(compile_result["output"])
            if failure_kind != "compile_fail":
                emit_binary_failure(
                    "expected compile-stage failure, got link-stage failure",
                    failure_kind="link_fail",
                    severity="high",
                    raw_status="link_fail_unexpected",
                    test_id=test_id,
                    category=category,
                    input_count=input_count,
                    level=test_level,
                    differential_with=differential_with,
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
                emit_binary_failure(
                    "missing expected output fragments: "
                    f"{', '.join(missing_contains)}",
                    failure_kind="wrong_diagnostics",
                    severity="medium",
                    raw_status="compile_fail_unexpected",
                    test_id=test_id,
                    category=category,
                    input_count=input_count,
                    level=test_level,
                    differential_with=differential_with,
                )
                if compile_result["output"]:
                    print(compile_result["output"])
                failures += 1
                continue
            print(f"PASS {test_id}")
            continue

        if compile_result["exit_code"] != 0:
            compile_failure_kind = classify_compile_failure(compile_result["output"])
            fail_code = (
                "link_fail_unexpected"
                if compile_failure_kind == "link_fail"
                else "compile_fail_unexpected"
            )
            canonical_kind = (
                "link_fail"
                if compile_failure_kind == "link_fail"
                else "ir_or_codegen_fail"
            )
            emit_binary_failure(
                f"compiler exited {compile_result['exit_code']}",
                failure_kind=canonical_kind,
                severity="high",
                raw_status=fail_code,
                test_id=test_id,
                category=category,
                input_count=input_count,
                level=test_level,
                differential_with=differential_with,
            )
            if compile_result["output"]:
                print(compile_result["output"])
            failures += 1
            continue
        if not output_path.exists():
            emit_binary_failure(
                f"expected output missing: {output_path}",
                failure_kind="harness_error",
                severity="medium",
                raw_status="harness_error",
                test_id=test_id,
                category=category,
                input_count=input_count,
                level=test_level,
                differential_with=differential_with,
            )
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
            emit_binary_failure(
                f"program timed out after {run_timeout}s",
                failure_kind="runtime_crash",
                severity="critical",
                raw_status="runtime_timeout",
                test_id=test_id,
                category=category,
                input_count=input_count,
                level=test_level,
                differential_with=differential_with,
            )
            failures += 1
            continue

        test_failed = False
        try:
            expect_exit = int(test.get("expect_exit", 0))
        except (TypeError, ValueError):
            emit_binary_failure(
                "expect_exit must be an integer",
                failure_kind="harness_error",
                severity="medium",
                raw_status="harness_error",
                test_id=test_id,
                category=category,
                input_count=input_count,
                level=test_level,
                differential_with=differential_with,
            )
            failures += 1
            continue
        if run_result["exit_code"] != expect_exit:
            emit_binary_failure(
                f"expected {expect_exit}, got {run_result['exit_code']}",
                failure_kind="runtime_mismatch",
                severity="critical",
                raw_status="runtime_exit_mismatch",
                test_id=test_id,
                category=category,
                input_count=input_count,
                level=test_level,
                differential_with=differential_with,
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
                    emit_binary_failure(
                        f"missing expectation {path}",
                        failure_kind="harness_error",
                        severity="medium",
                        raw_status="harness_error",
                        test_id=test_id,
                        category=category,
                        input_count=input_count,
                        level=test_level,
                        differential_with=differential_with,
                    )
                    failures += 1
                    test_failed = True
                    return
                expected = path.read_text(encoding="utf-8")
                if expected != actual:
                    emit_binary_failure(
                        f"mismatch in {path.name}",
                        failure_kind="runtime_mismatch",
                        severity="critical",
                        raw_status=f"runtime_{kind}_mismatch",
                        test_id=test_id,
                        category=category,
                        input_count=input_count,
                        level=test_level,
                        differential_with=differential_with,
                    )
                    print(diff_text(expected, actual, path))
                    failures += 1
                    test_failed = True
            else:
                if actual != "":
                    emit_binary_failure(
                        f"expected empty {kind}",
                        failure_kind="runtime_mismatch",
                        severity="critical",
                        raw_status=f"runtime_{kind}_mismatch",
                        test_id=test_id,
                        category=category,
                        input_count=input_count,
                        level=test_level,
                        differential_with=differential_with,
                    )
                    failures += 1
                    test_failed = True

        check_stream("stdout", run_result["stdout"])
        check_stream("stderr", run_result["stderr"])

        expected_files = test.get("expected_files", [])
        if expected_files:
            for file_spec in expected_files:
                if not isinstance(file_spec, dict) or "path" not in file_spec:
                    emit_binary_failure(
                        "expected_files entries require 'path'",
                        failure_kind="harness_error",
                        severity="medium",
                        raw_status="harness_error",
                        test_id=test_id,
                        category=category,
                        input_count=input_count,
                        level=test_level,
                        differential_with=differential_with,
                    )
                    failures += 1
                    test_failed = True
                    continue
                rel_path = str(file_spec["path"])
                full_path = run_dir / rel_path
                if not full_path.exists():
                    emit_binary_failure(
                        f"expected runtime file missing: {full_path}",
                        failure_kind="runtime_mismatch",
                        severity="critical",
                        raw_status="runtime_file_missing",
                        test_id=test_id,
                        category=category,
                        input_count=input_count,
                        level=test_level,
                        differential_with=differential_with,
                    )
                    failures += 1
                    test_failed = True
                    continue
                if "content" in file_spec:
                    actual_content = full_path.read_text(encoding="utf-8")
                    expected_content = str(file_spec["content"])
                    if actual_content != expected_content:
                        emit_binary_failure(
                            f"runtime file content mismatch for {rel_path}",
                            failure_kind="runtime_mismatch",
                            severity="critical",
                            raw_status="runtime_file_mismatch",
                            test_id=test_id,
                            category=category,
                            input_count=input_count,
                            level=test_level,
                            differential_with=differential_with,
                        )
                        print(diff_text(expected_content, actual_content, full_path))
                        failures += 1
                        test_failed = True

        if not test_failed and differential_with:
            ub = bool(test.get("ub", False))
            impl_defined = bool(test.get("impl_defined", False))
            if ub:
                print(f"SKIP {test_id}: differential disabled for ub=true")
                skipped += 1
                continue
            if impl_defined:
                print(f"SKIP {test_id}: differential disabled for impl_defined=true")
                skipped += 1
                continue
            if differential_with != "clang":
                emit_binary_failure(
                    f"unsupported differential_with='{differential_with}'",
                    failure_kind="harness_error",
                    severity="medium",
                    raw_status="harness_error",
                    test_id=test_id,
                    category=category,
                    input_count=input_count,
                    level=test_level,
                    differential_with=differential_with,
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
                    emit_binary_failure(
                        f"clang timed out after {clang_compile_timeout}s",
                        failure_kind="harness_error",
                        severity="medium",
                        raw_status="differential_compile_timeout",
                        test_id=test_id,
                        category=category,
                        input_count=input_count,
                        level=test_level,
                        differential_with=differential_with,
                    )
                    failures += 1
                    test_failed = True
                elif clang_compile["exit_code"] != 0:
                    emit_binary_failure(
                        f"clang exited {clang_compile['exit_code']}",
                        failure_kind="harness_error",
                        severity="medium",
                        raw_status="differential_compile_fail",
                        test_id=test_id,
                        category=category,
                        input_count=input_count,
                        level=test_level,
                        differential_with=differential_with,
                    )
                    if clang_compile["output"]:
                        print(clang_compile["output"])
                    failures += 1
                    test_failed = True
                elif not clang_output.exists():
                    emit_binary_failure(
                        f"clang output missing: {clang_output}",
                        failure_kind="harness_error",
                        severity="medium",
                        raw_status="differential_compile_fail",
                        test_id=test_id,
                        category=category,
                        input_count=input_count,
                        level=test_level,
                        differential_with=differential_with,
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
                        emit_binary_failure(
                            f"clang binary timed out after {clang_run_timeout}s",
                            failure_kind="harness_error",
                            severity="medium",
                            raw_status="differential_runtime_timeout",
                            test_id=test_id,
                            category=category,
                            input_count=input_count,
                            level=test_level,
                            differential_with=differential_with,
                        )
                        failures += 1
                        test_failed = True
                    else:
                        if run_result["exit_code"] != clang_run["exit_code"]:
                            emit_binary_failure(
                                "exit mismatch "
                                f"fisics={run_result['exit_code']} clang={clang_run['exit_code']}",
                                failure_kind="runtime_mismatch",
                                severity="critical",
                                raw_status="differential_mismatch",
                                test_id=test_id,
                                category=category,
                                input_count=input_count,
                                level=test_level,
                                differential_with=differential_with,
                            )
                            failures += 1
                            test_failed = True
                        if run_result["stdout"] != clang_run["stdout"]:
                            emit_binary_failure(
                                "stdout mismatch vs clang",
                                failure_kind="runtime_mismatch",
                                severity="critical",
                                raw_status="differential_mismatch",
                                test_id=test_id,
                                category=category,
                                input_count=input_count,
                                level=test_level,
                                differential_with=differential_with,
                            )
                            print(diff_text(clang_run["stdout"], run_result["stdout"], "clang.stdout"))
                            failures += 1
                            test_failed = True
                        if run_result["stderr"] != clang_run["stderr"]:
                            emit_binary_failure(
                                "stderr mismatch vs clang",
                                failure_kind="runtime_mismatch",
                                severity="critical",
                                raw_status="differential_mismatch",
                                test_id=test_id,
                                category=category,
                                input_count=input_count,
                                level=test_level,
                                differential_with=differential_with,
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
        emit_binary_failure(
            "selector matched 0 binary tests",
            failure_kind="harness_error",
            severity="medium",
            raw_status="selector_zero_match",
        )
        return 1
    if skipped:
        print(f"\n0 failing, {skipped} skipped")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
