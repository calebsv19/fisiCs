# ESP CAN frame and queue transport fixture

This Pass 4 fixture consumes the exact Pass 3 simulated-observation hash and
models deterministic Classical CAN framing, payload encoding, arbitration,
bounded per-node queues, latency, retry outcomes, and bus utilization.

```sh
make integration-esp-can-transport ESP_WORK_ROOT=/absolute/path/to/ESP_Work
```

The model is protocol/software evidence. It does not emulate a specific ESP
TWAI peripheral, transceiver voltages, wiring, noise, oscillator error, or
physical sensor behavior.
