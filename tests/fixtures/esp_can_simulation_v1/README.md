# ESP CAN software simulation fixture

This fixture binds the exact Pass 2 static graph hashes to two admitted,
ESP-owned host simulation profiles. The integration runner executes each
profile twice, requires byte-identical `run.json` results, and projects the
selected sensor records over the declared three-node CAN topology.

Run from the fisiCs root:

```sh
make integration-esp-can-simulation ESP_WORK_ROOT=/absolute/path/to/ESP_Work
```

The transport adapter is intentionally a deterministic semantic-delivery
projection. It does not model CAN arbitration, bit stuffing, electrical
behavior, firmware execution, or physical hardware.
