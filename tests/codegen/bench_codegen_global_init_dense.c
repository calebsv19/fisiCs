typedef struct Span {
    int start;
    int end;
} Span;

typedef struct Table {
    Span spans[6];
    int weights[8];
    const char* label;
} Table;

static Table tables[4] = {
    {
        .spans = { {1, 2}, {3, 5}, {8, 13}, {21, 34}, {55, 89}, {144, 233} },
        .weights = { 1, 1, 2, 3, 5, 8, 13, 21 },
        .label = "alpha"
    },
    {
        .spans = { {2, 4}, {6, 10}, {14, 18}, {22, 26}, {30, 34}, {38, 42} },
        .weights = { 34, 21, 13, 8, 5, 3, 2, 1 },
        .label = "beta"
    },
    {
        .spans = { {5, 7}, {11, 13}, {17, 19}, {23, 29}, {31, 37}, {41, 43} },
        .weights = { 4, 9, 16, 25, 36, 49, 64, 81 },
        .label = "gamma"
    },
    {
        .spans = { {9, 12}, {15, 18}, {21, 24}, {27, 30}, {33, 36}, {39, 42} },
        .weights = { 7, 14, 21, 28, 35, 42, 49, 56 },
        .label = "delta"
    }
};

static int digest_table(const Table* table) {
    int acc = 0;
    for (int i = 0; i < 6; ++i) {
        acc += table->spans[i].start * table->spans[i].end;
    }
    for (int i = 0; i < 8; ++i) {
        acc ^= table->weights[i] + i;
    }
    return acc + (int)table->label[0];
}

int main(void) {
    int total = 0;
    static int local_offsets[10] = { 3, 1, 4, 1, 5, 9, 2, 6, 5, 3 };
    for (int i = 0; i < 4; ++i) {
        total += digest_table(&tables[i]);
        total += local_offsets[i + 2];
    }
    return total;
}
