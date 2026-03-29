union Payload {
    int i;
    long long ll;
};

struct Packet {
    char tag;
    union Payload payload;
    short tail;
};

struct Packet abi_make_packet(int base) {
    struct Packet p;
    p.tag = (char)(base + 1);
    p.payload.ll = (long long)base * 1000 + 77;
    p.tail = (short)(base - 2);
    return p;
}

long long abi_score_packet(struct Packet p) {
    return (long long)p.tag + p.payload.ll + (long long)p.tail * 9;
}

