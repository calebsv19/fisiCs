#ifndef FISICS_PROBE_WAVE90_FUNCTION_TYPE_TYPEDEF_FACTORY_CONTRACT_H
#define FISICS_PROBE_WAVE90_FUNCTION_TYPE_TYPEDEF_FACTORY_CONTRACT_H

typedef int Wave90LeafFunction(int value);
typedef Wave90LeafFunction *Wave90FactoryFunction(int route);

int wave90_function_type_typedef_factory_call(
    Wave90LeafFunction direct,
    Wave90FactoryFunction chooser,
    int route,
    int value);

#endif
