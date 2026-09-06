# Semantic Metadata Examples

These examples are the public, runnable introduction to fisiCs semantic
metadata and its static project, physical, and hardware graph consumers.

Start with the basic example. Use the portable example when the same source
must compile cleanly with both fisiCs and Clang. Run the hardware-project
example only after those two paths are clear.

## Basic Declaration Metadata

`basic/main.c` attaches stable identities, domains, roles, and
`produces`/`consumes` relationships to ordinary C declarations.

```bash
make examples-semantic-metadata
```

The target writes:

- `build/examples/semantic_metadata/basic/metadata.json`
- `build/examples/semantic_metadata/basic/main.o`
- `build/examples/semantic_metadata/basic/basic`
- `build/examples/semantic_metadata/basic/stdout.txt`

The metadata artifact is a single-translation-unit projection. A target
that names another authored ID in the file is resolved; a valid target that is
not declared there remains external until a project graph resolves or
allowlists it.

## Strict Clang And fisiCs Comparison

`portable/main.c` uses `include/fisics/extensions.h`. Under fisiCs the macros
expand to the canonical attributes. Under another compiler they expand to
nothing.

```bash
make examples-semantic-metadata-compare
```

The comparison target:

- compiles and runs the same source with fisiCs and Clang;
- compiles the Clang lane as C99 with `-Wall -Wextra -Werror -pedantic`;
- compares exact runtime output;
- verifies that the fisiCs lane retained semantic metadata and units evidence.

Clang is a control compiler for ordinary C behavior. It is not expected to
produce fisiCs extension diagnostics or metadata artifacts.

For the complete public units and semantic macro corpus, including normalized
native-versus-macro artifact parity, run:

```bash
make examples-extension-portability
```

## Static Hardware Project

The `hardware_project/` example declares:

- one controller method;
- one temperature signal and one fan-command signal;
- one physical catalog;
- one board/node and I2C bus;
- one sensor and one actuator with driver contracts;
- two non-colliding addresses; and
- two physical-signal bindings.

Run the complete deterministic pipeline:

```bash
make examples-semantic-hardware
```

Generated inputs and artifacts are written beneath:

```text
build/examples/semantic_metadata/hardware_project/
```

The final artifact is `hardware.graph.json`. Its successful static assessment
means that the supplied declarations, catalogs, topology, allocations, driver
contracts, and physical contracts are mutually compatible. It does not prove
that any device exists, is wired, is running, or has been physically accepted.

## Next References

- [Extension Selection And Quickstart](../../docs/extensions_guide.md)
- [Extension Agent Guide](../../docs/agent_extension_guide.md)
- [Portable Extension Authoring](../../docs/extension_portability.md)
- [Extension Overlay Reference](../../docs/extension_overlays.md)
- [Metadata System Concepts](../../docs/metadata_system_concepts.md)
- [Hardware Compatibility Graph Contract](../../docs/semantic_metadata_hardware_contract.md)
