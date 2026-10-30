# Finite population and external-field scalar contracts

Policy v7 admits two canonical linear kinds with different meanings:

| Kind | Dimensions | Meaning |
| --- | --- | --- |
| `quantity.specific_external_potential` | L²/T² | Fixed external gravitational potential per unit mass |
| `quantity.external_potential_energy` | ML²/T² | Active mass weighted contribution to external potential energy |

Both require their canonical dimensions and linear reference role. They are not
implicitly interchangeable with squared speed, orbital specific energy, torque,
or ordinary energy. No runtime field or C storage representation changes.

An explicit operation policy may select these finite tuples:

- gravitational parameter / geometric length → specific external potential;
- specific external potential / geometric length → acceleration;
- mass × specific external potential → external potential energy (either order).

A policy has one result per ordered operation/operand tuple. The existing
`mu / length → specific_energy` contract remains available in other policies.
A halo policy selects the external interpretation instead; loading both for the
same tuple refuses as a duplicate. No expected-output-driven inference is used.
The compiler does not infer a Hernquist model or a conserved system from a tuple.

The `cosmic_population` fixture retains the existing spatial operations and adds
these exact scalar contracts. It tests every declared scalar rule, strict
same-dimension refusals, distinct origins, unknown function-pointer calls, model
compatibility, transactional allocation failure, receipt replay and tampering.

```sh
make physical-cosmic-population-contract-test
```

The native model probe separately checks replacement lifetime and allocation
failures. Scoped model sanitizers do not establish whole-compiler memory safety.

Solver adoption uses private by-value scalar helpers. Dynamic population arrays,
profile-pointer loading, frame component extraction, hook/context ownership,
segment projection and transactional publication remain separate controller
boundaries. A declared function-pointer type does not grant physics trust.
Profile-center identity and guard ownership must be validated by the runtime owner;
a shared authored coordinate origin alone does not prove them.

A fixed halo is an external field. Its active-body momentum exchange is accounted
for by an external impulse ledger; it is not a simulated massive participant.
Test particles feel the field and active sources but contribute no source mass,
active force, active momentum or mass-weighted energy. Numerical and conservation
checks belong to the solver, separately from these static contracts.
