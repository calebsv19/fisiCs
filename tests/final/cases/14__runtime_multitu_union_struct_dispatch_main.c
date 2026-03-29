#include <stdio.h>

union Payload {
    int i;
    long long ll;
};

struct Packet {
    char tag;
    union Payload payload;
    short tail;
};

struct Packet abi_make_packet(int base);
long long abi_score_packet(struct Packet p);

int main(void) {
    struct Packet p = abi_make_packet(6);
    long long score = abi_score_packet(p);
    printf("%d %lld %d\n", (int)p.tag, score, (int)p.tail);
    return 0;
}

