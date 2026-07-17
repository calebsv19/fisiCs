struct Packet {
    int lane[2];
};

static struct Packet build(int seed) {
    struct Packet p = {{seed, seed + 3}};
    return p;
}

int main(void) {
    int broken[] = {1, 2, 3}
    int sentinel = 0;
    {
        struct Packet p = build(4);
        return p.lane[0] + p.lane[1];
    }
}
