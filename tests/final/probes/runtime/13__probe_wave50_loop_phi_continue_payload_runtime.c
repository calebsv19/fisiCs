#include <stdio.h>

typedef struct {
    int lane[3];
    int mark;
} Wave50PhiCell;

typedef struct {
    int tag;
    int mode;
    union {
        Wave50PhiCell cells[2];
        struct {
            int left;
            int right;
            int carry;
        } edge;
    } payload;
    int tail;
} Wave50PhiPacket;

static Wave50PhiCell cell_from(int seed, int twist) {
    Wave50PhiCell cell;
    cell.lane[0] = seed + twist * 2;
    cell.lane[1] = seed * 3 - twist;
    cell.lane[2] = seed ^ (twist + 9);
    cell.mark = seed * seed - twist * 5;
    return cell;
}

static Wave50PhiPacket packet_from(int seed, int mode) {
    Wave50PhiPacket packet;
    packet.tag = seed * 2 + mode;
    packet.mode = mode & 1;
    packet.tail = seed * 7 - mode;
    if (packet.mode) {
        packet.payload.edge.left = seed + mode * 4;
        packet.payload.edge.right = seed * 5 - mode;
        packet.payload.edge.carry = seed ^ (mode + 13);
    } else {
        packet.payload.cells[0] = cell_from(seed + 1, mode);
        packet.payload.cells[1] = cell_from(seed + 2, mode + 1);
    }
    return packet;
}

static int score(Wave50PhiPacket packet) {
    int total = packet.tag * 11 + packet.mode * 13 + packet.tail * 17;
    if (packet.mode) {
        total += packet.payload.edge.left * 19;
        total -= packet.payload.edge.right * 23;
        total += packet.payload.edge.carry * 29;
    } else {
        total += packet.payload.cells[0].lane[0] * 31;
        total -= packet.payload.cells[1].lane[1] * 37;
        total += packet.payload.cells[1].mark * 41;
    }
    return total;
}

int main(void) {
    Wave50PhiPacket current = packet_from(5, 0);
    Wave50PhiPacket saved = packet_from(3, 1);
    int total = 0;
    int i;

    for (i = 0; i < 13; ++i) {
        Wave50PhiPacket next = ((score(current) + i) & 1)
            ? packet_from(i + 6, current.mode ^ 1)
            : current;
        if (((next.tag + total + i) % 3) == 0) {
            saved = next;
            total += score(saved) & 63;
            continue;
        }
        if (next.mode) {
            next.payload.edge.carry += saved.tail - i;
            next.payload.edge.left -= current.tag + i;
        } else {
            next.payload.cells[i & 1].lane[(i + 1) % 3] += saved.tag - current.tail;
            next.payload.cells[(i + 1) & 1].mark -= i + next.tag;
        }
        current = ((score(next) ^ total) & 2) ? next : saved;
        total += score(current);
    }

    printf("%d %d %d %d %d\n", current.tag, saved.tail, current.mode, score(current), total);
    return 0;
}
