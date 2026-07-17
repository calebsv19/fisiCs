#include <stdio.h>

typedef struct {
    int data[2];
    int scale;
} Wave50SwitchPair;

typedef struct {
    int kind;
    union {
        Wave50SwitchPair pair;
        int raw[3];
    } value;
    int suffix;
} Wave50SwitchNode;

static Wave50SwitchNode node_from(int seed, int kind) {
    Wave50SwitchNode node;
    node.kind = kind;
    node.suffix = seed * 5 - kind;
    if (kind & 1) {
        node.value.raw[0] = seed + 3;
        node.value.raw[1] = seed * 2 - 1;
        node.value.raw[2] = seed ^ 17;
    } else {
        node.value.pair.data[0] = seed * 3 + kind;
        node.value.pair.data[1] = seed - kind * 2;
        node.value.pair.scale = seed + kind + 7;
    }
    return node;
}

static int node_score(Wave50SwitchNode node) {
    int total = node.kind * 13 + node.suffix * 17;
    if (node.kind & 1) {
        total += node.value.raw[0] * 19 - node.value.raw[1] * 23 + node.value.raw[2] * 29;
    } else {
        total += node.value.pair.data[0] * 31 - node.value.pair.data[1] * 37 + node.value.pair.scale * 41;
    }
    return total;
}

int main(void) {
    Wave50SwitchNode node = node_from(4, 0);
    Wave50SwitchNode mirror = node_from(7, 1);
    int total = 0;
    int i;

    for (i = 0; i < 12; ++i) {
        Wave50SwitchNode next = node;
        switch ((node_score(node) + i + total) & 3) {
            case 0:
                next = node_from(i + 5, 0);
                next.value.pair.scale += mirror.suffix;
                /* fallthrough */
            case 1:
                next.kind = 0;
                next.value.pair.data[i & 1] += total & 15;
                mirror = next;
                break;
            case 2:
                next = node_from(i + 3, 1);
                next.value.raw[(i + 1) % 3] -= mirror.kind + i;
                node = next;
                total += node_score(node) & 127;
                continue;
            default:
                next = mirror;
                next.suffix += i - node.kind;
                break;
        }
        node = ((node_score(next) ^ total) & 1) ? next : node_from(i + 9, next.kind & 1);
        total += node_score(node);
    }

    printf("%d %d %d %d %d\n", node.kind, node.suffix, mirror.suffix, node_score(node), total);
    return 0;
}
