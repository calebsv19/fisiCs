#!/usr/bin/env python3

import ast
import unittest
from collections import Counter
from pathlib import Path


INVENTORY_DIR = Path(__file__).resolve().parent / "inventory"
REGISTRY_PATH = INVENTORY_DIR / "registry.py"
PROBE_LIST_NAMES = (
    "RUNTIME_PROBES",
    "DIAG_PROBES",
    "DIAG_JSON_PROBES",
)


def inventory_sources():
    sources = {
        path.stem: path
        for path in INVENTORY_DIR.glob("bucket_*.py")
    }
    for package in INVENTORY_DIR.glob("bucket_*"):
        if not package.is_dir():
            continue
        init_path = package / "__init__.py"
        if not init_path.is_file():
            raise AssertionError(f"inventory package lacks __init__.py: {package.name}")
        sources[package.name] = init_path
    return sources


def declared_probe_lists(path):
    declarations = {}
    for node in ast.parse(path.read_text(encoding="utf-8"), filename=str(path)).body:
        if not isinstance(node, (ast.Assign, ast.AnnAssign)):
            continue
        targets = node.targets if isinstance(node, ast.Assign) else (node.target,)
        for target in targets:
            if not isinstance(target, ast.Name) or target.id not in PROBE_LIST_NAMES:
                continue
            if target.id in declarations:
                raise AssertionError(f"duplicate {target.id} assignment in {path.name}")
            value = node.value
            literal_empty = isinstance(value, (ast.List, ast.Tuple)) and not value.elts
            declarations[target.id] = not literal_empty
    return declarations


def registry_imports(tree):
    imported = []
    for node in tree.body:
        if isinstance(node, ast.ImportFrom) and node.level == 1 and node.module is None:
            imported.extend(alias.name for alias in node.names)
    return imported


def aggregate_references(tree, probe_list_name):
    assignments = [
        node
        for node in tree.body
        if isinstance(node, ast.Assign)
        and any(
            isinstance(target, ast.Name) and target.id == probe_list_name
            for target in node.targets
        )
    ]
    if len(assignments) != 1:
        raise AssertionError(
            f"registry must assign {probe_list_name} exactly once; found {len(assignments)}"
        )
    return [
        node.value.id
        for node in ast.walk(assignments[0].value)
        if isinstance(node, ast.Attribute)
        and node.attr == probe_list_name
        and isinstance(node.value, ast.Name)
    ]


class InventoryRegistryContractTests(unittest.TestCase):
    def test_nonempty_top_level_modules_and_packages_are_reachable_once(self):
        sources = inventory_sources()
        declarations = {
            module_name: declared_probe_lists(path)
            for module_name, path in sources.items()
        }
        active_modules = {
            module_name
            for module_name, probe_lists in declarations.items()
            if any(probe_lists.values())
        }

        registry_tree = ast.parse(
            REGISTRY_PATH.read_text(encoding="utf-8"),
            filename=str(REGISTRY_PATH),
        )
        imports = registry_imports(registry_tree)
        import_counts = Counter(imports)

        self.assertEqual(set(imports), active_modules)
        self.assertEqual(
            sorted(name for name, count in import_counts.items() if count != 1),
            [],
        )

        for probe_list_name in PROBE_LIST_NAMES:
            references = aggregate_references(registry_tree, probe_list_name)
            reference_counts = Counter(references)
            with self.subTest(probe_list=probe_list_name):
                self.assertEqual(
                    sorted(name for name, count in reference_counts.items() if count != 1),
                    [],
                )
                self.assertEqual(set(references) - set(imports), set())
                self.assertEqual(
                    sorted(
                        module_name
                        for module_name, probe_lists in declarations.items()
                        if probe_lists.get(probe_list_name, False)
                        and reference_counts[module_name] != 1
                    ),
                    [],
                )
                self.assertEqual(
                    sorted(
                        module_name
                        for module_name in references
                        if probe_list_name not in declarations.get(module_name, {})
                    ),
                    [],
                )


if __name__ == "__main__":
    unittest.main()
