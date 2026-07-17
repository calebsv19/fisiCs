struct Packet {
    int lane[2];
};

static struct Packet build(int seed) {
    struct Packet p = {{seed, seed + 3}};
    return p;
}

int main(void) {
    int seed = 4;
    if () {
        seed++;
    }
    {
        struct Packet p = build(seed);
        return p.lane[0] + p.lane[1];
    }
}
