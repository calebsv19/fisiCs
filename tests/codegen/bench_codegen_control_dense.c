typedef struct Node {
    int value;
    int weight;
} Node;

static int mix(Node n, int salt) {
    int acc = n.value * 3 + n.weight - salt;
    for (int i = 0; i < 6; ++i) {
        if ((acc + i) & 1) {
            acc += (n.weight ^ i);
        } else {
            acc -= (n.value + i);
        }
        switch ((acc + salt + i) & 7) {
            case 0: acc += i; break;
            case 1: acc -= i * 2; break;
            case 2: acc ^= (salt + i); break;
            case 3: acc += n.value; break;
            case 4: acc -= n.weight; break;
            case 5: acc += salt; break;
            case 6: acc ^= n.weight; break;
            default: acc += n.value - n.weight; break;
        }
    }
    return acc;
}

static int fold_nodes(Node* nodes, int count, int seed) {
    int acc = seed;
    for (int i = 0; i < count; ++i) {
        acc += mix(nodes[i], acc);
        if ((acc & 3) == 0) {
            acc ^= mix(nodes[count - 1 - i], i);
        }
    }
    return acc;
}

int main(void) {
    Node nodes[24];
    for (int i = 0; i < 24; ++i) {
        nodes[i].value = i * 7 + 3;
        nodes[i].weight = i * 5 - 9;
    }
    return fold_nodes(nodes, 24, 19);
}
