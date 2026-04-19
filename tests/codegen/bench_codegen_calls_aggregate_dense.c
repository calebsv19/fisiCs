typedef struct Packet {
    int id;
    int values[4];
} Packet;

static Packet build_packet(int seed) {
    Packet p = {0};
    p.id = seed;
    for (int i = 0; i < 4; ++i) {
        p.values[i] = seed * (i + 3) + i;
    }
    return p;
}

static int reduce_packet(Packet p) {
    int total = p.id;
    for (int i = 0; i < 4; ++i) {
        total += p.values[i];
    }
    return total;
}

static int dispatch(Packet* packets, int count) {
    int total = 0;
    for (int i = 0; i < count; ++i) {
        Packet current = packets[i];
        total += reduce_packet(current);
        total += reduce_packet(build_packet(current.id + i));
    }
    return total;
}

int main(void) {
    Packet packets[12];
    for (int i = 0; i < 12; ++i) {
        packets[i] = build_packet(i * 11 + 5);
    }
    return dispatch(packets, 12);
}
