/*
 * Compiler-side contract mirror derived from immutable os-dev tag
 * edu-23-bounded-workload-parallelism, commit
 * cf375eaea291c5d00a3b09fef789be387d63aa9e.
 *
 * Exact generated-C authorities:
 *   queue_kernel.c
 *   SHA-256 a891ccada3fa02531e926b1300771a9174734b0a95c7e33c911248ca6e46cb02
 *   control_kernel.c
 *   SHA-256 1a722b5ea3fb017f419b309a755f7d91e3134bd7cf819b9fff079add796ecde6
 *
 * This mirror isolates the request/grant/path correlation added by EDU-23.
 * Resource mutation and BSP/AP execution remain OS-owned.
 */
typedef unsigned int edu23_u32;
typedef unsigned long long edu23_u64;

enum {
    EDU23_STATE_PENDING = 1,
    EDU23_STATE_RUNNING = 2,
    EDU23_STATE_COMPLETE = 3,
    EDU23_STATE_FAILED = 4,
    EDU23_STATE_CANCELLED = 5,
    EDU23_OK = 0,
    EDU23_ERR_FORMAT = 1,
    EDU23_ERR_RESOURCE = 2,
    EDU23_CANCEL = 3
};

edu23_u64 edu23_admission_action(
    edu23_u32 requested_workers,
    edu23_u32 requested_pages,
    edu23_u64 cpu_count,
    edu23_u64 free_pages,
    edu23_u32 cancel_pending) {
    if (cancel_pending > 1) return EDU23_ERR_FORMAT;
    if (cancel_pending != 0) return EDU23_CANCEL;
    if ((requested_workers != 1 && requested_workers != 2) ||
        requested_pages != 2 || cpu_count < 2 || free_pages < 2) {
        return EDU23_ERR_RESOURCE;
    }
    return EDU23_OK;
}

edu23_u64 edu23_grant_value(
    edu23_u32 requested_workers, edu23_u32 effective_workers) {
    return (edu23_u64)requested_workers |
           ((edu23_u64)effective_workers << 32);
}

edu23_u64 edu23_compute_value(
    edu23_u64 bsp_calls,
    edu23_u64 ap_dispatches,
    edu23_u64 ap_completions) {
    if (bsp_calls > 0xffffULL ||
        ap_dispatches > 0xffffULL ||
        ap_completions > 0xffffULL) return ~0ULL;
    return bsp_calls | (ap_dispatches << 16) | (ap_completions << 32);
}

int edu23_path_evidence_valid(
    edu23_u32 requested_workers,
    edu23_u32 effective_workers,
    edu23_u64 bsp_calls,
    edu23_u64 ap_dispatches,
    edu23_u64 ap_completions) {
    if (requested_workers < 1 || requested_workers > 2 ||
        effective_workers != requested_workers) return 0;
    if (effective_workers == 1) {
        return bsp_calls == 6 &&
               ap_dispatches == 0 &&
               ap_completions == 0;
    }
    return bsp_calls == 3 &&
           ap_dispatches == 3 &&
           ap_completions == 3;
}

int edu23_entry_grant_valid(
    edu23_u32 state,
    edu23_u32 requested_workers,
    edu23_u32 effective_workers,
    edu23_u32 running_published,
    edu23_u32 resource_granted,
    edu23_u64 grant_value,
    edu23_u32 compute_completed,
    edu23_u64 compute_value) {
    int post_running;
    if (state < EDU23_STATE_PENDING || state > EDU23_STATE_CANCELLED ||
        running_published > 1 || resource_granted > 1 ||
        compute_completed > 1) return 0;
    post_running =
        state == EDU23_STATE_RUNNING ||
        state == EDU23_STATE_COMPLETE ||
        (state == EDU23_STATE_FAILED && running_published != 0);
    if (!post_running) {
        return effective_workers == 0 &&
               resource_granted == 0 &&
               grant_value == 0 &&
               compute_completed == 0 &&
               compute_value == 0;
    }
    if (requested_workers < 1 || requested_workers > 2 ||
        effective_workers != requested_workers ||
        resource_granted == 0 ||
        grant_value !=
            edu23_grant_value(requested_workers, effective_workers)) {
        return 0;
    }
    if (state != EDU23_STATE_COMPLETE) {
        return compute_completed == 0 && compute_value == 0;
    }
    if (running_published == 0 || compute_completed == 0) return 0;
    if (effective_workers == 1) return compute_value == 6ULL;
    return compute_value ==
        (3ULL | (3ULL << 16) | (3ULL << 32));
}
