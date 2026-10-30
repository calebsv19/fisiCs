#!/usr/bin/env python3
"""Select and report the supported analysis toolchain before source export."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess

from provenance import ProvenanceError, require, sha


def analysis_environment():
    selected = shutil.which('llvm-config')
    require(selected is not None, 'llvm-config is required for the optional developer workflow')
    llvm = Path(selected).resolve()
    version = subprocess.check_output([str(llvm), '--version'], text=True).strip()
    require(version.split('.', 1)[0] == '22',
            'reviewed matrix-flow analysis requires LLVM 22; select its llvm-config explicitly in PATH')
    bindir = Path(subprocess.check_output([str(llvm), '--bindir'], text=True).strip()).resolve()
    clang = (bindir / 'clang').resolve()
    require(clang.is_file(), 'matching LLVM analysis clang is missing: ' + str(clang))
    resource = Path(subprocess.check_output([str(clang), '-print-resource-dir'], text=True).strip()).resolve()
    require(resource.name == '22', 'analysis resource does not match reviewed LLVM 22 headers')
    env = dict(os.environ)
    env['PATH'] = str(bindir) + os.pathsep + env.get('PATH', os.defpath)
    require(Path(shutil.which('clang', path=env['PATH'])).resolve() == clang,
            'analysis clang differs from selected LLVM toolchain')
    sdk = Path(subprocess.check_output(['xcrun', '--show-sdk-path'], env=env, text=True).strip()).resolve()
    config = dict(schema='fisiCs.experimental.analysis_configuration.v1',
                  llvm_config=str(llvm), llvm_config_sha256=sha(llvm), llvm_version=version,
                  clang=str(clang), clang_sha256=sha(clang), resource=str(resource), sdk=str(sdk),
                  selection='matching clang from the explicitly selected LLVM 22 installation')
    return env, config


def main():
    argparse.ArgumentParser(description=__doc__).parse_args()
    try:
        _, config = analysis_environment()
        print(json.dumps(dict(passed=True, analysis_configuration=config), indent=2))
        return 0
    except (OSError, ValueError, subprocess.SubprocessError) as error:
        print(json.dumps(dict(passed=False, diagnostic=str(error)), indent=2))
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
