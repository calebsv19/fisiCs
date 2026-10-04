# ESP CAN topology v1 fixture

This fixed fixture owns the Pass 2 compiler-side declarations, catalogs,
topology, and digest-bound external intake contract. Run it through:

```bash
make integration-esp-can-topology ESP_WORK_ROOT=/absolute/path/to/ESP_Work
```

Do not edit generated graphs under `build/`. Change the earliest authored input,
update the external contract only from inspected current ESP inputs, and rerun
the complete positive/negative deterministic gate.

Passing establishes static compatibility of the supplied declarations only.
It does not establish simulation, firmware, runtime CAN, wiring, resistance, or
physical acceptance.
