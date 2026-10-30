# Finite local mutable physical storage

With the optional `physics-semantics` overlay and an explicit v7 policy, the
compiler can retain physical provenance through explicit local array writes.
This uses ordinary C syntax and adds no runtime metadata or ABI changes.

```c
#include <fisics/extensions.h>
#define E FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.energy")
#define Q FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.torque")
E double select_energy(double energy E, double torque Q) {
    const unsigned map[2] = {1, 0};
    double values[2];
    values[map[0]] = energy - energy;
    values[map[0]] += energy;
    values[map[1]] = torque;
    return values[map[0]];
}
```

Each read uses the preceding write to its exact resolved C object and element.
The first assignment needs physical provenance from its actual right-hand
expression. A subsequent assignment checks the established slot role;
accumulation checks both the previous value and the new operand. A typed
subtraction such as `energy - energy` supplies physical zero. Bare `0` does not
establish a kind, and literal-assumption permission cannot repair an untyped
mutable overwrite. Annotated destinations cannot restore missing provenance.
Declared producer results remain interface trust, require explicit permission,
and do not satisfy the separate nonvacuous checked-obligation gate alone.

The finite path admits uninitialized local non-static, non-volatile `double`
arrays of rank one or two, at most 729 elements and 32 candidate roots per
function. Initializer-based mutable arrays remain outside this rule; immutable
arrays retain their [separate snapshot contract](physical_storage_snapshots.md).
Existing homogeneous annotated arrays keep their existing checking contract.

Indices are unsuffixed nonnegative decimal literals or finite `+`, `-`, `*`
expressions whose intermediate results stay in 0..4096 and final offsets are
in range. A map may be a complete positional local `const unsigned int` array
of at most 64 decimal entries, used only in array indices through literal map
indices. Legal repeated map entries retain accumulation checks; a collision
between incompatible physical roles is rejected. Map values describe storage
positions, not quantities or proof of a model's DOF layout.

Only straight-line statements and nested lexical blocks are admitted. Branches,
loops, labels, nested assignments, conditional expressions and unsupported
statements refuse this proof. There is no inferred join, loop induction or
interprocedural mutation summary. Every use of an admitted root must be an
exact scalar access. Taking an address, escaping an array/row, unsupported
indices or possible alias writes refuse the root, including an escape after an
earlier read. Calls cannot recover an escaped root. `memcpy`/`memset`, pointer
outputs, fields reached through owner pointers, VLA, static/global storage and
arbitrary dynamic DOF maps are not admitted.

Preparation is limited to 4096 expressions and finite index recursion. An
incomplete or oversized traversal supplies no mutable facts. Functions with
any resolved concrete-unit expression refuse this rule, including equal-unit
cases, to avoid erasing scale identity. Use existing explicit unit checking
and conversions at those boundaries.

Arrays, map construction and index arithmetic are checked structural
relationships; they never become homogeneous physical quantities. Required
coverage obligations remain present. Checked connections, authored scalar
inputs, literal/interface trust and unknowns stay distinct. Compiler acceptance
does not prove coefficient accuracy, matrix assembly from a model, conservation,
solver stability or physical acceptance. In particular, a checked finite local
assembly specialization does not establish the original pointer-owned,
loop-driven, cross-function solver storage path.

Run the stable contract gate from a source build:

```sh
python3 -B tests/integration/run_physical_mutable_storage.py \
  --compiler ./fisics --out /tmp/fisics-mutable-storage-check
```

Use a fresh output directory. The gate checks valid, contradictory, unknown,
trust, resource-limit and unit-refusal paths, plus ordinary C runtime behavior
with overlays off, units-only, physical checking and Clang. Actual solver
connections and numerical acceptance require separate evidence.

Multi-digit integer tokens beginning with zero are refused by this finite proof: C interprets them as octal. Decimal provenance must not select a different slot from the runtime C index.
