#include <stdio.h>

struct Row {
    char name[6];
    int id;
};

static unsigned score(const struct Row *row) {
    unsigned acc = (unsigned)row->id * 100u;

    for (int i = 0; i < 6; ++i) {
        acc += (unsigned char)row->name[i];
    }
    return acc;
}

int main(void) {
    struct Row rows[2] = {
        {"core", 1},
        {
            .id = 2,
            .name = "io",
        },
    };

    printf(
        "%u %u %u %u\n",
        (unsigned char)rows[0].name[4],
        (unsigned char)rows[1].name[2],
        (unsigned char)rows[1].name[5],
        score(&rows[1])
    );
    return 0;
}
