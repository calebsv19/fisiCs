# Fixed-direction rotation pilot inputs

`source_pins.json` pins the selected read-only `core_structural` basis utility,
its diagnostic dependency, required headers and VERSION. The portable pilot
runner copies these inputs into a fresh task-owned output root and generates
six fixed unary 3D double wrappers around a private constant basis. It never
mutates owner sources. See [rotation contracts](../../../../docs/physical_rotations.md)
for trust boundaries, invocation, numeric controls and the known mixed-record
ABI limitation. Source pins must be reviewed against owner changes before repinning.
