#include <stdio.h>

typedef struct {
    int prefix;
    struct {
        int x;
        int y;
        int z;
    } member;
    int slots[3];
} Wave50MemberRecord;

typedef struct {
    Wave50MemberRecord records[2];
    int cursor;
} Wave50MemberTable;

static int record_score(Wave50MemberRecord record) {
    return record.prefix * 5
        + record.member.x * 7
        - record.member.y * 11
        + record.member.z * 13
        + record.slots[0] * 17
        - record.slots[1] * 19
        + record.slots[2] * 23;
}

static int table_score(Wave50MemberTable table) {
    return table.cursor * 29 + record_score(table.records[0]) - record_score(table.records[1]);
}

int main(void) {
    Wave50MemberTable table = {
        {
            {3, {5, 8, 13}, {21, 34, 55}},
            {4, {7, 11, 18}, {29, 47, 76}}
        },
        2
    };
    int total = 0;
    int i;

    for (i = 0; i < 14; ++i) {
        Wave50MemberRecord *record = &table.records[(table.cursor + i) & 1];
        int *field = ((table_score(table) + i) & 1)
            ? &record->member.y
            : record->slots + ((record->prefix + i + 3000) % 3);
        int offset = (field >= record->slots && field < record->slots + 3)
            ? (int)(field - record->slots)
            : 5;
        *field += table.cursor + offset - i;
        table.records[(i + 1) & 1] = (offset == 5)
            ? (Wave50MemberRecord){record->member.z - i, {record->member.y, record->member.x + table.cursor, *field}, {record->slots[2], record->slots[0] + i, record->slots[1] - table.cursor}}
            : (Wave50MemberRecord){record->prefix + offset, {record->slots[0] - i, record->slots[1] + offset, record->slots[2] - table.cursor}, {record->member.x, record->member.y + i, record->member.z - offset}};
        table.cursor = (table.cursor + offset + record->prefix + i) & 7;
        total += table_score(table);
    }

    printf("%d %d %d %d %d\n", table.cursor, table.records[0].member.z, table.records[1].slots[1], table_score(table), total);
    return 0;
}
