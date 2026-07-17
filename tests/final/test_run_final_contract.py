#!/usr/bin/env python3
import json
import sys
import unittest
from collections import Counter
from pathlib import Path

from run_final import (
    META_DIR,
    META_INDEX_PATH,
    ROOT,
    SUPPORTED_EXPECTATION_EXTENSIONS,
    extract_sections,
    is_meaningful_ir_expectation,
    load_meta,
    mixed_object_path,
    normalize_final_suite_paths,
    should_capture_frontend_diagnostics,
    should_update_expectation,
    stage_expectation_update,
    test_oracle_extensions,
    validate_test_definition,
    command_timed_out,
    expected_diag_frontend_capture,
    expected_compile_exit_from_diagnostics,
    run_cmd,
    run_program,
)


class FinalHarnessContractTests(unittest.TestCase):
    def test_mixed_object_paths_are_unique_for_colliding_basenames(self):
        directory = Path("/tmp/mixed")
        first = mixed_object_path(directory, Path("/one/shared.c"), 0)
        second = mixed_object_path(directory, Path("/two/shared.c"), 1)
        self.assertNotEqual(first, second)
        self.assertEqual(first.name, "000_shared.clang.o")
        self.assertEqual(second.name, "001_shared.clang.o")

    def test_compile_command_timeout_is_fail_closed(self):
        exit_code, output = run_cmd(
            [sys.executable, "-c", "import time; time.sleep(0.2)"],
            timeout_sec=0.01,
        )
        self.assertEqual(exit_code, 124)
        self.assertTrue(command_timed_out(output))

    def test_runtime_command_timeout_is_fail_closed(self):
        exit_code, _, stderr = run_program(
            [sys.executable, "-c", "import time; time.sleep(0.2)"],
            timeout_sec=0.01,
        )
        self.assertEqual(exit_code, 124)
        self.assertTrue(command_timed_out(stderr))

    def test_every_final_manifest_is_registered_once(self):
        index = json.loads(META_INDEX_PATH.read_text(encoding="utf-8"))
        registered = index["manifests"]
        self.assertEqual(len(registered), len(set(registered)))

        on_disk = set()
        for path in META_DIR.glob("*.json"):
            if path.name in {META_INDEX_PATH.name, "feature_map.json"}:
                continue
            data = json.loads(path.read_text(encoding="utf-8"))
            if isinstance(data, dict) and "tests" in data:
                on_disk.add(path.name)

        self.assertEqual(set(registered), on_disk)

    def test_ordered_ir_dump_is_captured_after_marker(self):
        output = "\n".join(
            [
                " AST Output:",
                "PROGRAM",
                "",
                " Semantic Analysis:",
                "Semantic analysis: no issues found.",
                "",
                "LLVM Code Generation:",
                "; ModuleID = 'compiler_module'",
                "define i32 @main() {",
                "  ret i32 0",
                "}",
                "",
            ]
        )

        _, diagnostics, _, _, ir = extract_sections(output)

        self.assertEqual(
            diagnostics,
            "Diagnostics:\nSemantic analysis: no issues found.\n",
        )
        self.assertEqual(
            ir,
            "IR:\n; ModuleID = 'compiler_module'\n"
            "define i32 @main() {\n  ret i32 0\n}\n",
        )

    def test_ir_only_update_rejects_other_expectation_types(self):
        self.assertTrue(should_update_expectation(True, ".ir"))
        for extension in (".ast", ".diag", ".diagjson", ".sema", ".stdout"):
            with self.subTest(extension=extension):
                self.assertFalse(should_update_expectation(True, extension))

    def test_normal_update_accepts_every_expectation_type(self):
        for extension in (".ir", ".ast", ".diag", ".sema", ".stdout"):
            with self.subTest(extension=extension):
                self.assertTrue(should_update_expectation(False, extension))

    def test_shared_expectation_updates_require_identical_content(self):
        pending = {}
        path = ROOT / "expect/shared.stdout"
        self.assertIsNone(stage_expectation_update(pending, path, "same\n", "first"))
        self.assertIsNone(stage_expectation_update(pending, path, "same\n", "second"))
        self.assertEqual(
            stage_expectation_update(pending, path, "different\n", "third"),
            "first",
        )
        self.assertEqual(pending[path.resolve()], ("same\n", "first"))

    def test_frontend_error_capture_is_automatic_for_text_diagnostics(self):
        self.assertTrue(
            should_capture_frontend_diagnostics(False, False, True, True, 1)
        )
        self.assertFalse(
            should_capture_frontend_diagnostics(False, False, True, True, 0)
        )
        self.assertFalse(
            should_capture_frontend_diagnostics(False, False, False, True, 1)
        )
        self.assertTrue(
            should_capture_frontend_diagnostics(True, False, False, False, 0)
        )
        self.assertFalse(
            should_capture_frontend_diagnostics(False, True, True, True, 1)
        )

    def test_existing_diag_oracle_controls_frontend_capture(self):
        semantic = ROOT / "expect/12__diag_incompatible_ptr.diag"
        parser = ROOT / "expect/12__missing_semicolon.diag"
        self.assertFalse(expected_diag_frontend_capture([semantic]))
        self.assertTrue(expected_diag_frontend_capture([parser]))
        self.assertFalse(
            should_capture_frontend_diagnostics(
                False, False, True, True, 1, expected_capture=False
            )
        )
        self.assertTrue(
            should_capture_frontend_diagnostics(
                False, False, True, True, 1, expected_capture=True
            )
        )

    def test_diagnostic_oracle_implies_exact_compile_exit(self):
        hard_error = ROOT / "expect/10__redeclaration_mismatch.diag"
        warning = ROOT / "expect/12__diag_invalid_shift_width.diag"
        clean = ROOT / "expect/10__extern_array_consistent_definition.diag"
        self.assertEqual(expected_compile_exit_from_diagnostics([hard_error]), 1)
        self.assertIsNone(expected_compile_exit_from_diagnostics([warning]))
        self.assertIsNone(expected_compile_exit_from_diagnostics([clean]))
        self.assertIsNone(expected_compile_exit_from_diagnostics([]))

    def test_final_suite_paths_are_checkout_independent(self):
        for absolute in (
            str(ROOT.parent.parent / "tests/final/cases/example.c"),
            "/tmp/other-checkout/fisiCs/tests/final/cases/example.c",
        ):
            with self.subTest(absolute=absolute):
                self.assertEqual(
                    normalize_final_suite_paths(f"Error: {absolute}:3:2\n"),
                    "Error: tests/final/cases/example.c:3:2\n",
                )

    def test_empty_registered_diagnostics_are_explicit(self):
        diag_expectation_count = 0
        for test in load_meta()["tests"]:
            for relative_path in test.get("expects", []):
                if not relative_path.endswith(".diag"):
                    continue
                diag_expectation_count += 1
                path = ROOT / relative_path
                with self.subTest(test_id=test["id"], path=relative_path):
                    self.assertTrue(path.is_file(), f"missing diagnostic expectation: {path}")
                    if path.stat().st_size == 0:
                        self.assertTrue(
                            test.get("allow_empty_diag", False),
                            f"empty diagnostic expectation is not explicit: {path}",
                        )
        self.assertGreater(diag_expectation_count, 0)

    def test_registered_ir_expectations_are_meaningful(self):
        ir_expectation_count = 0
        for test in load_meta()["tests"]:
            for relative_path in test.get("expects", []):
                if not relative_path.endswith(".ir"):
                    continue
                ir_expectation_count += 1
                path = ROOT / relative_path
                with self.subTest(test_id=test["id"], path=relative_path):
                    self.assertTrue(path.is_file(), f"missing IR expectation: {path}")
                    self.assertTrue(
                        is_meaningful_ir_expectation(
                            path.read_text(encoding="utf-8")
                        ),
                        f"IR expectation lacks a module or explicit skip record: {path}",
                    )
        self.assertEqual(ir_expectation_count, 67)

    def test_negative_empty_diagnostic_json_is_explicit(self):
        empty_count = 0
        explicit_negative_empty_count = 0
        for test in load_meta()["tests"]:
            tags = {str(tag) for tag in test.get("tags", [])}
            identifier = str(test["id"])
            claims_diagnostic = bool(tags & {"negative", "reject", "strict"}) or any(
                marker in identifier for marker in ("reject", "strict", "conflict")
            )
            explicit_control = bool(
                tags
                & {
                    "clean",
                    "clean-path",
                    "clean-control",
                    "control",
                    "current",
                    "current-threshold",
                }
            ) or any(
                marker in identifier
                for marker in (
                    "current_empty",
                    "current_sparse",
                    "_clean",
                    "_control",
                    "_compatible",
                )
            )
            for relative_path in test.get("expects", []):
                if not relative_path.endswith(".diagjson"):
                    continue
                data = json.loads(
                    (ROOT / relative_path).read_text(encoding="utf-8")
                )
                if int(
                    data.get("diag_count", len(data.get("diagnostics", [])))
                ) != 0:
                    continue
                empty_count += 1
                if claims_diagnostic and not explicit_control:
                    explicit_negative_empty_count += 1
                    with self.subTest(test_id=identifier, path=relative_path):
                        self.assertTrue(
                            test.get("allow_empty_diag_json", False),
                            "negative/strict empty diagnostics JSON is not explicit",
                        )
        self.assertGreater(empty_count, 0)
        self.assertGreater(explicit_negative_empty_count, 0)

    def test_registered_diagnostic_json_matches_schema_and_counts(self):
        top_level_fields = {
            "diag_count",
            "diagnostics",
            "error_count",
            "note_count",
            "profile",
            "schema_version",
            "warning_count",
        }
        diagnostic_field_types = {
            "category_id": int,
            "category_name": str,
            "code": int,
            "code_id": int,
            "code_name": str,
            "column": int,
            "has_file": bool,
            "has_hint": bool,
            "has_message": bool,
            "kind": int,
            "length": int,
            "line": int,
            "severity_id": int,
            "severity_name": str,
            "stage": str,
        }
        checked_paths = set()
        diagnostic_count = 0
        for test in load_meta()["tests"]:
            for relative_path in test.get("expects", []):
                if not relative_path.endswith(".diagjson"):
                    continue
                path = ROOT / relative_path
                if path in checked_paths:
                    continue
                checked_paths.add(path)
                data = json.loads(path.read_text(encoding="utf-8"))
                with self.subTest(path=relative_path, field="schema"):
                    self.assertEqual(set(data), top_level_fields)
                    self.assertEqual(data["profile"], "fisics_diagnostics_v1")
                    self.assertEqual(data["schema_version"], 1)
                    self.assertIsInstance(data["diagnostics"], list)

                severities = Counter()
                for index, diagnostic in enumerate(data["diagnostics"]):
                    diagnostic_count += 1
                    with self.subTest(path=relative_path, diagnostic=index):
                        self.assertIsInstance(diagnostic, dict)
                        self.assertTrue(
                            diagnostic_field_types.keys() <= diagnostic.keys()
                        )
                        for field, expected_type in diagnostic_field_types.items():
                            self.assertIs(type(diagnostic[field]), expected_type)
                        severity = diagnostic["severity_name"].lower()
                        self.assertIn(severity, {"error", "warning", "note"})
                        severities[severity] += 1

                with self.subTest(path=relative_path, field="counts"):
                    self.assertEqual(data["diag_count"], len(data["diagnostics"]))
                    self.assertEqual(data["error_count"], severities["error"])
                    self.assertEqual(data["warning_count"], severities["warning"])
                    self.assertEqual(data["note_count"], severities["note"])

        self.assertGreater(len(checked_paths), 0)
        self.assertGreater(diagnostic_count, 0)

    def test_registered_semantic_models_do_not_capture_llvm_ir(self):
        sema_expectation_count = 0
        for test in load_meta()["tests"]:
            for relative_path in test.get("expects", []):
                if not relative_path.endswith(".sema"):
                    continue
                sema_expectation_count += 1
                path = ROOT / relative_path
                text = path.read_text(encoding="utf-8")
                with self.subTest(test_id=test["id"], path=relative_path):
                    self.assertNotIn("; ModuleID =", text)
                    self.assertNotIn("LLVM Code Generation:", text)
        self.assertGreater(sema_expectation_count, 0)

    def test_registered_expectations_do_not_embed_checkout_paths(self):
        checked_paths = set()
        for test in load_meta()["tests"]:
            for relative_path in test.get("expects", []):
                if relative_path in checked_paths:
                    continue
                checked_paths.add(relative_path)
                path = ROOT / relative_path
                text = path.read_text(encoding="utf-8")
                with self.subTest(path=relative_path):
                    self.assertEqual(text, normalize_final_suite_paths(text))
        self.assertGreater(len(checked_paths), 0)

    def test_definition_validation_rejects_vacuous_and_ignored_oracles(self):
        invalid_cases = (
            ({"id": "empty", "expects": [], "run": False}, "no file"),
            (
                {"id": "suffix", "expects": ["expect/value.stdout"]},
                "unsupported expectation",
            ),
            (
                {
                    "id": "ignored-runtime",
                    "expects": ["expect/value.diag"],
                    "expect_exit": 3,
                },
                "require run=true",
            ),
            (
                {
                    "id": "ignored-json",
                    "expects": ["expect/value.diag"],
                    "capture_diag_json": True,
                },
                "obsolete diagnostic JSON capture field",
            ),
            (
                {
                    "id": "wrong-runtime-suffix",
                    "expects": [],
                    "run": True,
                    "expected_stdout": "expect/value.txt",
                },
                "must use the .stdout extension",
            ),
            (
                {
                    "id": "unknown-field",
                    "expects": ["expect/value.diag"],
                    "expected_stdut": "expect/value.stdout",
                },
                "unknown manifest field",
            ),
            (
                {
                    "id": "dropped-primary",
                    "input": "cases/main.c",
                    "inputs": ["cases/helper.c"],
                    "expects": ["expect/value.diag"],
                },
                "input must be present",
            ),
            (
                {
                    "id": "absolute-input",
                    "input": "/tmp/source.c",
                    "expects": ["expect/value.diag"],
                },
                "input must remain inside the repository",
            ),
            (
                {
                    "id": "escaped-input",
                    "input": "../../../outside.c",
                    "expects": ["expect/value.diag"],
                },
                "input must remain inside the repository",
            ),
            (
                {
                    "id": "escaped-expectation",
                    "expects": ["../outside.diag"],
                },
                "expects entries must remain inside the final suite",
            ),
            (
                {
                    "id": "duplicate-inputs",
                    "input": "cases/main.c",
                    "inputs": ["cases/main.c", "cases/main.c"],
                    "expects": ["expect/value.diag"],
                },
                "inputs entries must be unique",
            ),
            (
                {
                    "id": "classification-field",
                    "expects": ["expect/value.diag"],
                    "abi_sensitive": True,
                },
                "unknown manifest field",
            ),
            (
                {
                    "id": "bad-status",
                    "expects": ["expect/value.diag"],
                    "status": "xfail",
                },
                "status=ok",
            ),
            (
                {
                    "id": "unknown-requirement",
                    "expects": ["expect/value.diag"],
                    "requires": ["tokn-dump"],
                },
                "unknown requirement",
            ),
            (
                {"id": "link-without-exit", "expects": [], "link": True},
                "requires expect_compile_exit",
            ),
            (
                {
                    "id": "ambiguous-compile-exit",
                    "expects": [],
                    "expect_compile_exit": 1,
                    "allow_nonzero_exit": True,
                },
                "cannot be combined",
            ),
            (
                {
                    "id": "empty-json-without-json",
                    "expects": ["expect/value.diag"],
                    "allow_empty_diag_json": True,
                },
                "requires a .diagjson expectation",
            ),
        )
        for test, expected_fragment in invalid_cases:
            with self.subTest(test=test["id"]):
                self.assertTrue(
                    any(
                        expected_fragment in error
                        for error in validate_test_definition(test)
                    )
                )

        valid_cases = (
            {"id": "file", "expects": ["expect/value.diag"]},
            {"id": "marker", "expects": [], "ir_contains": ["define i32"]},
            {"id": "runtime", "expects": [], "run": True},
            {
                "id": "link-failure",
                "expects": [],
                "link": True,
                "expect_compile_exit": 1,
            },
            {
                "id": "ordered-inputs",
                "input": "cases/main.c",
                "inputs": ["cases/helper.c", "cases/main.c"],
                "expects": ["expect/value.diag"],
            },
            {
                "id": "repo-owned-example-input",
                "input": "../../examples/source.c",
                "expects": ["expect/value.diag"],
            },
        )
        for test in valid_cases:
            with self.subTest(test=test["id"]):
                self.assertEqual(validate_test_definition(test), [])

    def test_oracle_extension_selector_includes_runtime_outputs(self):
        test = {
            "expects": ["expect/value.diag"],
            "expected_stdout": "expect/value.stdout",
            "expected_stderr": "expect/value.stderr",
        }
        self.assertEqual(
            test_oracle_extensions(test), {".diag", ".stdout", ".stderr"}
        )

    def test_registered_tests_have_supported_effective_oracles(self):
        tests = load_meta()["tests"]
        self.assertGreater(len(tests), 0)
        for test in tests:
            with self.subTest(test_id=test["id"]):
                self.assertEqual(test.get("status"), "ok")
                self.assertEqual(validate_test_definition(test), [])

    def test_registered_expectation_files_exist_and_stay_in_suite(self):
        expectation_count = 0
        for test in load_meta()["tests"]:
            paths = list(test.get("expects", []))
            for field in ("expected_stdout", "expected_stderr"):
                if test.get(field):
                    paths.append(test[field])
            for relative_path in paths:
                expectation_count += 1
                relative = Path(relative_path)
                path = ROOT / relative
                with self.subTest(test_id=test["id"], path=relative_path):
                    self.assertFalse(relative.is_absolute())
                    self.assertNotIn("..", relative.parts)
                    self.assertTrue(path.is_file(), f"missing expectation: {path}")
                    if relative_path in test.get("expects", []):
                        self.assertIn(path.suffix, SUPPORTED_EXPECTATION_EXTENSIONS)
        self.assertGreater(expectation_count, 0)

    def test_registered_compiler_inputs_exist_and_are_unique(self):
        input_count = 0
        for test in load_meta()["tests"]:
            inputs = list(test.get("inputs") or [test.get("input")])
            inputs.extend(test.get("mixed_clang_inputs", []))
            inputs = [relative_path for relative_path in inputs if relative_path]
            input_count += len(inputs)
            with self.subTest(test_id=test["id"], field="duplicates"):
                self.assertEqual(len(inputs), len(set(inputs)))
            for relative_path in inputs:
                with self.subTest(test_id=test["id"], path=relative_path):
                    self.assertTrue(
                        (ROOT / relative_path).is_file(),
                        f"missing compiler input: {relative_path}",
                    )
        self.assertGreater(input_count, 0)


if __name__ == "__main__":
    unittest.main()
