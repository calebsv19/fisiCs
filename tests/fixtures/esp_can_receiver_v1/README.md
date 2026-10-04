# ESP CAN receiver and conformance fixture

Pass 5 consumes the exact Pass 4 transport trace and models the supervisor's
bounded RX queue, payload admission, sequence handling, latest-good state, and
freshness expiry. It also emits compact replay slices for a visual debugger.

```sh
make integration-esp-can-receiver ESP_WORK_ROOT=/absolute/path/to/ESP_Work
```

The v1 eight-byte payload has no session identity or in-frame codec version.
The receiver therefore cannot distinguish a source restart from an old
sequence and rejects a reset as out of order. That is an explicit protocol gap.
