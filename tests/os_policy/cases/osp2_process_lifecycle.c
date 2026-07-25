// SPDX-License-Identifier: Apache-2.0

typedef unsigned long osp2_u64;

#define OSP2_PROCESS_EMPTY 0UL
#define OSP2_PROCESS_RUNNING 1UL
#define OSP2_PROCESS_EXITED 2UL
#define OSP2_PROCESS_FAULTED 3UL

#define OSP2_ACTION_CREATE 1UL
#define OSP2_ACTION_DESTROY 2UL

#define OSP2_TRANSITION_OK 0UL
#define OSP2_TRANSITION_BAD_ACTION 1UL
#define OSP2_TRANSITION_BAD_STATE 2UL
#define OSP2_TRANSITION_BAD_CALLER 3UL
#define OSP2_TRANSITION_BAD_SLOT 4UL
#define OSP2_TRANSITION_BAD_AUTHORITY 5UL

#define OSP2_AUTHORITY_OK 0UL
#define OSP2_AUTHORITY_NO_ACTIVE 1UL
#define OSP2_AUTHORITY_PRESENTED_ZERO 2UL
#define OSP2_AUTHORITY_TOKEN_MISMATCH 3UL
#define OSP2_AUTHORITY_POINTER_MISMATCH 4UL

#define OSP2_LIFECYCLE_OK 0UL
#define OSP2_LIFECYCLE_BAD_COUNTS 1UL
#define OSP2_LIFECYCLE_BAD_TERMINALS 2UL
#define OSP2_LIFECYCLE_BAD_RECLAIM 3UL
#define OSP2_LIFECYCLE_BAD_BASELINE 4UL
#define OSP2_LIFECYCLE_BAD_STALE 5UL
#define OSP2_LIFECYCLE_BAD_SLOT 6UL

osp2_u64 osp2_process_transition_admit(
    osp2_u64 action,
    osp2_u64 state,
    osp2_u64 current_task,
    osp2_u64 slot_present,
    osp2_u64 object_token,
    osp2_u64 object_pointer
) {
    if (action != OSP2_ACTION_CREATE && action != OSP2_ACTION_DESTROY) {
        return OSP2_TRANSITION_BAD_ACTION;
    }
    if (action == OSP2_ACTION_CREATE) {
        if (state != OSP2_PROCESS_EMPTY) {
            return OSP2_TRANSITION_BAD_STATE;
        }
    } else if (state != OSP2_PROCESS_EXITED &&
               state != OSP2_PROCESS_FAULTED) {
        return OSP2_TRANSITION_BAD_STATE;
    }
    if (current_task != 0) {
        return OSP2_TRANSITION_BAD_CALLER;
    }
    if (action == OSP2_ACTION_CREATE) {
        if (slot_present != 0) {
            return OSP2_TRANSITION_BAD_SLOT;
        }
        if (object_token != 0 || object_pointer != 0) {
            return OSP2_TRANSITION_BAD_AUTHORITY;
        }
    } else {
        if (slot_present != 1) {
            return OSP2_TRANSITION_BAD_SLOT;
        }
        if (object_token == 0 || object_pointer == 0) {
            return OSP2_TRANSITION_BAD_AUTHORITY;
        }
    }
    return OSP2_TRANSITION_OK;
}

osp2_u64 osp2_process_authority_check(
    osp2_u64 active_token,
    osp2_u64 active_pointer,
    osp2_u64 presented_token,
    osp2_u64 presented_pointer
) {
    if (active_token == 0 || active_pointer == 0) {
        return OSP2_AUTHORITY_NO_ACTIVE;
    }
    if (presented_token == 0 || presented_pointer == 0) {
        return OSP2_AUTHORITY_PRESENTED_ZERO;
    }
    if (presented_token != active_token) {
        return OSP2_AUTHORITY_TOKEN_MISMATCH;
    }
    if (presented_pointer != active_pointer) {
        return OSP2_AUTHORITY_POINTER_MISMATCH;
    }
    return OSP2_AUTHORITY_OK;
}

osp2_u64 osp2_process_lifecycle_validate(
    osp2_u64 creates,
    osp2_u64 destroys,
    osp2_u64 exits,
    osp2_u64 faults,
    osp2_u64 reclaimed_pages,
    osp2_u64 pmm_baseline_restored,
    osp2_u64 cache_baseline_restored,
    osp2_u64 stale_release_rejected,
    osp2_u64 slot_empty
) {
    if (creates != 4 || destroys != 4) {
        return OSP2_LIFECYCLE_BAD_COUNTS;
    }
    if (exits != 2 || faults != 2) {
        return OSP2_LIFECYCLE_BAD_TERMINALS;
    }
    if (reclaimed_pages != 32) {
        return OSP2_LIFECYCLE_BAD_RECLAIM;
    }
    if (pmm_baseline_restored != 1 || cache_baseline_restored != 1) {
        return OSP2_LIFECYCLE_BAD_BASELINE;
    }
    if (stale_release_rejected != 1) {
        return OSP2_LIFECYCLE_BAD_STALE;
    }
    if (slot_empty != 1) {
        return OSP2_LIFECYCLE_BAD_SLOT;
    }
    return OSP2_LIFECYCLE_OK;
}
