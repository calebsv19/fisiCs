struct Record {
    int id;
    int pad;
};

struct Record g = {
    .id = seed(),
    .pad = 1,
};
