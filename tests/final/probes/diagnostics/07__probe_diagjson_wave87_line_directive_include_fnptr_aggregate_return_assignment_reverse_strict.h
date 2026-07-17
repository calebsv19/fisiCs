#line 20001 "virtual_wave87_include_fnptr_aggregate_return_assignment_reverse_strict.h"
struct Wave87IncludeReversePacket {
    int value;
};
typedef struct Wave87IncludeReversePacket (*Wave87IncludeReversePacketFn)(int value);
typedef int (*Wave87IncludeReverseScalarFn)(int value);

extern struct Wave87IncludeReversePacket wave87_include_reverse_packet(int value);

static int wave87_include_reverse_assign(void) {
    Wave87IncludeReverseScalarFn selected = 0;
    selected = wave87_include_reverse_packet;
    return selected != 0;
}
