struct Record {
    int id;
    int pad;
};

int seed(void) {
    return 4;
}

struct Record g = {
    .id = seed(),
    .pad = 1,
};

int main(void) {
    return 0;
}
