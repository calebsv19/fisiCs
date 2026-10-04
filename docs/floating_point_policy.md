# Floating-point evaluation and compiler comparisons

fisiCs evaluates ordinary `float` and `double` arithmetic with separate LLVM
operations at their resolved types. Its existing default disables implicit
multiply-add/subtract contraction. The CLI accepts `-ffp-contract=off` as an
explicit assertion of that policy. `on`, `fast`, `fast-honor-pragmas`, empty,
and unrecognized contraction modes are rejected with exit status 1.

This option preserves existing results. It does not enable a new optimizer,
change literal precision, or replace the arithmetic model. Explicit `fma` and
`fmaf` library calls still request fused arithmetic. The physics-units overlay
checks dimensions and units while retaining the ordinary arithmetic policy.

## Compare under matched policies

For an ordinary binary32/binary64 workload on the same target, start with:

```bash
clang -std=c99 -O2 -fno-fast-math -ffp-contract=off input.c -lm -o clang.out
FISICS_LLVM_OPT_LEVEL=2 ./fisics -ffp-contract=off input.c -lm -o fisics.out
```

Match target, headers, linked libraries, rounding environment, and input bytes
as well. `FISICS_LLVM_OPT_LEVEL` selects LLVM optimization level 0 through 3;
the default is 0. The explicit non-contraction policy remains the same at each
level. An ordinary C `-O2` flag is not the fisiCs optimization selector.

Clang normally permits contraction within an expression. It can replace a
multiply followed by addition with one fused multiply-add, avoiding the
intermediate rounding. Both results can be legal, yet differ in their final
bits. See the [Clang floating-point documentation](https://clang.llvm.org/docs/UsersManual.html#controlling-floating-point-behavior)
and [LLVM floating-point flags](https://llvm.org/docs/LangRef.html#fast-math-flags).
LLVM `contract` flags or `llvm.fmuladd` intrinsics authorize transformations
that ordinary unflagged `fmul`/`fadd` operations do not.

fisiCs currently warns that `#pragma STDC FP_CONTRACT` and `FENV_ACCESS` are
ignored. Use the CLI policy rather than a pragma to establish this comparison.
Dynamic rounding modes, floating-point exception behavior, fast-math,
long-double/excess-precision configurations, other targets, and differing libm
implementations are outside the exact-parity evidence below. This is not a
universal promise of bit identity across unspecified C compiler settings.

## Known-answer regression

The stable runtime manifest is
`tests/final/meta/14-runtime-surface-fp-contract-off.json`:

```bash
make final-manifest MANIFEST=14-runtime-surface-fp-contract-off.json
```

It checks default and explicit policies, optimization levels 0/2/3, and a
portable units-annotated control. Independent binary32/binary64 answers cover
addition/subtraction around a rounded product, assignment and compound
assignment, explicit FMA, integer conversion, narrowing, and division. IR
checks forbid contraction/fast flags and implicit `llvm.fmuladd`.

A reduced Fire seed expression exposed the policy distinction. With input
bits `type=0x3f16f736`, `patch=0x3f7594d7`, `swirl=0x3d4ea668`:

```c
float first = type * 0.45f + patch * 0.35f;
float mix = type * 0.45f + patch * 0.35f + swirl * 0.20f;
```

| Value | Separate operations | Clang contraction enabled |
|---|---|---|
| `first` | `0x3f19e356` | `0x3f19e357` |
| `mix` | `0x3f1c789e` | `0x3f1c789f` |

The float constants and operand types match in both IRs. Clang emits
`llvm.fmuladd.f32` with contraction enabled; fisiCs and Clang with contraction
disabled emit separate `fmul float` and `fadd float`. At the first Fire cell,
all earlier seed intermediates agree; the first weighted sum diverges.
The canonical 8x8 seed-42 fixture has 25 fuel, 29 fuel-type, and 15 moisture
nodes differing under unmatched defaults. Even from identical initial state,
the unmatched policies produce different evolved states after 100 ticks.
Disabling contraction in Clang closes both differences without a lowering
change or any alteration to the source model.

## External read-only Fire canary

The optional runner requires a separate clean GrowthSim reference checkout at
`4890c632fb45d03e314b5be4ca7612b2716e01ff` and its existing shared libraries.
It copies library inputs to the requested output directory, compiles fresh
workers there, hashes source/header/adapter/library inputs, and verifies that
reference source and compiler identity remain unchanged. It never builds in
that checkout or invokes installation or stream migration.

```bash
PYTHONDONTWRITEBYTECODE=1 python3 tests/real_projects/runners/run_growth_fire_fp_parity.py \
  --growth-root /path/to/read-only/growth_sim \
  --fisics ./fisics \
  --clang clang \
  --output build/fire-fp-parity
```

On arm64 Darwin, the 2026-10-04 pass established exact seeded state, evolved
state, native observations, and source-accounting cells/totals/diagnostics
with Homebrew Clang 22.1.8 (LLVM 22.1.8), Apple Clang 17.0.0
(`clang-1700.6.4.2`), and fisiCs 0.7.0 plus this explicit-policy change.
All workers link the same frozen `core_space`, `core_units`, `core_base`
archives and macOS `libSystem`/libm. Each matched compiler runs at O0/O2/O3
with contraction off and no fast-math, source precision, and the default
round-to-nearest environment.

Cases are 8x8/42, 2x2/0, 8x8/1, 13x9/1024, 17x16/3735928559,
and 9x13/4294967295, with cell size 0.1 m, dt 1/60 s and zero wind.
Checks include an identical-input 100-tick step, 1+99 split continuation,
and independently seeded 100+100 ticks. The runner emits commands, identities,
input hashes, state/accounting hashes, totals, and unmatched-policy controls
in `verification.json`. These are authored source-model accounting values,
not physical combustion/CFD measurements. Exact continuation still requires
pinned worker and adapter bytes; this comparison grants no stream migration.

## Literal conversion

Floating literals are converted from their complete spelling at their resolved
semantic precision through LLVM's direct decimal/hexadecimal parser. Trailing
`f/F`, `l/L` and imaginary suffixes are removed without removing hexadecimal
`f/F` digits. Runtime values, direct scalar/aggregate/static initializers and
floating leaves of folded initializers share this conversion. Literal spellings
are not truncated to a fixed buffer.

For example, `0x1.fp0f` has binary32 bits `0x3ff80000`. The decimal
`1.0000000596046448f` rounds directly to `0x3f800001`; the unsuffixed double
literal followed by conversion to float yields `0x3f800000`. The distinction
between literal type and destination type is retained, including a float literal
widened to double. The decimal nearest-rounding choice is a documented policy;
it does not imply that every C implementation must choose the same neighboring
decimal value.

Run `make final-manifest MANIFEST=14-runtime-surface-float-literal-precision.json`
for known bits and Clang comparisons at optimization levels 0/2/3. General
constant-expression precision, dynamic rounding, and cross-target long-double
behavior require their own evidence beyond these literal conversion checks.

Imaginary and precision suffixes are accepted in both orders (`1.0fi` and
`1.0iF`, including the system `complex.h` spelling). Semantic analysis, codegen
type resolution and direct conversion use the same suffix decoder, preserving
the declared float/double/long-double component precision. The known-answer
regression covers both orders, uppercase/J variants, imaginary decimal rounding,
and component type sizes.

Mixed real/complex arithmetic selects component precision from both operands.
Complex operands are widened component by component before arithmetic, rather
than reinterpreting a float pair as a double pair. The literal regression also
covers both operand orders, multiplication and division with mixed precision.
