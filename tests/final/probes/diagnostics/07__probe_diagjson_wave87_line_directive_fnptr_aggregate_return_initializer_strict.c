#line 19701 "virtual_wave87_fnptr_aggregate_return_initializer_strict.c"
struct Wave87DirectPacket {
    int value;
};
typedef struct Wave87DirectPacket (*Wave87DirectPacketFn)(int value);
typedef int (*Wave87DirectScalarFn)(int value);

extern int wave87_direct_scalar(int value);

int wave87_direct_select(void) {
    Wave87DirectPacketFn selected = wave87_direct_scalar;
    return selected != 0;
}

int main(void) {
    return wave87_direct_select();
}
