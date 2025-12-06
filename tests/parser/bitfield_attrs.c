struct Packed {
    unsigned :0;
    unsigned a:3;
    int :5;
} __attribute__((packed));

union U {
    int x;
    unsigned y:4;
} __attribute__((aligned(8)));

int g = -1;
