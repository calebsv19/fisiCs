import tempfile
import unittest
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))

from run_project_operational_differential_tests import (
    build_run_environment,
    expand_target_inputs,
    select_targets,
    stage_runtime_fixtures,
)


class OperationalDifferentialRunnerContractTests(unittest.TestCase):
    def test_zero_selection_fails_closed(self):
        with self.assertRaisesRegex(ValueError, "zero Stage-G targets selected"):
            select_targets([{"id": "bite_1"}], "missing")

    def test_input_globs_are_sorted_and_deduplicated(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            sources = root / "src"
            sources.mkdir()
            first = sources / "a.c"
            second = sources / "b.c"
            first.write_text("int a;\n", encoding="utf-8")
            second.write_text("int b;\n", encoding="utf-8")
            expanded = expand_target_inputs(
                root,
                {"inputs": ["src/b.c"], "input_globs": ["src/*.c"]},
            )
            self.assertEqual(expanded, [second.resolve(), first.resolve()])

    def test_input_exclusions_apply_after_glob_expansion(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            sources = root / "src"
            generated = sources / "generated"
            generated.mkdir(parents=True)
            keep = sources / "keep.c"
            main = sources / "main.c"
            generated_source = generated / "table.c"
            for path in (keep, main, generated_source):
                path.write_text("int value;\n", encoding="utf-8")
            expanded = expand_target_inputs(
                root,
                {
                    "input_globs": ["src/**/*.c"],
                    "exclude_inputs": ["src/main.c"],
                    "exclude_globs": ["src/generated/*.c"],
                },
            )
            self.assertEqual(expanded, [keep.resolve()])

    def test_input_expansion_can_report_zero_for_compile_fail_closed(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.assertEqual(
                expand_target_inputs(root, {"input_globs": ["src/**/*.c"]}),
                [],
            )

    def test_missing_fixture_fails_closed(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            with self.assertRaisesRegex(FileNotFoundError, "runtime fixture missing"):
                stage_runtime_fixtures(
                    root,
                    [{"source": "missing.json", "path": "fixtures/input.json"}],
                    root / "run",
                )

    def test_fixture_traversal_fails_closed(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root / "input.json"
            source.write_text("{}\n", encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "unsafe runtime fixture destination"):
                stage_runtime_fixtures(
                    root,
                    [{"source": str(source), "path": "../escape.json"}],
                    root / "run",
                )

    def test_duplicate_fixture_destination_fails_closed(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            first = root / "first.json"
            second = root / "second.json"
            first.write_text("1\n", encoding="utf-8")
            second.write_text("2\n", encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "duplicate runtime fixture destination"):
                stage_runtime_fixtures(
                    root,
                    [
                        {"source": str(first), "path": "fixtures/input.json"},
                        {"source": str(second), "path": "fixtures/input.json"},
                    ],
                    root / "run",
                )

    def test_environment_prefix_is_scrubbed_before_explicit_values(self):
        environment, applied = build_run_environment(
            {"PATH": "/bin", "MAPFORGE_LEAK": "bad", "OTHER": "kept"},
            ["MAPFORGE_"],
            {
                "MAPFORGE_RUNTIME_DIR": "{run_root}/runtime",
                "PROJECT": "{project_root}",
            },
            Path("/project"),
            Path("/run"),
        )
        self.assertNotIn("MAPFORGE_LEAK", environment)
        self.assertEqual(environment["OTHER"], "kept")
        self.assertEqual(environment["MAPFORGE_RUNTIME_DIR"], "/run/runtime")
        self.assertEqual(applied["PROJECT"], "/project")

    def test_fixture_mutation_is_isolated_between_runs(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root / "source.txt"
            source.write_text("original\n", encoding="utf-8")
            fixture = [{"source": str(source), "path": "fixtures/source.txt"}]
            first_run = root / "run_1"
            second_run = root / "run_2"
            stage_runtime_fixtures(root, fixture, first_run)
            (first_run / "fixtures/source.txt").write_text("mutated\n", encoding="utf-8")
            stage_runtime_fixtures(root, fixture, second_run)
            self.assertEqual(source.read_text(encoding="utf-8"), "original\n")
            self.assertEqual(
                (second_run / "fixtures/source.txt").read_text(encoding="utf-8"),
                "original\n",
            )


if __name__ == "__main__":
    unittest.main()
