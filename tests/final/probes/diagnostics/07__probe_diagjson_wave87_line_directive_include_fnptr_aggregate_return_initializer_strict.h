#line 19901 "virtual_wave87_include_fnptr_aggregate_return_initializer_strict.h"
struct Wave87IncludePacket {
    int value;
};
typedef struct Wave87IncludePacket (*Wave87IncludePacketFn)(int value);
typedef int (*Wave87IncludeScalarFn)(int value);

extern int wave87_include_scalar(int value);

static int wave87_include_select(void) {
    Wave87IncludePacketFn selected = wave87_include_scalar;
    return selected != 0;
}
