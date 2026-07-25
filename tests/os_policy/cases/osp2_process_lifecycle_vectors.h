// SPDX-License-Identifier: Apache-2.0

#ifndef OSP2_PROCESS_LIFECYCLE_VECTORS_H
#define OSP2_PROCESS_LIFECYCLE_VECTORS_H

typedef unsigned long osp2_u64;

#define OSP2_PROCESS_VECTOR_COUNT 34UL
#define OSP2_PROCESS_CORPUS_ID "process34-v1"

osp2_u64 osp2_process_transition_admit(
    osp2_u64, osp2_u64, osp2_u64, osp2_u64, osp2_u64, osp2_u64
);
osp2_u64 osp2_process_authority_check(
    osp2_u64, osp2_u64, osp2_u64, osp2_u64
);
osp2_u64 osp2_process_lifecycle_validate(
    osp2_u64, osp2_u64, osp2_u64, osp2_u64, osp2_u64,
    osp2_u64, osp2_u64, osp2_u64, osp2_u64
);

static osp2_u64 osp2_process_expect(osp2_u64 actual, osp2_u64 expected) {
    return actual == expected ? 0 : 1;
}

static osp2_u64 osp2_process_run_vectors(void) {
    osp2_u64 failures = 0;

    failures += osp2_process_expect(
        osp2_process_transition_admit(1, 0, 0, 0, 0, 0), 0
    );
    failures += osp2_process_expect(
        osp2_process_transition_admit(2, 2, 0, 1, 0x11, 0x1000), 0
    );
    failures += osp2_process_expect(
        osp2_process_transition_admit(2, 3, 0, 1, 0x12, 0x2000), 0
    );
    failures += osp2_process_expect(
        osp2_process_transition_admit(99, 99, 1, 1, 1, 1), 1
    );
    failures += osp2_process_expect(
        osp2_process_transition_admit(1, 1, 1, 1, 1, 1), 2
    );
    failures += osp2_process_expect(
        osp2_process_transition_admit(2, 0, 1, 0, 0, 0), 2
    );
    failures += osp2_process_expect(
        osp2_process_transition_admit(1, 0, 1, 1, 1, 1), 3
    );
    failures += osp2_process_expect(
        osp2_process_transition_admit(2, 2, 1, 0, 0, 0), 3
    );
    failures += osp2_process_expect(
        osp2_process_transition_admit(1, 0, 0, 1, 1, 1), 4
    );
    failures += osp2_process_expect(
        osp2_process_transition_admit(2, 2, 0, 0, 0, 0), 4
    );
    failures += osp2_process_expect(
        osp2_process_transition_admit(1, 0, 0, 0, 1, 0), 5
    );
    failures += osp2_process_expect(
        osp2_process_transition_admit(1, 0, 0, 0, 0, 1), 5
    );
    failures += osp2_process_expect(
        osp2_process_transition_admit(2, 3, 0, 1, 0, 0x2000), 5
    );
    failures += osp2_process_expect(
        osp2_process_transition_admit(2, 3, 0, 1, 0x12, 0), 5
    );

    failures += osp2_process_expect(
        osp2_process_authority_check(0x11, 0x1000, 0x11, 0x1000), 0
    );
    failures += osp2_process_expect(
        osp2_process_authority_check(
            ~0UL, 0xFFFFFFFFFFFFF000UL, ~0UL, 0xFFFFFFFFFFFFF000UL
        ), 0
    );
    failures += osp2_process_expect(
        osp2_process_authority_check(0, 0, 0x11, 0x1000), 1
    );
    failures += osp2_process_expect(
        osp2_process_authority_check(0, 0x1000, 0x11, 0x1000), 1
    );
    failures += osp2_process_expect(
        osp2_process_authority_check(0x11, 0x1000, 0, 0x1000), 2
    );
    failures += osp2_process_expect(
        osp2_process_authority_check(0x11, 0x1000, 0x11, 0), 2
    );
    failures += osp2_process_expect(
        osp2_process_authority_check(0x11, 0x1000, 0x12, 0x2000), 3
    );
    failures += osp2_process_expect(
        osp2_process_authority_check(0x11, 0x1000, 0x11, 0x2000), 4
    );

    failures += osp2_process_expect(
        osp2_process_lifecycle_validate(4, 4, 2, 2, 32, 1, 1, 1, 1), 0
    );
    failures += osp2_process_expect(
        osp2_process_lifecycle_validate(3, 3, 1, 1, 31, 0, 0, 0, 0), 1
    );
    failures += osp2_process_expect(
        osp2_process_lifecycle_validate(4, 3, 2, 2, 32, 1, 1, 1, 1), 1
    );
    failures += osp2_process_expect(
        osp2_process_lifecycle_validate(4, 4, 1, 2, 31, 0, 0, 0, 0), 2
    );
    failures += osp2_process_expect(
        osp2_process_lifecycle_validate(4, 4, 2, 1, 32, 1, 1, 1, 1), 2
    );
    failures += osp2_process_expect(
        osp2_process_lifecycle_validate(4, 4, 2, 2, 31, 0, 0, 0, 0), 3
    );
    failures += osp2_process_expect(
        osp2_process_lifecycle_validate(4, 4, 2, 2, 33, 1, 1, 1, 1), 3
    );
    failures += osp2_process_expect(
        osp2_process_lifecycle_validate(4, 4, 2, 2, 32, 0, 0, 0, 0), 4
    );
    failures += osp2_process_expect(
        osp2_process_lifecycle_validate(4, 4, 2, 2, 32, 1, 0, 1, 1), 4
    );
    failures += osp2_process_expect(
        osp2_process_lifecycle_validate(4, 4, 2, 2, 32, 2, 1, 1, 1), 4
    );
    failures += osp2_process_expect(
        osp2_process_lifecycle_validate(4, 4, 2, 2, 32, 1, 1, 0, 0), 5
    );
    failures += osp2_process_expect(
        osp2_process_lifecycle_validate(4, 4, 2, 2, 32, 1, 1, 1, 0), 6
    );

    return failures;
}

#endif
