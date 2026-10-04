# ESP protocol-compatible session and heartbeat fixture

Pass 6 reconciles the earlier custom 11-bit simulation transport with the
actual retained ESP protocol v1. It projects the same sensor observations into
extended 29-bit, big-endian signal frames and protocol-v1 heartbeat frames,
then proves receiver restart, offline, recovery, and compatibility behavior.

```sh
make integration-esp-can-protocol-session ESP_WORK_ROOT=/absolute/path/to/ESP_Work
```

The protocol's signal frame does not contain the heartbeat session ID. The
contract therefore requires a restarting producer to flush pending signal
frames before announcing its new session; the simulator exposes this residual
assumption instead of claiming old-session attribution from signal bytes.
