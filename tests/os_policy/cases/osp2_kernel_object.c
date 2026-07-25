// SPDX-License-Identifier: Apache-2.0

typedef unsigned long osp2_u64;

#define OSP2_KOBJ_OK 0UL
#define OSP2_KOBJ_BYTES 64UL
#define OSP2_KOBJ_ALIGN 64UL
#define OSP2_KOBJ_COUNT 64UL
#define OSP2_KOBJ_BACKING_PAGES 1UL
#define OSP2_KOBJ_POISON 0xA5UL

osp2_u64 osp2_kobj_request_admit(
    osp2_u64 object_bytes,
    osp2_u64 object_align,
    osp2_u64 object_count
) {
    if (object_bytes != OSP2_KOBJ_BYTES) return 1;
    if (object_align != OSP2_KOBJ_ALIGN) return 2;
    if (object_count != OSP2_KOBJ_COUNT) return 3;
    return OSP2_KOBJ_OK;
}

osp2_u64 osp2_kobj_observation_admit(
    osp2_u64 active_objects,
    osp2_u64 free_objects,
    osp2_u64 contention,
    osp2_u64 ap_attempts,
    osp2_u64 zeroed,
    osp2_u64 backing_pages
) {
    if (active_objects != 2 || free_objects != OSP2_KOBJ_COUNT - 2) return 4;
    if (contention == 0 || ap_attempts != 1) return 5;
    if (zeroed != 1 || backing_pages != OSP2_KOBJ_BACKING_PAGES) return 6;
    return OSP2_KOBJ_OK;
}

osp2_u64 osp2_kobj_final_admit(
    osp2_u64 active_objects,
    osp2_u64 cache_ready,
    osp2_u64 backing_handles,
    osp2_u64 reused,
    osp2_u64 generation_changed,
    osp2_u64 poison_and_irq
) {
    if (active_objects != 0 || cache_ready != 1 || backing_handles != 1) return 7;
    if (reused != 1 || generation_changed != 1) return 8;
    if (poison_and_irq != 1) return 9;
    return OSP2_KOBJ_OK;
}

osp2_u64 osp2_kobj_geometry(osp2_u64 selector) {
    if (selector == 0) return OSP2_KOBJ_BYTES;
    if (selector == 1) return OSP2_KOBJ_COUNT;
    if (selector == 2) return OSP2_KOBJ_BACKING_PAGES;
    if (selector == 3) return OSP2_KOBJ_POISON;
    return 0;
}
