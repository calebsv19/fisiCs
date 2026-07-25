#!/usr/bin/env python3
"""Invariant verifier for OS-P x86_64-unknown-none objects."""

from __future__ import annotations

import re
import subprocess
from pathlib import Path
from typing import Any


def _run(*args: str) -> str:
    completed = subprocess.run(args, check=True, text=True, capture_output=True)
    return completed.stdout


def _symbol_blocks(symbol_output: str) -> list[str]:
    return re.findall(r"  Symbol \{\n(.*?)\n  \}", symbol_output, re.DOTALL)


def _named_global_symbols(symbol_output: str) -> tuple[set[str], set[str]]:
    defined: set[str] = set()
    undefined: set[str] = set()
    for block in _symbol_blocks(symbol_output):
        name_match = re.search(
            r"^    Name: ([A-Za-z_][A-Za-z0-9_]*) ",
            block,
            re.MULTILINE,
        )
        if not name_match or "Binding: Global" not in block:
            continue
        name = name_match.group(1)
        if "Section: Undefined" in block:
            undefined.add(name)
        else:
            defined.add(name)
    return defined, undefined


def _instruction_mnemonics(disassembly: str) -> set[str]:
    return set(
        re.findall(
            r"^\s*[0-9a-f]+:\s+([a-z][a-z0-9]*)\b",
            disassembly,
            re.MULTILINE,
        )
    )


def verify_object(
    *,
    object_path: Path,
    readobj: Path,
    objdump: Path,
    contract: dict[str, Any],
) -> dict[str, Any]:
    data = object_path.read_bytes()
    if len(data) < 20 or data[:4] != b"\x7fELF":
        raise RuntimeError("object does not have ELF magic")
    if data[4] != 2 or data[5] != 1:
        raise RuntimeError("object is not ELF64 little-endian")
    if int.from_bytes(data[18:20], "little") != 0x3E:
        raise RuntimeError("ELF e_machine is not EM_X86_64")

    header = _run(str(readobj), "--file-headers", str(object_path))
    for expected in (
        "Format: elf64-x86-64",
        "Arch: x86_64",
        "Class: 64-bit",
        "DataEncoding: LittleEndian",
        "OS/ABI: SystemV",
        "Type: Relocatable",
        "Machine: EM_X86_64",
    ):
        if expected not in header:
            raise RuntimeError(f"missing ELF header fact: {expected}")

    symbol_output = _run(str(readobj), "--symbols", str(object_path))
    defined, undefined = _named_global_symbols(symbol_output)
    required_exports = set(contract.get("required_exports", []))
    missing_exports = sorted(required_exports - defined)
    if missing_exports:
        raise RuntimeError(f"missing required exports: {missing_exports}")

    allowed_undefined = set(contract.get("allowed_undefined", []))
    unexpected_undefined = sorted(undefined - allowed_undefined)
    missing_undefined = sorted(allowed_undefined - undefined)
    if unexpected_undefined or missing_undefined:
        raise RuntimeError(
            "undefined symbol contract mismatch: "
            f"unexpected={unexpected_undefined} missing={missing_undefined}"
        )

    relocation_output = _run(str(readobj), "--relocations", str(object_path))
    relocations = set(re.findall(r"\b(R_X86_64_[A-Z0-9_]+)\b", relocation_output))
    allowed_relocations = set(contract.get("allowed_relocations", []))
    unexpected_relocations = sorted(relocations - allowed_relocations)
    if unexpected_relocations:
        raise RuntimeError(f"unexpected relocations: {unexpected_relocations}")

    disassembly = _run(
        str(objdump),
        "-d",
        "--no-show-raw-insn",
        str(object_path),
    )
    mnemonics = _instruction_mnemonics(disassembly)
    forbidden = set(contract.get("forbidden_instructions", []))
    present_forbidden = sorted(mnemonics & forbidden)
    if present_forbidden:
        raise RuntimeError(f"forbidden instructions present: {present_forbidden}")

    if contract.get("forbid_red_zone", False):
        if re.search(r"-0x[0-9a-f]+\(%rsp\)", disassembly):
            raise RuntimeError("negative rsp-relative access indicates red-zone use")

    return {
        "defined_global_symbols": sorted(defined),
        "undefined_global_symbols": sorted(undefined),
        "relocations": sorted(relocations),
        "instruction_mnemonics": sorted(mnemonics),
        "required_exports": sorted(required_exports),
        "forbidden_instructions": sorted(forbidden),
        "red_zone_forbidden": bool(contract.get("forbid_red_zone", False)),
    }
