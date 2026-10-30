#!/usr/bin/env python3
"""Build a task-owned native policy probe with ASan/UBSan or owned-allocation ledger.

The frontend archive supplies existing JSON/unit services. The catalog/model modules, plus optional P5.3 modules,
are rebuilt with instrumentation; no complete compiler sanitizer
claim is made. Scratch output belongs to the caller.
"""
import argparse
import pathlib
import shlex
import subprocess

ROOT=pathlib.Path(__file__).resolve().parents[2]
SOURCES=["src/Compiler/physical_catalog_io.c", "src/Compiler/physical_contract_catalog.c",
         "src/Extensions/PhysicalSemantics/physical_policy.c",
         "src/Extensions/PhysicalSemantics/physical_policy_profiles.c",
         "src/Extensions/PhysicalSemantics/physical_policy_model.c",
         "src/Extensions/PhysicalSemantics/physical_operations.c",
         "src/Extensions/PhysicalSemantics/physical_operation_receipt.c",
         "src/Extensions/PhysicalSemantics/physical_scalar_rules.c",
         "src/Extensions/PhysicalSemantics/physical_mechanical_rules.c",
         "src/Extensions/PhysicalSemantics/physical_structural_rules.c",
         "src/Extensions/PhysicalSemantics/physical_thermal_rules.c",
         "src/Extensions/PhysicalSemantics/physical_cosmic_rules.c",
    "src/Extensions/PhysicalSemantics/physical_cosmic_shapes.c",
         "src/Extensions/PhysicalSemantics/physical_cosmic_math_rules.c",
         "src/Extensions/PhysicalSemantics/physical_representations.c",
         "src/Extensions/PhysicalSemantics/physical_frames.c",
         "src/Extensions/PhysicalSemantics/physical_energy.c",
         "src/Extensions/PhysicalSemantics/physical_frame_transforms.c",
         "src/Extensions/PhysicalSemantics/physical_frame_model_receipt.c"]

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--library",type=pathlib.Path,required=True)
    p.add_argument("--out",type=pathlib.Path,required=True)
    p.add_argument("--mode",choices=["sanitized","ledger"],required=True)
    p.add_argument("--energy",action="store_true",help="Build energy model allocation/roundtrip contract")
    p.add_argument("--frame-flow",action="store_true",help="Build the frame propagation and strict coverage contract")
    p.add_argument("--frames",action="store_true",help="Build the frame model/declaration failure contract")
    p.add_argument("--representations",action="store_true",help="Build the representation model failure/lifetime contract")
    p.add_argument("--operations",action="store_true",help="Build the native operation-model lifetime/failure contract")
    p.add_argument("--cli",action="store_true",help="Link the selected build main/object emitter for the full P5.3 CLI contract")
    p.add_argument("--semantics",action="store_true",help="Build the P5.3 native state/lifetime contract instead of the catalog driver")
    a=p.parse_args();a.out=a.out.resolve();a.semantics=a.semantics or a.cli or a.frames or a.frame_flow;a.out.mkdir(parents=True,exist_ok=False)
    common=["cc","-g","-O1","-fno-omit-frame-pointer",*shlex.split(subprocess.check_output(["llvm-config","--cflags"],text=True)),*["-I"+x for x in ["src","src/Lexer","src/Parser","src/Syntax","src/Syntax/Expr","src/Syntax/Decls","src/CodeGen","src/CodeGen/Expr"]]]
    for name in ["core_base","core_data","core_io","core_pack"]:
        common.append("-Ithird_party/codework_shared/core/"+name+"/include")
    flags=["-fsanitize=address,undefined"] if a.mode=="sanitized" else []
    objects=[]
    sources=SOURCES+(["src/Extensions/PhysicalSemantics/physical_semantics_"+name+".c" for name in ["state","attributes","expr","affine","walk"]] if a.semantics else [])
    if a.semantics:sources += ["src/Extensions/PhysicalSemantics/physical_mutable_storage.c",
                               "src/Extensions/PhysicalSemantics/physical_storage_snapshots.c",
                               "src/Extensions/PhysicalSemantics/physical_constructors.c",
                               "src/Extensions/PhysicalSemantics/physical_frame_context.c",
                               "src/Extensions/PhysicalSemantics/physical_frame_semantics.c",
                               "src/Extensions/PhysicalSemantics/physical_point_semantics.c",
                               "src/Extensions/PhysicalSemantics/physical_energy_semantics.c",
                               "src/Extensions/PhysicalSemantics/physical_energy_transfer.c",
                               "src/Extensions/PhysicalSemantics/physical_frame_transform_semantics.c",
                               "src/Extensions/PhysicalSemantics/physical_frame_binding.c",
                               "src/Extensions/PhysicalSemantics/physical_frame_components.c",
                               "src/Extensions/PhysicalSemantics/physical_representation_binding.c",
                               "src/Extensions/PhysicalSemantics/physical_shape_semantics.c",
                               "src/Extensions/PhysicalSemantics/physical_math_semantics.c",
                               "src/Extensions/PhysicalSemantics/physical_scalar_semantics.c"]
    if a.semantics:sources += ["src/Extensions/PhysicalSemantics/physical_"+name+".c" for name in ["provenance","coverage","coverage_interfaces"]]
    common.append('-DFISICS_VERSION="'+(ROOT/"VERSION").read_text().strip()+'"')
    if a.cli:sources += ["src/main_physical.c","src/main_physical_coverage.c","src/Compiler/physical_receipt.c","src/Compiler/physical_receipt_signatures.c","src/Compiler/physical_receipt_check.c","src/Compiler/physical_frame_receipt.c","src/Compiler/physical_frame_receipt_check.c","src/Compiler/physical_energy_receipt.c"]
    for i,source in enumerate(sources):
        obj=a.out/f"module-{i}.o";objects.append(obj)
        # Attribute inheritance transfers cloned C attributes to existing parser
        # ownership. Ledger only the fully owned model/state allocations;
        # ASan/UBSan instruments all new implementation files.
        tracked=source in SOURCES or source.endswith(("physical_semantics_state.c","physical_frame_binding.c")) or (a.cli and (source.endswith("physical_provenance.c") or source.endswith("physical_coverage.c") or "/physical_receipt" in source))
        injection=["-include","tests/unit/physical_policy_allocation_probe.h"] if a.mode=="ledger" and tracked else []
        subprocess.run(common+flags+injection+["-c",source,"-o",str(obj)],cwd=ROOT,check=True)
    link=shlex.split(subprocess.check_output(["llvm-config","--ldflags","--libs","core","native","nativecodegen","x86codegen","passes"],text=True))
    ledger=["tests/unit/physical_policy_allocation_probe.c"] if a.mode=="ledger" else []
    driver=[str(a.library.resolve().parent/"main.o"),str(a.library.resolve().parent/"Compiler/object_emit.o")] if a.cli else ["tests/unit/physical_energy_contract.c" if a.energy else "tests/unit/physical_frame_flow_contract.c" if a.frame_flow else "tests/unit/physical_frames_contract.c" if a.frames else "tests/unit/physical_representations_contract.c" if a.representations else "tests/unit/physical_operations_contract.c" if a.operations else "tests/unit/physical_semantics_contract.c" if a.semantics else "tests/unit/physical_policy_loader.c"]
    subprocess.run(common+flags+[*driver,*map(str,objects),*ledger,
                              str(a.library.resolve()),*link,"-o",str(a.out/"probe")],cwd=ROOT,check=True)

if __name__=="__main__":main()
