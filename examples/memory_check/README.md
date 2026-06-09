# Memory-Check Example

This is an explicitly opt-in demo for the `memory-check` overlay. It is not
part of default `make examples` because it intentionally leaks memory so users
can see the runtime report shape.

Run it with:

```bash
cd /Users/calebsv/Desktop/CodeWork/fisiCs
make examples-memory-check
```

Expected stderr includes:

```text
[fisics:memory-check] summary: active=1 leaked_bytes=21 allocs=1 frees=0 double_free=0 unknown_free=0 tracker_failures=0
[fisics:memory-check] leak: size=21 allocated_at=leak_demo.c:4
```

The overlay remains default-off and is not included in `--overlay=all`.
