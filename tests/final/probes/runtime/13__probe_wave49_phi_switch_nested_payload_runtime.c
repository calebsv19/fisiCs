#include <stdio.h>

typedef struct {
    int xy[2];
    int mark;
} Wave49Point;

typedef struct {
    int tag;
    union {
        Wave49Point pair[2];
        struct {
            int left;
            int right;
            int extra;
        } edge;
    } payload;
    int tail;
} Wave49Node;

static Wave49Point make_point(int seed, int twist) {
    Wave49Point point;
    point.xy[0] = seed * 3 + twist;
    point.xy[1] = seed - twist * 2;
    point.mark = seed * seed - twist;
    return point;
}

static Wave49Node make_node(int seed, int mode) {
    Wave49Node node;
    node.tag = seed + mode * 2;
    node.tail = seed * 5 - mode;
    if (mode & 1) {
        node.payload.edge.left = seed * 7 + mode;
        node.payload.edge.right = seed - mode * 3;
        node.payload.edge.extra = seed ^ (mode + 11);
    } else {
        node.payload.pair[0] = make_point(seed + 1, mode);
        node.payload.pair[1] = make_point(seed + 2, mode + 1);
    }
    return node;
}

static int score(Wave49Node node, int mode) {
    int total = node.tag * 13 + node.tail * 3;
    if (mode & 1) {
        total += node.payload.edge.left * 5;
        total -= node.payload.edge.right * 7;
        total += node.payload.edge.extra * 11;
    } else {
        total += node.payload.pair[0].xy[0] * 17;
        total -= node.payload.pair[0].xy[1] * 19;
        total += node.payload.pair[1].mark * 23;
    }
    return total;
}

int main(void) {
    Wave49Node current = make_node(4, 0);
    int mode = 0;
    int total = 0;
    int i;

    for (i = 0; i < 10; ++i) {
        Wave49Node next;
        switch ((score(current, mode) + i + total) & 3) {
            case 0:
                next = make_node(i + 5, mode ^ 1);
                break;
            case 1:
                next = (Wave49Node){current.tag + i, {.edge = {current.tail - i, current.tag + i * 2, score(current, mode) & 31}}, current.tail + i};
                mode = 1;
                break;
            case 2:
                next = (Wave49Node){current.tail - i, {{make_point(i + 2, current.tag & 3), make_point(i + 4, current.tail & 5)}}, current.tag + current.tail};
                mode = 0;
                break;
            default:
                next = current;
                if (mode & 1) {
                    next.payload.edge.extra += i + next.tail;
                } else {
                    next.payload.pair[i & 1].mark -= next.tag - i;
                }
                break;
        }
        current = ((score(next, mode) ^ total) & 1) ? next : make_node(i + 7, mode);
        total += score(current, mode);
    }

    printf("%d %d %d %d\n", current.tag, current.tail, score(current, mode), total);
    return 0;
}
