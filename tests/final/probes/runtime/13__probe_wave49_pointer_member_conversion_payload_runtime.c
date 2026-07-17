#include <stdio.h>

typedef struct {
    int head;
    int slots[4];
    int tail;
} Wave49Record;

typedef struct {
    Wave49Record records[2];
    int bias;
} Wave49Table;

static int checksum_record(Wave49Record record) {
    return record.head * 5
        + record.slots[0] * 7
        - record.slots[1] * 11
        + record.slots[2] * 13
        - record.slots[3] * 17
        + record.tail * 19;
}

static int checksum_table(Wave49Table table) {
    return table.bias * 3 + checksum_record(table.records[0]) - checksum_record(table.records[1]);
}

int main(void) {
    Wave49Table table = {
        {
            {3, {5, 8, 13, 21}, 34},
            {4, {7, 11, 18, 29}, 47}
        },
        6
    };
    int total = 0;
    int i;

    for (i = 0; i < 11; ++i) {
        Wave49Record *record = &table.records[(i + table.bias) & 1];
        int *base = record->slots;
        int *member = ((checksum_table(table) + i) & 1)
            ? base + ((record->head + i + 4000) % 4)
            : &record->tail;
        int offset = (member == &record->tail) ? 4 : (int)(member - base);
        *member += table.bias + offset - i;
        table.records[(i + 1) & 1] = (offset & 1)
            ? (Wave49Record){record->tail - i, {record->slots[3], record->slots[0] + offset, record->slots[1] - i, record->slots[2] + table.bias}, record->head + i}
            : (Wave49Record){record->head + offset, {record->slots[1] + i, record->slots[2], record->slots[3] - offset, record->slots[0] + table.bias}, record->tail - i};
        table.bias += offset - (i & 1);
        total += checksum_table(table);
    }

    printf("%d %d %d %d %d\n", table.bias, table.records[0].slots[2], table.records[1].tail, checksum_table(table), total);
    return 0;
}
