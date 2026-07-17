#line 19801 "virtual_wave87_fnptr_aggregate_return_assignment_reverse_strict.c"
struct Wave87ReversePacket {
    int value;
};
typedef struct Wave87ReversePacket (*Wave87ReversePacketFn)(int value);
typedef int (*Wave87ReverseScalarFn)(int value);

extern struct Wave87ReversePacket wave87_reverse_packet(int value);

int wave87_reverse_assign(void) {
    Wave87ReverseScalarFn selected = 0;
    selected = wave87_reverse_packet;
    return selected != 0;
}

int main(void) {
    return wave87_reverse_assign();
}
