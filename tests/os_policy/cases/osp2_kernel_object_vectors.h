// SPDX-License-Identifier: Apache-2.0

#ifndef OSP2_KERNEL_OBJECT_VECTORS_H
#define OSP2_KERNEL_OBJECT_VECTORS_H

typedef unsigned long osp2_u64;

#define OSP2_KOBJ_VECTOR_COUNT 51UL
#define OSP2_KOBJ_CORPUS_ID "kobj51-v1"

osp2_u64 osp2_kobj_request_admit(osp2_u64, osp2_u64, osp2_u64);
osp2_u64 osp2_kobj_observation_admit(
    osp2_u64, osp2_u64, osp2_u64, osp2_u64, osp2_u64, osp2_u64
);
osp2_u64 osp2_kobj_final_admit(
    osp2_u64, osp2_u64, osp2_u64, osp2_u64, osp2_u64, osp2_u64
);
osp2_u64 osp2_kobj_geometry(osp2_u64);

static osp2_u64 osp2_kobj_expect(osp2_u64 actual, osp2_u64 expected) {
    return actual == expected ? 0 : 1;
}

static osp2_u64 osp2_kobj_run_vectors(void) {
    osp2_u64 failures = 0;

    failures += osp2_kobj_expect(osp2_kobj_request_admit(64, 64, 64), 0);
    failures += osp2_kobj_expect(osp2_kobj_request_admit(0, 64, 64), 1);
    failures += osp2_kobj_expect(osp2_kobj_request_admit(63, 64, 64), 1);
    failures += osp2_kobj_expect(osp2_kobj_request_admit(65, 64, 64), 1);
    failures += osp2_kobj_expect(osp2_kobj_request_admit(~0UL, 64, 64), 1);
    failures += osp2_kobj_expect(osp2_kobj_request_admit(64, 0, 64), 2);
    failures += osp2_kobj_expect(osp2_kobj_request_admit(64, 63, 64), 2);
    failures += osp2_kobj_expect(osp2_kobj_request_admit(64, 65, 64), 2);
    failures += osp2_kobj_expect(osp2_kobj_request_admit(64, ~0UL, 64), 2);
    failures += osp2_kobj_expect(osp2_kobj_request_admit(64, 64, 0), 3);
    failures += osp2_kobj_expect(osp2_kobj_request_admit(64, 64, 63), 3);
    failures += osp2_kobj_expect(osp2_kobj_request_admit(64, 64, 65), 3);
    failures += osp2_kobj_expect(osp2_kobj_request_admit(64, 64, ~0UL), 3);
    failures += osp2_kobj_expect(osp2_kobj_request_admit(0, 0, 0), 1);
    failures += osp2_kobj_expect(osp2_kobj_request_admit(64, 0, 0), 2);
    failures += osp2_kobj_expect(osp2_kobj_request_admit(~0UL, ~0UL, ~0UL), 1);

    failures += osp2_kobj_expect(
        osp2_kobj_observation_admit(2, 62, 1, 1, 1, 1), 0
    );
    failures += osp2_kobj_expect(
        osp2_kobj_observation_admit(2, 62, ~0UL, 1, 1, 1), 0
    );
    failures += osp2_kobj_expect(
        osp2_kobj_observation_admit(0, 62, 1, 1, 1, 1), 4
    );
    failures += osp2_kobj_expect(
        osp2_kobj_observation_admit(1, 62, 1, 1, 1, 1), 4
    );
    failures += osp2_kobj_expect(
        osp2_kobj_observation_admit(3, 62, 1, 1, 1, 1), 4
    );
    failures += osp2_kobj_expect(
        osp2_kobj_observation_admit(2, 61, 1, 1, 1, 1), 4
    );
    failures += osp2_kobj_expect(
        osp2_kobj_observation_admit(2, 63, 1, 1, 1, 1), 4
    );
    failures += osp2_kobj_expect(
        osp2_kobj_observation_admit(2, ~0UL, 1, 1, 1, 1), 4
    );
    failures += osp2_kobj_expect(
        osp2_kobj_observation_admit(2, 62, 0, 1, 1, 1), 5
    );
    failures += osp2_kobj_expect(
        osp2_kobj_observation_admit(2, 62, 1, 0, 1, 1), 5
    );
    failures += osp2_kobj_expect(
        osp2_kobj_observation_admit(2, 62, 1, 2, 1, 1), 5
    );
    failures += osp2_kobj_expect(
        osp2_kobj_observation_admit(2, 62, 1, ~0UL, 1, 1), 5
    );
    failures += osp2_kobj_expect(
        osp2_kobj_observation_admit(2, 62, 1, 1, 0, 1), 6
    );
    failures += osp2_kobj_expect(
        osp2_kobj_observation_admit(2, 62, 1, 1, 2, 1), 6
    );
    failures += osp2_kobj_expect(
        osp2_kobj_observation_admit(2, 62, 1, 1, 1, 0), 6
    );
    failures += osp2_kobj_expect(
        osp2_kobj_observation_admit(2, 62, 1, 1, 1, 2), 6
    );
    failures += osp2_kobj_expect(
        osp2_kobj_observation_admit(0, 0, 0, 0, 0, 0), 4
    );

    failures += osp2_kobj_expect(
        osp2_kobj_final_admit(0, 1, 1, 1, 1, 1), 0
    );
    failures += osp2_kobj_expect(
        osp2_kobj_final_admit(1, 1, 1, 1, 1, 1), 7
    );
    failures += osp2_kobj_expect(
        osp2_kobj_final_admit(~0UL, 1, 1, 1, 1, 1), 7
    );
    failures += osp2_kobj_expect(
        osp2_kobj_final_admit(0, 0, 1, 1, 1, 1), 7
    );
    failures += osp2_kobj_expect(
        osp2_kobj_final_admit(0, ~0UL, 1, 1, 1, 1), 7
    );
    failures += osp2_kobj_expect(
        osp2_kobj_final_admit(0, 1, 0, 1, 1, 1), 7
    );
    failures += osp2_kobj_expect(
        osp2_kobj_final_admit(0, 1, ~0UL, 1, 1, 1), 7
    );
    failures += osp2_kobj_expect(
        osp2_kobj_final_admit(0, 1, 1, 0, 1, 1), 8
    );
    failures += osp2_kobj_expect(
        osp2_kobj_final_admit(0, 1, 1, 1, 0, 1), 8
    );
    failures += osp2_kobj_expect(
        osp2_kobj_final_admit(0, 1, 1, ~0UL, ~0UL, 1), 8
    );
    failures += osp2_kobj_expect(
        osp2_kobj_final_admit(0, 1, 1, 1, 1, 0), 9
    );
    failures += osp2_kobj_expect(
        osp2_kobj_final_admit(0, 1, 1, 1, 1, ~0UL), 9
    );

    failures += osp2_kobj_expect(osp2_kobj_geometry(0), 64);
    failures += osp2_kobj_expect(osp2_kobj_geometry(1), 64);
    failures += osp2_kobj_expect(osp2_kobj_geometry(2), 1);
    failures += osp2_kobj_expect(osp2_kobj_geometry(3), 0xA5);
    failures += osp2_kobj_expect(osp2_kobj_geometry(4), 0);
    failures += osp2_kobj_expect(osp2_kobj_geometry(~0UL), 0);

    return failures;
}

#endif
