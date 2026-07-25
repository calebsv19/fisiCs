// SPDX-License-Identifier: Apache-2.0

#ifndef OSP2_PMM_EXTENT_VECTORS_H
#define OSP2_PMM_EXTENT_VECTORS_H

typedef unsigned long osp2_u64;

#define OSP2_EXTENT_OK 0UL
#define OSP2_EXTENT_COUNT_ERROR 10UL
#define OSP2_EXTENT_OWNER_ERROR 14UL
#define OSP2_EXTENT_OWNER_QUEUE 1UL
#define OSP2_EXTENT_OWNER_TEST 2UL
#define OSP2_EXTENT_VECTOR_COUNT 62UL
#define OSP2_EXTENT_CORPUS_ID "extent62-v1"

osp2_u64 osp2_extent_request_admit(
    osp2_u64 page_count,
    osp2_u64 owner
);
osp2_u64 osp2_extent_observation_admit(
    osp2_u64 free_before,
    osp2_u64 free_during,
    osp2_u64 active_handles,
    osp2_u64 contention,
    osp2_u64 ap_attempts,
    osp2_u64 zeroed
);
osp2_u64 osp2_extent_final_admit(
    osp2_u64 free_before,
    osp2_u64 free_after,
    osp2_u64 active_handles,
    osp2_u64 reused,
    osp2_u64 generation_changed,
    osp2_u64 irq_progress
);
osp2_u64 osp2_extent_geometry(osp2_u64 selector);

static osp2_u64 osp2_extent_expect(
    osp2_u64 actual,
    osp2_u64 expected
) {
    return actual == expected ? 0 : 1;
}

static osp2_u64 osp2_extent_run_vectors(void) {
    osp2_u64 failures = 0;

    failures += osp2_extent_expect(
        osp2_extent_request_admit(1, OSP2_EXTENT_OWNER_QUEUE), OSP2_EXTENT_OK
    );
    failures += osp2_extent_expect(
        osp2_extent_request_admit(1, OSP2_EXTENT_OWNER_TEST), OSP2_EXTENT_OK
    );
    failures += osp2_extent_expect(
        osp2_extent_request_admit(16, OSP2_EXTENT_OWNER_QUEUE), OSP2_EXTENT_OK
    );
    failures += osp2_extent_expect(
        osp2_extent_request_admit(16, OSP2_EXTENT_OWNER_TEST), OSP2_EXTENT_OK
    );
    failures += osp2_extent_expect(
        osp2_extent_request_admit(0, OSP2_EXTENT_OWNER_QUEUE),
        OSP2_EXTENT_COUNT_ERROR
    );
    failures += osp2_extent_expect(
        osp2_extent_request_admit(17, OSP2_EXTENT_OWNER_QUEUE),
        OSP2_EXTENT_COUNT_ERROR
    );
    failures += osp2_extent_expect(
        osp2_extent_request_admit(~0UL, OSP2_EXTENT_OWNER_QUEUE),
        OSP2_EXTENT_COUNT_ERROR
    );
    failures += osp2_extent_expect(
        osp2_extent_request_admit(0, OSP2_EXTENT_OWNER_TEST),
        OSP2_EXTENT_COUNT_ERROR
    );
    failures += osp2_extent_expect(
        osp2_extent_request_admit(17, OSP2_EXTENT_OWNER_TEST),
        OSP2_EXTENT_COUNT_ERROR
    );
    failures += osp2_extent_expect(
        osp2_extent_request_admit(~0UL, OSP2_EXTENT_OWNER_TEST),
        OSP2_EXTENT_COUNT_ERROR
    );
    failures += osp2_extent_expect(
        osp2_extent_request_admit(1, 0), OSP2_EXTENT_OWNER_ERROR
    );
    failures += osp2_extent_expect(
        osp2_extent_request_admit(1, 3), OSP2_EXTENT_OWNER_ERROR
    );
    failures += osp2_extent_expect(
        osp2_extent_request_admit(1, ~0UL), OSP2_EXTENT_OWNER_ERROR
    );
    failures += osp2_extent_expect(
        osp2_extent_request_admit(16, 0), OSP2_EXTENT_OWNER_ERROR
    );
    failures += osp2_extent_expect(
        osp2_extent_request_admit(16, 3), OSP2_EXTENT_OWNER_ERROR
    );
    failures += osp2_extent_expect(
        osp2_extent_request_admit(16, ~0UL), OSP2_EXTENT_OWNER_ERROR
    );
    failures += osp2_extent_expect(
        osp2_extent_request_admit(0, 0), OSP2_EXTENT_COUNT_ERROR
    );
    failures += osp2_extent_expect(
        osp2_extent_request_admit(17, 3), OSP2_EXTENT_COUNT_ERROR
    );
    failures += osp2_extent_expect(
        osp2_extent_request_admit(~0UL, ~0UL), OSP2_EXTENT_COUNT_ERROR
    );

    failures += osp2_extent_expect(
        osp2_extent_observation_admit(6, 0, 2, 1, 1, 1), OSP2_EXTENT_OK
    );
    failures += osp2_extent_expect(
        osp2_extent_observation_admit(12, 6, 2, 2, 1, 1), OSP2_EXTENT_OK
    );
    failures += osp2_extent_expect(
        osp2_extent_observation_admit(~0UL, ~0UL - 6, 2, ~0UL, 1, 1),
        OSP2_EXTENT_OK
    );
    failures += osp2_extent_expect(
        osp2_extent_observation_admit(0, 0, 2, 1, 1, 1), 1
    );
    failures += osp2_extent_expect(
        osp2_extent_observation_admit(5, ~0UL, 2, 1, 1, 1), 1
    );
    failures += osp2_extent_expect(
        osp2_extent_observation_admit(6, 1, 2, 1, 1, 1), 1
    );
    failures += osp2_extent_expect(
        osp2_extent_observation_admit(12, 5, 2, 1, 1, 1), 1
    );
    failures += osp2_extent_expect(
        osp2_extent_observation_admit(6, 0, 0, 1, 1, 1), 2
    );
    failures += osp2_extent_expect(
        osp2_extent_observation_admit(6, 0, 1, 1, 1, 1), 2
    );
    failures += osp2_extent_expect(
        osp2_extent_observation_admit(6, 0, 3, 1, 1, 1), 2
    );
    failures += osp2_extent_expect(
        osp2_extent_observation_admit(6, 0, ~0UL, 1, 1, 1), 2
    );
    failures += osp2_extent_expect(
        osp2_extent_observation_admit(6, 0, 2, 0, 1, 1), 3
    );
    failures += osp2_extent_expect(
        osp2_extent_observation_admit(6, 0, 2, 1, 0, 1), 3
    );
    failures += osp2_extent_expect(
        osp2_extent_observation_admit(6, 0, 2, 1, 2, 1), 3
    );
    failures += osp2_extent_expect(
        osp2_extent_observation_admit(6, 0, 2, 1, ~0UL, 1), 3
    );
    failures += osp2_extent_expect(
        osp2_extent_observation_admit(6, 0, 2, 1, 1, 0), 4
    );
    failures += osp2_extent_expect(
        osp2_extent_observation_admit(6, 0, 2, 1, 1, 2), 4
    );
    failures += osp2_extent_expect(
        osp2_extent_observation_admit(6, 0, 2, 1, 1, ~0UL), 4
    );
    failures += osp2_extent_expect(
        osp2_extent_observation_admit(0, 1, 0, 0, 0, 0), 1
    );
    failures += osp2_extent_expect(
        osp2_extent_observation_admit(6, 0, 0, 0, 0, 0), 2
    );
    failures += osp2_extent_expect(
        osp2_extent_observation_admit(6, 0, 2, 0, 0, 0), 3
    );
    failures += osp2_extent_expect(
        osp2_extent_observation_admit(6, 0, 2, 1, 0, 0), 3
    );

    failures += osp2_extent_expect(
        osp2_extent_final_admit(0, 0, 0, 1, 1, 1), OSP2_EXTENT_OK
    );
    failures += osp2_extent_expect(
        osp2_extent_final_admit(6, 6, 0, 1, 1, 1), OSP2_EXTENT_OK
    );
    failures += osp2_extent_expect(
        osp2_extent_final_admit(~0UL, ~0UL, 0, 1, 1, 1), OSP2_EXTENT_OK
    );
    failures += osp2_extent_expect(
        osp2_extent_final_admit(6, 5, 0, 1, 1, 1), 5
    );
    failures += osp2_extent_expect(
        osp2_extent_final_admit(6, 6, 1, 1, 1, 1), 5
    );
    failures += osp2_extent_expect(
        osp2_extent_final_admit(6, 6, ~0UL, 1, 1, 1), 5
    );
    failures += osp2_extent_expect(
        osp2_extent_final_admit(6, 5, 1, 0, 0, 0), 5
    );
    failures += osp2_extent_expect(
        osp2_extent_final_admit(6, 6, 0, 0, 1, 1), 6
    );
    failures += osp2_extent_expect(
        osp2_extent_final_admit(6, 6, 0, 2, 1, 1), 6
    );
    failures += osp2_extent_expect(
        osp2_extent_final_admit(6, 6, 0, ~0UL, 1, 1), 6
    );
    failures += osp2_extent_expect(
        osp2_extent_final_admit(6, 6, 0, 1, 0, 1), 6
    );
    failures += osp2_extent_expect(
        osp2_extent_final_admit(6, 6, 0, 1, 2, 1), 6
    );
    failures += osp2_extent_expect(
        osp2_extent_final_admit(6, 6, 0, 1, ~0UL, 1), 6
    );
    failures += osp2_extent_expect(
        osp2_extent_final_admit(6, 6, 0, 0, 0, 0), 6
    );
    failures += osp2_extent_expect(
        osp2_extent_final_admit(6, 6, 0, 1, 1, 0), 7
    );
    failures += osp2_extent_expect(
        osp2_extent_final_admit(6, 6, 0, 1, 1, 2), 7
    );
    failures += osp2_extent_expect(
        osp2_extent_final_admit(6, 6, 0, 1, 1, ~0UL), 7
    );

    failures += osp2_extent_expect(osp2_extent_geometry(0), 16);
    failures += osp2_extent_expect(osp2_extent_geometry(1), 8);
    failures += osp2_extent_expect(osp2_extent_geometry(2), 0);
    failures += osp2_extent_expect(osp2_extent_geometry(~0UL), 0);

    return failures;
}

#endif
