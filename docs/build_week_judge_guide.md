# OpenAI Build Week Judge Guide

This is the shortest installation and testing path for the `fisiCs` 0.5.0
OpenAI Build Week submission. It is designed to let a judge run the compiler
without rebuilding the project or setting up the full development test suite.

The recorded compiler demonstration used the immutable `0.4.0` package. The
commands below follow the current `0.5.0` release. Archive filenames and
SHA-256 values change with each release, so verify the checksum published for
the exact archive you download rather than reusing the value visible in the
video.

## Supported Platform

The submission release is tested and packaged for:

- macOS on Apple Silicon (`arm64`)
- a POSIX shell (`zsh` or `bash`)
- the Apple command-line linker/toolchain available through Xcode Command Line
  Tools

The source tree is written for POSIX environments and its archive tooling also
has Linux paths, but Linux and Windows are not claimed as judge-supported
binary platforms for this submission. The public CI release guardrail runs on
GitHub's macOS runner with Homebrew LLVM.

## Install Without Rebuilding

1. Open the [fisiCs program page](https://ecosystem.calebsv.tech/suite/program/?repo=fisiCs).
2. Download the macOS `arm64` 0.5.0 `.tar.gz` or `.zip` archive.
3. Compare the archive's SHA-256 with the checksum shown on the program page.
4. Extract the archive. No installer or administrator access is required.

Terminal example for a downloaded tarball:

```bash
shasum -a 256 fisiCs-0.5.0-macOS-arm64-stable.tar.gz
tar -xzf fisiCs-0.5.0-macOS-arm64-stable.tar.gz
cd fisiCs-0.5.0-macOS-arm64-stable
bin/fisics --version
bin/fisics --help
```

Expected version output includes `0.5.0`. The archive includes the compiler,
documentation, examples, and the small compile/link fixtures used below.

## Five-Minute Test

Run these commands from the extracted archive root:

```bash
mkdir -p build/judge
bin/fisics examples/hello_world.c -o build/judge/hello_world
build/judge/hello_world

bin/fisics \
  compilation/multi_main.c compilation/multi_helper.c \
  -o build/judge/multi_file
build/judge/multi_file
```

Expected output:

```text
Hello from fisiCs.
multi-file smoke: helper=7 total=12
```

To test the opt-in physics-units extension and its tooling output:

```bash
bin/fisics --overlay=physics-units --dump-sema \
  -c examples/physics_units/ballistics_valid.c \
  -o build/judge/ballistics_valid.o
```

The command should complete successfully and print semantic metadata containing
physics-unit information. Ordinary C compilation remains the default; the
extension activates only when its flag is present.

## Optional Source Build

The prebuilt archive above is the supported judge path. To inspect or rebuild
the complete repository instead, install `clang`, `make`, and LLVM with
`llvm-config` on `PATH`, then run:

```bash
git clone https://github.com/calebsv19/fisiCs.git
cd fisiCs
make
./fisics --version
./compilation/run_single.sh ./fisics
./compilation/run_multi.sh ./fisics
make examples-canaries
```

For the curated demo project and expected diagnostics, continue with the
[release example pack](../examples/release_example_pack.md).

## What The Submission Demonstrates

- a C99-oriented compiler hardened toward broader C17-compatible behavior
- single-file and multi-translation-unit compilation and linking
- deterministic examples and practical libc/math canaries
- opt-in physics-units diagnostics and semantic metadata
- source-level build-graph output for IDE and agent workflows
- Clang-versus-`fisiCs` operational differential validation against real
  CodeWork programs

The project is experimental and does not claim complete C17 or cross-platform
ABI conformance. The exact public boundary is documented in the
[supported feature matrix](supported_feature_matrix.md).

## Codex Evidence

The principal `/feedback` Session ID for the reusable Stage-G operational
differential work is:

```text
019f6486-6fc5-79b2-91dd-4ecab0b34118
```

That GPT-5.6 Codex thread exercised the reusable runner through RayTracing's
four production-shaped workflow bites, repeated each Clang and `fisiCs` path,
verified trace and artifact parity, and then ran the monitored final and
promotion-audit gates. The repository's Build Week evidence boundary starts
after commit `aa4b3268ce552fd3ead88fa9b6bd8df52842b3df`, includes the stabilized compiler
checkpoint `3256df3555af09772a41079dd9357ac0120e7ba2`, and includes the immutable
`v0.4.0` demonstration release. The `0.5.0` release adds the separately tested
freestanding x86-64 object-emission capability while preserving that historical
release and its checksums.
