enum Kind {
    KIND_A = 1,
    KIND_B = 2
};

int table[(KIND_A + KIND_B) == 3 ? 1 : -1];

