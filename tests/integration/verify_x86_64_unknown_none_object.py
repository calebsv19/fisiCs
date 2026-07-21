#!/usr/bin/env python3
"""Verify the stable fisiCs x86_64-unknown-none object contract."""

import re
import subprocess
import sys
from pathlib import Path


def run(*args: str) -> str:
    result = subprocess.run(args, check=True, text=True, capture_output=True)
    return result.stdout


def fail(message: str) -> None:
    raise SystemExit(f"x86_64-unknown-none object verification failed: {message}")


def function_body(disassembly: str, name: str) -> str:
    match = re.search(
        rf"^[0-9a-f]+ <{re.escape(name)}>:\n(.*?)(?=\n[0-9a-f]+ <|\Z)",
        disassembly,
        re.MULTILINE | re.DOTALL,
    )
    if not match:
        fail(f"missing disassembly for {name}")
    return match.group(1)


def main() -> int:
    if len(sys.argv) != 4:
        fail("usage: verify_x86_64_unknown_none_object.py <readobj> <objdump> <object>")

    readobj, objdump, object_arg = sys.argv[1:]
    object_path = Path(object_arg)
    data = object_path.read_bytes()
    if len(data) < 20 or data[:4] != b"\x7fELF":
        fail("object does not have ELF magic")
    if data[4] != 2 or data[5] != 1:
        fail("object is not ELF64 little-endian")
    if int.from_bytes(data[18:20], "little") != 0x3E:
        fail("ELF e_machine is not EM_X86_64")

    header = run(readobj, "--file-headers", str(object_path))
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
            fail(f"missing header fact: {expected}")

    section_output = run(readobj, "--sections", str(object_path))
    sections = []
    for line in section_output.splitlines():
        stripped = line.strip()
        if stripped.startswith("Name:"):
            sections.append(stripped[5:].split(" (", 1)[0].strip())
    expected_sections = [
        "",
        ".strtab",
        ".text",
        ".rela.text",
        ".note.GNU-stack",
        ".eh_frame",
        ".rela.eh_frame",
        ".symtab",
    ]
    if sections != expected_sections:
        fail(f"unexpected section surface: {sections}")

    reloc_output = run(readobj, "--relocations", str(object_path))
    relocations = re.findall(r"\b(R_X86_64_[A-Z0-9_]+)\b", reloc_output)
    if len(relocations) != 4 or set(relocations) != {"R_X86_64_PLT32", "R_X86_64_PC32"}:
        fail(f"unexpected relocations: {relocations}")
    for symbol in ("edu08_leaf", "edu08_sink", ".text"):
        if symbol not in reloc_output:
            fail(f"missing expected relocation symbol: {symbol}")

    symbol_output = run(readobj, "--symbols", str(object_path))
    symbol_blocks = re.findall(r"  Symbol \{\n(.*?)\n  \}", symbol_output, re.DOTALL)
    global_symbols = set()
    undefined = set()
    for block in symbol_blocks:
        name_match = re.search(r"^    Name: ([A-Za-z_][A-Za-z0-9_]*) ", block, re.MULTILINE)
        if not name_match:
            continue
        name = name_match.group(1)
        global_symbols.add(name)
        if "Binding: Global" in block and "Section: Undefined" in block:
            undefined.add(name)
    expected_symbol_names = {"compiler_module", "edu08_leaf", "edu08_probe", "edu08_sink"}
    if global_symbols != expected_symbol_names:
        fail(f"unexpected named symbol surface: {sorted(global_symbols)}")
    if undefined != {"edu08_sink"}:
        fail(f"hidden runtime helper or missing sink: {sorted(undefined)}")

    disassembly = run(objdump, "-d", "--no-show-raw-insn", str(object_path))
    if "file format elf64-x86-64" not in disassembly:
        fail("objdump did not identify elf64-x86-64")
    leaf = function_body(disassembly, "edu08_leaf")
    probe = function_body(disassembly, "edu08_probe")
    if "retq" not in leaf or probe.count("callq") != 2 or "retq" not in probe:
        fail("unexpected x86-64 instruction surface")

    frame_match = re.search(r"subq\s+\$0x([0-9a-f]+), %rsp", leaf)
    if not frame_match:
        fail("leaf function has no explicit stack allocation")
    frame_bytes = int(frame_match.group(1), 16)
    if (8 - frame_bytes) % 16 != 0:
        fail(f"leaf stack is not 16-byte aligned: frame={frame_bytes}")
    if re.search(r"-0x[0-9a-f]+\(%rsp\)", disassembly):
        fail("negative rsp-relative access indicates red-zone use")
    stack_offsets = [int(value, 16) for value in re.findall(r"0x([0-9a-f]+)\(%rsp\)", leaf)]
    max_access_end = (max(stack_offsets) if stack_offsets else 0) + 8
    if max_access_end > frame_bytes:
        fail(f"leaf stack access exceeds allocation: access={max_access_end} frame={frame_bytes}")

    rsp_mod_16 = 8
    calls_checked = 0
    for line in probe.splitlines():
        instruction = line.split(":", 1)[-1].strip()
        if instruction.startswith("pushq"):
            rsp_mod_16 = (rsp_mod_16 - 8) % 16
        elif instruction.startswith("popq"):
            rsp_mod_16 = (rsp_mod_16 + 8) % 16
        else:
            adjust = re.match(r"(subq|addq)\s+\$0x([0-9a-f]+), %rsp", instruction)
            if adjust:
                amount = int(adjust.group(2), 16)
                rsp_mod_16 = (rsp_mod_16 + (-amount if adjust.group(1) == "subq" else amount)) % 16
        if instruction.startswith("callq"):
            calls_checked += 1
            if rsp_mod_16 != 0:
                fail(f"misaligned x86-64 call site: rsp mod 16 = {rsp_mod_16}")
    if calls_checked != 2:
        fail(f"expected two aligned call sites, found {calls_checked}")

    print("PASS header=ELF64-little-endian-systemv-relocatable machine=EM_X86_64")
    print("PASS sections=" + ",".join(name or "<null>" for name in sections))
    print("PASS relocations=count:4 allowed:R_X86_64_PLT32,R_X86_64_PC32")
    print("PASS symbols=defined:edu08_leaf,edu08_probe undefined:edu08_sink hidden_helpers:none")
    print("PASS disassembly=x86_64 leaf:ret probe:calls:2,ret")
    print(f"PASS stack_alignment=16 no_red_zone=true leaf_frame_bytes={frame_bytes} max_access_end={max_access_end}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
