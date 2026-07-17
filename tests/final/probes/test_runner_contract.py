#!/usr/bin/env python3

import json
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import Mock, patch

from lib.models import DiagnosticExpectation, DiagnosticJsonProbe, DiagnosticProbe, RuntimeProbe
from lib.runner import (
    compile_output_substrings,
    diagnostic_record_matches,
    main,
    mixed_object_path,
    run_diag_json_probe,
    run_diag_probe,
    run_runtime_probe,
    stable_json_identity_expectations,
    stable_oracle_path_for_probe,
    stable_text_identity_markers,
)


class ProbeRunnerContractTests(unittest.TestCase):
    def test_diagnostic_identity_rejects_wrong_file_message_or_trace_order(self):
        expectation = DiagnosticExpectation(
            code=3000,
            file="virtual.c",
            message_substrings=("macro expansion failed",),
            macro_trace=(
                {"macro": "WRAP", "role": "call_site", "file": "virtual.c"},
                {"macro": "WRAP", "role": "definition", "file": "source.c"},
            ),
        )
        valid = {
            "code": 3000,
            "file": "/tmp/virtual.c",
            "message": "macro expansion failed at token",
            "macro_trace": [
                {"macro": "WRAP", "role": "call_site", "file": "/tmp/virtual.c"},
                {"macro": "WRAP", "role": "definition", "file": "/tmp/source.c"},
            ],
        }
        self.assertTrue(diagnostic_record_matches(valid, expectation))
        for field, replacement in (
            ("file", "other.c"),
            ("message", "unrelated failure"),
            ("macro_trace", list(reversed(valid["macro_trace"]))),
        ):
            candidate = dict(valid)
            candidate[field] = replacement
            with self.subTest(field=field):
                self.assertFalse(diagnostic_record_matches(candidate, expectation))

    def test_stable_json_identity_preserves_file_message_and_macro_trace(self):
        with tempfile.TemporaryDirectory() as tmp:
            oracle = Path(tmp) / "expected.diagjson"
            oracle.write_text(
                json.dumps({"diagnostics": [{
                    "code": 3000,
                    "line": 41,
                    "column": 7,
                    "has_file": True,
                    "file": "virtual.c",
                    "message": "macro expansion failed",
                    "severity_name": "error",
                    "stage": "preprocess",
                    "macro_trace": [
                        {"macro": "WRAP", "role": "call_site", "file": "virtual.c", "line": 41, "column": 7},
                        {"macro": "WRAP", "role": "definition", "file": "source.c", "line": 2, "column": 1},
                    ],
                }]}),
                encoding="utf-8",
            )
            with patch("lib.runner.stable_oracle_path_for_probe", return_value=oracle):
                expectations = stable_json_identity_expectations(
                    "contract__rich", Path("/probe/source.c")
                )
        self.assertEqual(len(expectations), 1)
        self.assertEqual(expectations[0].file, "virtual.c")
        self.assertEqual(expectations[0].message_substrings, ("macro expansion failed",))
        self.assertEqual(expectations[0].macro_trace[1]["role"], "definition")

    def test_mixed_object_paths_are_unique_for_colliding_basenames(self):
        directory = Path("/tmp/mixed")
        first = mixed_object_path(directory, Path("/one/shared.c"), 0)
        second = mixed_object_path(directory, Path("/two/shared.c"), 1)
        self.assertNotEqual(first, second)
        self.assertEqual(first.name, "000_shared.clang.o")
        self.assertEqual(second.name, "001_shared.clang.o")

    def test_unique_source_oracle_precedes_heuristic_id_variant(self):
        source_owner = Path("/stable/source-owner.diagjson")
        with patch(
            "lib.runner.stable_oracle_path_for_source",
            return_value=source_owner,
        ), patch("lib.runner.stable_oracle_index") as index:
            selected = stable_oracle_path_for_probe(
                "15__probe_diagjson_collision",
                "diagnostic-json",
                Path("/probe/source.c"),
                ".diagjson",
            )
        self.assertEqual(selected, source_owner)
        index.assert_not_called()

    def test_stable_text_identity_discards_checkout_relative_path_suffix(self):
        with tempfile.TemporaryDirectory() as tmp:
            diag = Path(tmp) / "expected.diag"
            diag.write_text(
                "Error: Expected identifier at tests/final/probes/diagnostics/x.c:3 "
                "(got ';')\n",
                encoding="utf-8",
            )
            with patch("lib.runner.stable_oracle_path_for_probe", return_value=diag):
                markers = stable_text_identity_markers("contract__path", Path("/unused.c"))
        self.assertEqual(markers, ("Expected identifier",))

    def test_parser_only_identity_uses_path_independent_line_marker(self):
        with tempfile.TemporaryDirectory() as tmp:
            diag = Path(tmp) / "expected.diag"
            diag.write_text(
                "Diagnostics:\nSemantic analysis: no issues found.\n",
                encoding="utf-8",
            )
            parser_diag = Path(tmp) / "expected.pdiag"
            parser_diag.write_text(
                "code=1000 line=12 column=9 length=1 kind=0\n",
                encoding="utf-8",
            )

            def oracle_path(_probe_id, _family, _source, suffix):
                if suffix == ".diag":
                    return diag
                return parser_diag if suffix == ".pdiag" else None

            with patch("lib.runner.stable_oracle_path_for_probe", side_effect=oracle_path):
                markers = stable_text_identity_markers("contract__parser", Path("/unused.c"))
        self.assertEqual(markers, ("@diagnostic-line:12",))
        self.assertTrue(
            compile_output_substrings(
                "Error: parser failure at line 12",
                required_substrings=markers,
            )[0]
        )
        self.assertTrue(
            compile_output_substrings(
                "Error: parser failure at /checkout/source.c:12 (got ')')",
                required_substrings=markers,
            )[0]
        )

    def test_runtime_probe_fails_closed_on_missing_input(self):
        probe = RuntimeProbe(
            probe_id="contract__missing_runtime",
            source=Path("/definitely/missing/runtime.c"),
            note="contract canary",
        )
        status, summary, detail = run_runtime_probe(probe, None, Path("/unused/fisics"))
        self.assertEqual(status, "BLOCKED")
        self.assertEqual(summary, "probe input missing")
        self.assertIn("runtime.c", detail)

    def test_text_diagnostic_probe_fails_closed_on_timeout(self):
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp) / "timeout.c"
            source.write_text("int main(void) { return 0; }\n", encoding="utf-8")
            probe = DiagnosticProbe(
                probe_id="contract__diag_timeout",
                source=source,
                note="contract canary",
            )
            with patch("lib.runner.run_cmd", return_value=(124, "", True)):
                status, summary, _ = run_diag_probe(probe, Path("/unused/fisics"))
        self.assertEqual(status, "BLOCKED")
        self.assertIn("timeout", summary)

    def test_clean_text_probe_requires_successful_exit(self):
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp) / "clean.c"
            source.write_text("int main(void) { return 0; }\n", encoding="utf-8")
            probe = DiagnosticProbe(
                probe_id="contract__clean_exit",
                source=source,
                note="contract canary",
                expect_any_diagnostic=False,
            )
            with patch("lib.runner.run_cmd", return_value=(1, "internal failure", False)):
                status, summary, _ = run_diag_probe(probe, Path("/unused/fisics"))
        self.assertEqual(status, "BLOCKED")
        self.assertIn("unexpected compile exit", summary)

    def test_clean_json_probe_rejects_nonempty_payload(self):
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp) / "clean_json.c"
            source.write_text("int main(void) { return 0; }\n", encoding="utf-8")
            probe = DiagnosticJsonProbe(
                probe_id="contract__clean_json",
                source=source,
                note="contract canary",
                require_any_diagnostic=False,
            )

            def fake_run(cmd, _timeout, env=None):
                json_path = Path(cmd[cmd.index("--emit-diags-json") + 1])
                json_path.write_text(
                    json.dumps({"diagnostics": [{"code": 1}]}),
                    encoding="utf-8",
                )
                return 0, "", False

            with patch("lib.runner.run_cmd", side_effect=fake_run):
                status, summary, _ = run_diag_json_probe(probe, Path("/unused/fisics"))
        self.assertEqual(status, "BLOCKED")
        self.assertIn("unexpectedly has", summary)

    def test_positive_text_probe_rejects_crash_like_exit(self):
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp) / "crash.c"
            source.write_text("int main(void) { return 0; }\n", encoding="utf-8")
            probe = DiagnosticProbe(
                probe_id="contract__diag_crash_exit",
                source=source,
                note="contract canary",
                required_substrings=("intended diagnostic",),
            )
            with patch(
                "lib.runner.run_cmd",
                return_value=(70, "Error: intended diagnostic", False),
            ):
                status, summary, _ = run_diag_probe(probe, Path("/unused/fisics"))
        self.assertEqual(status, "BLOCKED")
        self.assertIn("unexpected compile exit", summary)

    def test_positive_text_probe_requires_identity_markers(self):
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp) / "generic.c"
            source.write_text("int main(void) { return 0; }\n", encoding="utf-8")
            probe = DiagnosticProbe(
                probe_id="contract__diag_generic",
                source=source,
                note="contract canary",
            )
            with patch("lib.runner.run_cmd", return_value=(1, "Error: unrelated", False)):
                status, summary, _ = run_diag_probe(probe, Path("/unused/fisics"))
        self.assertEqual(status, "BLOCKED")
        self.assertIn("lacks identity", summary)

    def test_positive_json_requires_atomic_record_match(self):
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp) / "atomic.c"
            source.write_text("int main(void) { return 0; }\n", encoding="utf-8")
            probe = DiagnosticJsonProbe(
                probe_id="contract__diagjson_atomic",
                source=source,
                note="contract canary",
                expected_diagnostics=(
                    DiagnosticExpectation(code=2000, line=42, column=7, has_file=True),
                ),
            )

            def fake_run(cmd, _timeout, env=None):
                json_path = Path(cmd[cmd.index("--emit-diags-json") + 1])
                json_path.write_text(
                    json.dumps(
                        {
                            "diagnostics": [
                                {"code": 2000, "line": 1, "column": 1, "has_file": False},
                                {"code": 1000, "line": 42, "column": 7, "has_file": True},
                            ]
                        }
                    ),
                    encoding="utf-8",
                )
                return 1, "", False

            with patch("lib.runner.run_cmd", side_effect=fake_run):
                status, summary, _ = run_diag_json_probe(probe, Path("/unused/fisics"))
        self.assertEqual(status, "BLOCKED")
        self.assertIn("atomic", summary)

    def test_positive_json_accepts_one_complete_atomic_record(self):
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp) / "atomic_valid.c"
            source.write_text("int main(void) { return 0; }\n", encoding="utf-8")
            probe = DiagnosticJsonProbe(
                probe_id="contract__diagjson_atomic_valid",
                source=source,
                note="contract canary",
                expected_diagnostics=(
                    DiagnosticExpectation(code=2000, line=42, column=7, has_file=True),
                ),
            )

            def fake_run(cmd, _timeout, env=None):
                json_path = Path(cmd[cmd.index("--emit-diags-json") + 1])
                json_path.write_text(
                    json.dumps(
                        {
                            "diagnostics": [
                                {"code": 2000, "line": 42, "column": 7, "has_file": True}
                            ]
                        }
                    ),
                    encoding="utf-8",
                )
                return 1, "", False

            with patch("lib.runner.run_cmd", side_effect=fake_run):
                status, summary, _ = run_diag_json_probe(probe, Path("/unused/fisics"))
        self.assertEqual(status, "RESOLVED")
        self.assertIn("1 item", summary)

    def test_positive_json_rejects_crash_like_exit_even_with_valid_payload(self):
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp) / "json_crash.c"
            source.write_text("int main(void) { return 0; }\n", encoding="utf-8")
            probe = DiagnosticJsonProbe(
                probe_id="contract__diagjson_crash_exit",
                source=source,
                note="contract canary",
                expected_codes=(2000,),
            )

            def fake_run(cmd, _timeout, env=None):
                json_path = Path(cmd[cmd.index("--emit-diags-json") + 1])
                json_path.write_text(
                    json.dumps({"diagnostics": [{"code": 2000}]}),
                    encoding="utf-8",
                )
                return 70, "internal crash", False

            with patch("lib.runner.run_cmd", side_effect=fake_run):
                status, summary, _ = run_diag_json_probe(probe, Path("/unused/fisics"))
        self.assertEqual(status, "BLOCKED")
        self.assertIn("unexpected compile exit", summary)

    def test_positive_json_preserves_expected_code_multiplicity(self):
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp) / "multiplicity.c"
            source.write_text("int main(void) { return 0; }\n", encoding="utf-8")
            probe = DiagnosticJsonProbe(
                probe_id="contract__diagjson_multiplicity",
                source=source,
                note="contract canary",
                expected_codes=(2000, 2000),
            )

            def fake_run(cmd, _timeout, env=None):
                json_path = Path(cmd[cmd.index("--emit-diags-json") + 1])
                json_path.write_text(
                    json.dumps({"diagnostics": [{"code": 2000}]}),
                    encoding="utf-8",
                )
                return 1, "", False

            with patch("lib.runner.run_cmd", side_effect=fake_run):
                status, summary, _ = run_diag_json_probe(probe, Path("/unused/fisics"))
        self.assertEqual(status, "BLOCKED")
        self.assertIn("missing expected code", summary)

    def test_main_returns_nonzero_when_any_probe_is_blocked(self):
        probe = DiagnosticProbe(
            probe_id="contract__main_blocked",
            source=Path("/unused/source.c"),
            note="contract canary",
        )
        staged = SimpleNamespace(
            resolved_path=Path("/unused/fisics"),
            staged_path=Path("/unused/staged-fisics"),
            used_fallback=False,
            cleanup=Mock(),
        )
        with patch("lib.runner.stage_bin_copy", return_value=staged), \
             patch("lib.runner.RUNTIME_PROBES", ()), \
             patch("lib.runner.DIAG_PROBES", (probe,)), \
             patch("lib.runner.DIAG_JSON_PROBES", ()), \
             patch("lib.runner.parse_probe_filters", return_value=()), \
             patch("lib.runner.run_diag_probe", return_value=("BLOCKED", "canary", "")), \
             patch("lib.runner.emit_probe_blocked_classification"):
            result = main()
        self.assertEqual(result, 1)
        staged.cleanup.assert_called_once_with()


def load_tests(loader, tests, pattern):
    tests.addTests(loader.loadTestsFromName("test_inventory_registry_contract"))
    return tests


if __name__ == "__main__":
    unittest.main()
