// SPDX-License-Identifier: Apache-2.0

typedef unsigned long osp2_u64;

#define OSP2_EXTENT_OK 0UL
#define OSP2_EXTENT_COUNT_ERROR 10UL
#define OSP2_EXTENT_OWNER_ERROR 14UL
#define OSP2_EXTENT_OWNER_QUEUE 1UL
#define OSP2_EXTENT_OWNER_TEST 2UL
#define OSP2_EXTENT_MAX_PAGES 16UL
#define OSP2_EXTENT_HANDLE_COUNT 8UL

osp2_u64 osp2_extent_request_admit(
    osp2_u64 page_count,
    osp2_u64 owner
) {
    if (page_count == 0 || page_count > OSP2_EXTENT_MAX_PAGES) {
        return OSP2_EXTENT_COUNT_ERROR;
    }
    if (owner != OSP2_EXTENT_OWNER_QUEUE &&
        owner != OSP2_EXTENT_OWNER_TEST) {
        return OSP2_EXTENT_OWNER_ERROR;
    }
    return OSP2_EXTENT_OK;
}

osp2_u64 osp2_extent_observation_admit(
    osp2_u64 free_before,
    osp2_u64 free_during,
    osp2_u64 active_handles,
    osp2_u64 contention,
    osp2_u64 ap_attempts,
    osp2_u64 zeroed
) {
    if (free_before < 6 || free_during != free_before - 6) {
        return 1;
    }
    if (active_handles != 2) {
        return 2;
    }
    if (contention == 0 || ap_attempts != 1) {
        return 3;
    }
    if (zeroed != 1) {
        return 4;
    }
    return OSP2_EXTENT_OK;
}

osp2_u64 osp2_extent_final_admit(
    osp2_u64 free_before,
    osp2_u64 free_after,
    osp2_u64 active_handles,
    osp2_u64 reused,
    osp2_u64 generation_changed,
    osp2_u64 irq_progress
) {
    if (free_after != free_before || active_handles != 0) {
        return 5;
    }
    if (reused != 1 || generation_changed != 1) {
        return 6;
    }
    if (irq_progress != 1) {
        return 7;
    }
    return OSP2_EXTENT_OK;
}

osp2_u64 osp2_extent_geometry(osp2_u64 selector) {
    if (selector == 0) {
        return OSP2_EXTENT_MAX_PAGES;
    }
    if (selector == 1) {
        return OSP2_EXTENT_HANDLE_COUNT;
    }
    return 0;
}
