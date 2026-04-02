struct Pair {
    int x;
    int y;
};

struct Box {
    struct Pair p;
};

static int sum_box(struct Box b) {
    return b.p.x + b.p.y;
}

int main(void) {
    struct Box box = {{3, 4}};
    struct Box *ptr = &box;
    ptr->p.x += 6;
    return sum_box(*ptr);
}
