// SPDX-License-Identifier: Apache-2.0

#ifndef OSP2_QUEUE_TRANSITION_VECTORS_H
#define OSP2_QUEUE_TRANSITION_VECTORS_H

typedef unsigned char osp2_u8;
typedef unsigned int osp2_u32;
typedef unsigned long osp2_u64;

#define OSP2_QUEUE_SECTOR 512UL
#define OSP2_QUEUE_STATE_EMPTY 0UL
#define OSP2_QUEUE_STATE_PENDING 1UL
#define OSP2_QUEUE_STATE_RUNNING 2UL
#define OSP2_QUEUE_STATE_COMPLETE 3UL
#define OSP2_QUEUE_STATE_FAILED 4UL
#define OSP2_QUEUE_STATE_CANCELLED 5UL
#define OSP2_QUEUE_OK 0UL
#define OSP2_QUEUE_FORMAT 1UL
#define OSP2_QUEUE_RESOURCE 2UL
#define OSP2_QUEUE_CANCEL 3UL
#define OSP2_QUEUE_COMPUTE_FAILED 4UL
#define OSP2_QUEUE_INTERRUPTED 6UL
#define OSP2_QUEUE_VECTOR_COUNT 44UL
#define OSP2_QUEUE_CORPUS_ID "queue44-v1"

#define OSP2_QUEUE_META_MAGIC 0x0000513531554445UL
#define OSP2_QUEUE_ENTRY_MAGIC 0x00004A3531554445UL
#define OSP2_QUEUE_RESULT 0x6EC4E5DB9E1056CFUL
#define OSP2_QUEUE_COMPILER 0x1E3C373BAF48FAF7UL
#define OSP2_QUEUE_DECISION(state, reason, clear_leases, run_compute) \
    ((state) | ((reason) << 8) | ((clear_leases) << 16) | ((run_compute) << 17))

osp2_u64 osp2_queue_meta_admit(
    const osp2_u8* p,
    osp2_u64 entry_lba,
    osp2_u64 entry_count
);
osp2_u64 osp2_queue_entry_action(
    const osp2_u8* p,
    osp2_u64 cpu_count,
    osp2_u64 free_pages
);
osp2_u64 osp2_queue_existing_transition(
    osp2_u64 state,
    osp2_u64 result_matches
);
osp2_u64 osp2_queue_terminal_transition(
    osp2_u64 action,
    osp2_u64 compute_ok,
    osp2_u64 result_matches
);

static void osp2_queue_zero(osp2_u8* p) {
    osp2_u64 index;
    for (index = 0; index < OSP2_QUEUE_SECTOR; index = index + 1) {
        p[index] = 0;
    }
}

static void osp2_queue_write32(osp2_u8* p, osp2_u32 value) {
    p[0] = (osp2_u8)value;
    p[1] = (osp2_u8)(value >> 8);
    p[2] = (osp2_u8)(value >> 16);
    p[3] = (osp2_u8)(value >> 24);
}

static void osp2_queue_write64(osp2_u8* p, osp2_u64 value) {
    osp2_queue_write32(p, (osp2_u32)value);
    osp2_queue_write32(p + 4, (osp2_u32)(value >> 32));
}

static osp2_u32 osp2_queue_vector_fnv(const osp2_u8* p) {
    osp2_u32 value = 0x811C9DC5U;
    osp2_u64 index;
    for (index = 0; index < OSP2_QUEUE_SECTOR - 4; index = index + 1) {
        value = (value ^ p[index]) * 0x01000193U;
    }
    return value;
}

static void osp2_queue_seal(osp2_u8* p) {
    osp2_queue_write32(p + 508, osp2_queue_vector_fnv(p));
}

static void osp2_queue_make_meta(osp2_u8* p) {
    osp2_queue_zero(p);
    osp2_queue_write64(p, OSP2_QUEUE_META_MAGIC);
    osp2_queue_write32(p + 8, 1);
    osp2_queue_write32(p + 12, 4);
    osp2_queue_write32(p + 16, 146);
    osp2_queue_seal(p);
}

static void osp2_queue_make_entry(osp2_u8* p) {
    osp2_queue_zero(p);
    osp2_queue_write64(p, OSP2_QUEUE_ENTRY_MAGIC);
    osp2_queue_write32(p + 8, 1);
    osp2_queue_write32(p + 12, 1);
    osp2_queue_write32(p + 24, OSP2_QUEUE_STATE_PENDING);
    osp2_queue_write32(p + 28, 0);
    osp2_queue_write32(p + 32, 2);
    osp2_queue_write32(p + 36, 2);
    osp2_queue_write64(p + 40, OSP2_QUEUE_RESULT);
    osp2_queue_write64(p + 48, OSP2_QUEUE_COMPILER);
    osp2_queue_seal(p);
}

static osp2_u64 osp2_queue_expect(osp2_u64 actual, osp2_u64 expected) {
    return actual == expected ? 0 : 1;
}

static osp2_u64 osp2_queue_run_vectors(void) {
    osp2_u8 sector[OSP2_QUEUE_SECTOR];
    osp2_u64 failures = 0;

    osp2_queue_make_meta(sector);
    failures += osp2_queue_expect(
        osp2_queue_meta_admit(sector, 146, 4), OSP2_QUEUE_OK
    );
    osp2_queue_write64(sector, 0);
    osp2_queue_seal(sector);
    failures += osp2_queue_expect(
        osp2_queue_meta_admit(sector, 146, 4), OSP2_QUEUE_FORMAT
    );
    osp2_queue_make_meta(sector);
    osp2_queue_write32(sector + 8, 2);
    osp2_queue_seal(sector);
    failures += osp2_queue_expect(
        osp2_queue_meta_admit(sector, 146, 4), OSP2_QUEUE_FORMAT
    );
    osp2_queue_make_meta(sector);
    osp2_queue_write32(sector + 12, 3);
    osp2_queue_seal(sector);
    failures += osp2_queue_expect(
        osp2_queue_meta_admit(sector, 146, 4), OSP2_QUEUE_FORMAT
    );
    osp2_queue_make_meta(sector);
    failures += osp2_queue_expect(
        osp2_queue_meta_admit(sector, 146, 3), OSP2_QUEUE_FORMAT
    );
    osp2_queue_make_meta(sector);
    osp2_queue_write32(sector + 16, 147);
    osp2_queue_seal(sector);
    failures += osp2_queue_expect(
        osp2_queue_meta_admit(sector, 146, 4), OSP2_QUEUE_FORMAT
    );
    osp2_queue_make_meta(sector);
    failures += osp2_queue_expect(
        osp2_queue_meta_admit(sector, 147, 4), OSP2_QUEUE_FORMAT
    );
    osp2_queue_make_meta(sector);
    sector[128] = 1;
    failures += osp2_queue_expect(
        osp2_queue_meta_admit(sector, 146, 4), OSP2_QUEUE_FORMAT
    );

    osp2_queue_make_entry(sector);
    failures += osp2_queue_expect(
        osp2_queue_entry_action(sector, 2, 2), OSP2_QUEUE_OK
    );
    osp2_queue_write64(sector, 0);
    osp2_queue_seal(sector);
    failures += osp2_queue_expect(
        osp2_queue_entry_action(sector, 2, 2), OSP2_QUEUE_FORMAT
    );
    osp2_queue_make_entry(sector);
    osp2_queue_write32(sector + 8, 2);
    osp2_queue_seal(sector);
    failures += osp2_queue_expect(
        osp2_queue_entry_action(sector, 2, 2), OSP2_QUEUE_FORMAT
    );
    osp2_queue_make_entry(sector);
    osp2_queue_write32(sector + 12, 0);
    osp2_queue_seal(sector);
    failures += osp2_queue_expect(
        osp2_queue_entry_action(sector, 2, 2), OSP2_QUEUE_FORMAT
    );
    osp2_queue_make_entry(sector);
    osp2_queue_write32(sector + 12, 5);
    osp2_queue_seal(sector);
    failures += osp2_queue_expect(
        osp2_queue_entry_action(sector, 2, 2), OSP2_QUEUE_FORMAT
    );
    osp2_queue_make_entry(sector);
    sector[128] = 1;
    failures += osp2_queue_expect(
        osp2_queue_entry_action(sector, 2, 2), OSP2_QUEUE_FORMAT
    );
    osp2_queue_make_entry(sector);
    osp2_queue_write32(sector + 24, OSP2_QUEUE_STATE_RUNNING);
    osp2_queue_seal(sector);
    failures += osp2_queue_expect(
        osp2_queue_entry_action(sector, 2, 2), OSP2_QUEUE_FORMAT
    );
    osp2_queue_make_entry(sector);
    osp2_queue_write32(sector + 24, OSP2_QUEUE_STATE_COMPLETE);
    osp2_queue_seal(sector);
    failures += osp2_queue_expect(
        osp2_queue_entry_action(sector, 2, 2), OSP2_QUEUE_FORMAT
    );
    osp2_queue_make_entry(sector);
    osp2_queue_write64(sector + 40, 0);
    osp2_queue_seal(sector);
    failures += osp2_queue_expect(
        osp2_queue_entry_action(sector, 2, 2), OSP2_QUEUE_FORMAT
    );
    osp2_queue_make_entry(sector);
    osp2_queue_write64(sector + 48, 0);
    osp2_queue_seal(sector);
    failures += osp2_queue_expect(
        osp2_queue_entry_action(sector, 2, 2), OSP2_QUEUE_FORMAT
    );
    osp2_queue_make_entry(sector);
    osp2_queue_write32(sector + 28, 1);
    osp2_queue_seal(sector);
    failures += osp2_queue_expect(
        osp2_queue_entry_action(sector, 2, 2), OSP2_QUEUE_CANCEL
    );
    osp2_queue_make_entry(sector);
    osp2_queue_write32(sector + 32, 1);
    osp2_queue_seal(sector);
    failures += osp2_queue_expect(
        osp2_queue_entry_action(sector, 2, 2), OSP2_QUEUE_RESOURCE
    );
    osp2_queue_make_entry(sector);
    osp2_queue_write32(sector + 32, 3);
    osp2_queue_seal(sector);
    failures += osp2_queue_expect(
        osp2_queue_entry_action(sector, 3, 2), OSP2_QUEUE_RESOURCE
    );
    osp2_queue_make_entry(sector);
    osp2_queue_write32(sector + 36, 1);
    osp2_queue_seal(sector);
    failures += osp2_queue_expect(
        osp2_queue_entry_action(sector, 2, 2), OSP2_QUEUE_RESOURCE
    );
    osp2_queue_make_entry(sector);
    failures += osp2_queue_expect(
        osp2_queue_entry_action(sector, 1, 2), OSP2_QUEUE_RESOURCE
    );
    osp2_queue_make_entry(sector);
    failures += osp2_queue_expect(
        osp2_queue_entry_action(sector, 2, 1), OSP2_QUEUE_RESOURCE
    );
    osp2_queue_make_entry(sector);
    failures += osp2_queue_expect(
        osp2_queue_entry_action(sector, 8, 64), OSP2_QUEUE_OK
    );

    osp2_queue_make_entry(sector);
    sector[128] = 1;
    osp2_queue_write32(sector + 24, OSP2_QUEUE_STATE_RUNNING);
    failures += osp2_queue_expect(
        osp2_queue_entry_action(sector, 0, 0), OSP2_QUEUE_FORMAT
    );
    osp2_queue_make_entry(sector);
    osp2_queue_write32(sector + 24, OSP2_QUEUE_STATE_RUNNING);
    osp2_queue_write64(sector + 40, 0);
    osp2_queue_write32(sector + 28, 1);
    osp2_queue_seal(sector);
    failures += osp2_queue_expect(
        osp2_queue_entry_action(sector, 0, 0), OSP2_QUEUE_FORMAT
    );
    osp2_queue_make_entry(sector);
    osp2_queue_write64(sector + 40, 0);
    osp2_queue_write32(sector + 28, 1);
    osp2_queue_write32(sector + 32, 1);
    osp2_queue_seal(sector);
    failures += osp2_queue_expect(
        osp2_queue_entry_action(sector, 0, 0), OSP2_QUEUE_FORMAT
    );
    osp2_queue_make_entry(sector);
    osp2_queue_write32(sector + 28, 1);
    osp2_queue_write32(sector + 32, 1);
    osp2_queue_seal(sector);
    failures += osp2_queue_expect(
        osp2_queue_entry_action(sector, 0, 0), OSP2_QUEUE_CANCEL
    );

    failures += osp2_queue_expect(
        osp2_queue_existing_transition(OSP2_QUEUE_STATE_PENDING, 0),
        OSP2_QUEUE_DECISION(OSP2_QUEUE_STATE_PENDING, 0, 0, 1)
    );
    failures += osp2_queue_expect(
        osp2_queue_existing_transition(OSP2_QUEUE_STATE_RUNNING, 0),
        OSP2_QUEUE_DECISION(
            OSP2_QUEUE_STATE_FAILED, OSP2_QUEUE_INTERRUPTED, 1, 0
        )
    );
    failures += osp2_queue_expect(
        osp2_queue_existing_transition(OSP2_QUEUE_STATE_COMPLETE, 1),
        OSP2_QUEUE_DECISION(OSP2_QUEUE_STATE_COMPLETE, 0, 0, 0)
    );
    failures += osp2_queue_expect(
        osp2_queue_existing_transition(OSP2_QUEUE_STATE_COMPLETE, 0),
        OSP2_QUEUE_DECISION(OSP2_QUEUE_STATE_FAILED, OSP2_QUEUE_FORMAT, 1, 0)
    );
    failures += osp2_queue_expect(
        osp2_queue_existing_transition(OSP2_QUEUE_STATE_FAILED, 0),
        OSP2_QUEUE_DECISION(OSP2_QUEUE_STATE_FAILED, 0, 0, 0)
    );
    failures += osp2_queue_expect(
        osp2_queue_existing_transition(OSP2_QUEUE_STATE_CANCELLED, 0),
        OSP2_QUEUE_DECISION(OSP2_QUEUE_STATE_CANCELLED, 0, 0, 0)
    );
    failures += osp2_queue_expect(
        osp2_queue_existing_transition(OSP2_QUEUE_STATE_EMPTY, 0),
        OSP2_QUEUE_DECISION(OSP2_QUEUE_STATE_FAILED, OSP2_QUEUE_FORMAT, 1, 0)
    );
    failures += osp2_queue_expect(
        osp2_queue_existing_transition(99, 1),
        OSP2_QUEUE_DECISION(OSP2_QUEUE_STATE_FAILED, OSP2_QUEUE_FORMAT, 1, 0)
    );

    failures += osp2_queue_expect(
        osp2_queue_terminal_transition(OSP2_QUEUE_OK, 1, 1),
        OSP2_QUEUE_DECISION(OSP2_QUEUE_STATE_COMPLETE, 0, 1, 0)
    );
    failures += osp2_queue_expect(
        osp2_queue_terminal_transition(OSP2_QUEUE_OK, 0, 1),
        OSP2_QUEUE_DECISION(
            OSP2_QUEUE_STATE_FAILED, OSP2_QUEUE_COMPUTE_FAILED, 1, 0
        )
    );
    failures += osp2_queue_expect(
        osp2_queue_terminal_transition(OSP2_QUEUE_OK, 1, 0),
        OSP2_QUEUE_DECISION(
            OSP2_QUEUE_STATE_FAILED, OSP2_QUEUE_COMPUTE_FAILED, 1, 0
        )
    );
    failures += osp2_queue_expect(
        osp2_queue_terminal_transition(OSP2_QUEUE_FORMAT, 1, 1),
        OSP2_QUEUE_DECISION(OSP2_QUEUE_STATE_FAILED, OSP2_QUEUE_FORMAT, 1, 0)
    );
    failures += osp2_queue_expect(
        osp2_queue_terminal_transition(OSP2_QUEUE_RESOURCE, 1, 1),
        OSP2_QUEUE_DECISION(
            OSP2_QUEUE_STATE_FAILED, OSP2_QUEUE_RESOURCE, 1, 0
        )
    );
    failures += osp2_queue_expect(
        osp2_queue_terminal_transition(OSP2_QUEUE_CANCEL, 0, 0),
        OSP2_QUEUE_DECISION(
            OSP2_QUEUE_STATE_CANCELLED, OSP2_QUEUE_CANCEL, 1, 0
        )
    );
    failures += osp2_queue_expect(
        osp2_queue_terminal_transition(99, 1, 1),
        OSP2_QUEUE_DECISION(OSP2_QUEUE_STATE_FAILED, OSP2_QUEUE_FORMAT, 1, 0)
    );
    return failures;
}

#endif
