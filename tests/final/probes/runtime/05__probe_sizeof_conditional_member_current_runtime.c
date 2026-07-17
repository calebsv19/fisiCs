extern int printf(const char*, ...);

struct Box {
    int value;
    int slots[3];
};

int main(void) {
    int hits = 0;
    struct Box first = {7, {1, 2, 3}};
    struct Box second = {9, {4, 5, 6}};
    struct Box *left = &first;
    struct Box *right = &second;

    unsigned array_size = (unsigned)sizeof((1 ? left : right)->slots);
    int picked = (0 ? (hits += 100, left) : right)->slots[2];

    printf("%d %u %d\n", hits, array_size, picked);
    return 0;
}
