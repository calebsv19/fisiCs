#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef union RecordPayload {
    unsigned char bytes[10];
    unsigned short half[5];
} RecordPayload;

typedef struct Record {
    int key;
    RecordPayload payload;
    unsigned short stamp;
} Record;

typedef struct RecordBox {
    Record selected[2];
    unsigned short guard;
} RecordBox;

static Record make_record(int key, unsigned seed) {
    Record record;
    unsigned i;

    record.key = key;
    for (i = 0u; i < 10u; ++i) {
        record.payload.bytes[i] = (unsigned char)(0x31u + seed * 5u + i * 9u);
    }
    record.stamp = (unsigned short)(0x5900u + seed * 23u);
    return record;
}

static int compare_key_record(const void *lhs, const void *rhs) {
    const int *key = (const int *)lhs;
    const Record *record = (const Record *)rhs;
    return (*key > record->key) - (*key < record->key);
}

static unsigned fold_box(RecordBox box) {
    unsigned acc = (unsigned)box.guard * 149u;
    unsigned i;
    unsigned k;

    for (i = 0u; i < 2u; ++i) {
        acc = acc * 127u + (unsigned)(box.selected[i].key + 101);
        acc = acc * 127u + (unsigned)box.selected[i].stamp;
        for (k = 0u; k < 10u; ++k) {
            acc = acc * 127u + (unsigned)box.selected[i].payload.bytes[k] + k;
        }
    }
    return acc;
}

int main(void) {
    Record records[4];
    RecordBox box;
    Record *found;
    int key = 31;
    unsigned typed = 0u;

    records[0] = make_record(11, 2u);
    records[1] = make_record(19, 5u);
    records[2] = make_record(31, 8u);
    records[3] = make_record(47, 13u);

    found = (Record *)bsearch(&key, records, 4u, sizeof(records[0]), compare_key_record);
    if (found == 0) {
        return 2;
    }

    memcpy(&box.selected[0], found, sizeof(box.selected[0]));
    memcpy(&box.selected[1], &records[1], sizeof(box.selected[1]));
    box.selected[0].payload.half[3] =
        (unsigned short)(box.selected[0].payload.half[3] + box.selected[1].payload.bytes[4]);
    box.selected[1].payload.bytes[8] =
        (unsigned char)(box.selected[1].payload.bytes[8] ^ box.selected[0].payload.bytes[2]);
    box.guard = (unsigned short)(0x7200u + box.selected[0].stamp + box.selected[1].payload.bytes[8]);

    typed += (unsigned)box.selected[0].key * 3u;
    typed += (unsigned)box.selected[0].payload.half[3] * 5u;
    typed += (unsigned)box.selected[1].payload.bytes[8] * 7u;
    typed += (unsigned)box.guard;

    printf("%d %u %u %u %u %u\n",
           box.selected[0].key,
           (unsigned)box.selected[0].payload.half[3],
           (unsigned)box.selected[1].payload.bytes[8],
           (unsigned)box.guard,
           typed,
           fold_box(box));
    return 0;
}
