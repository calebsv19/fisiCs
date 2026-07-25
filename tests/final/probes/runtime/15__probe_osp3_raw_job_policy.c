// SPDX-License-Identifier: Apache-2.0

#include "15__probe_osp3_raw_job_policy.h"

#define OSP3_JOB_MAGIC 0x31474B50424F4A46UL
#define OSP3_JOB_HEADER_SIZE 64U
#define OSP3_JOB_STEP_SIZE 24U
#define OSP3_JOB_FLAG_ALLOWED 1U

static osp3_job_u32 job_read32(const osp3_job_u8* p) {
    return (osp3_job_u32)p[0] | ((osp3_job_u32)p[1] << 8) |
           ((osp3_job_u32)p[2] << 16) | ((osp3_job_u32)p[3] << 24);
}

static osp3_job_u64 job_read64(const osp3_job_u8* p) {
    return (osp3_job_u64)job_read32(p) |
           ((osp3_job_u64)job_read32(p + 4) << 32);
}

static osp3_job_u32 job_fnv(const osp3_job_u8* p, osp3_job_u64 count) {
    osp3_job_u32 value = 0x811c9dc5U;
    osp3_job_u64 index;
    for (index = 0; index < count; index = index + 1) {
        value = (value ^ p[index]) * 0x01000193U;
    }
    return value;
}

static void job_clear(struct Osp3JobState* state) {
    osp3_job_u32 index;
    state->accepted = 0;
    state->reason = 0;
    state->step_count = 0;
    state->generation = 0;
    state->payload_size = 0;
    state->payload_digest = 0;
    state->job_id = 0;
    state->authority = 0;
    for (index = 0; index < OSP3_JOB_MAX_STEPS; index = index + 1) {
        state->input_start[index] = 0;
        state->input_end[index] = 0;
        state->output_start[index] = 0;
        state->output_end[index] = 0;
    }
}

static int job_reject(struct Osp3JobState* state, osp3_job_u32 reason) {
    job_clear(state);
    state->reason = reason;
    return 0;
}

static int job_ranges_overlap(
    osp3_job_u32 a_start,
    osp3_job_u32 a_end,
    osp3_job_u32 b_start,
    osp3_job_u32 b_end
) {
    return a_start < b_end && b_start < a_end;
}

int osp3_raw_job_admit(
    const osp3_job_u8* image,
    osp3_job_u64 image_size,
    osp3_job_u64 expected_authority,
    osp3_job_u32 minimum_generation,
    osp3_job_u32 replay_floor,
    struct Osp3JobState* state
) {
    osp3_job_u32 total_size;
    osp3_job_u32 step_count;
    osp3_job_u32 table_end;
    osp3_job_u32 payload_checksum;
    osp3_job_u32 generation;
    osp3_job_u32 index;

    job_clear(state);
    if (image_size < OSP3_JOB_HEADER_SIZE) {
        return job_reject(state, OSP3_JOB_SHORT);
    }
    if (job_read64(image) != OSP3_JOB_MAGIC) {
        return job_reject(state, OSP3_JOB_IDENT);
    }
    if (job_read32(image + 8) != 1U) {
        return job_reject(state, OSP3_JOB_VERSION);
    }

    total_size = job_read32(image + 16);
    step_count = job_read32(image + 20);
    if (job_read32(image + 12) != OSP3_JOB_HEADER_SIZE ||
        total_size < OSP3_JOB_HEADER_SIZE || total_size > image_size ||
        step_count == 0 || step_count > OSP3_JOB_MAX_STEPS ||
        step_count > (0xffffffffU - OSP3_JOB_HEADER_SIZE) / OSP3_JOB_STEP_SIZE) {
        return job_reject(state, OSP3_JOB_GEOMETRY);
    }
    table_end = OSP3_JOB_HEADER_SIZE + step_count * OSP3_JOB_STEP_SIZE;
    if (table_end > total_size || (job_read32(image + 24) & ~OSP3_JOB_FLAG_ALLOWED) != 0 ||
        job_read32(image + 28) != 0) {
        return job_reject(state, OSP3_JOB_GEOMETRY);
    }
    if (job_fnv(image, 60) != job_read32(image + 60)) {
        return job_reject(state, OSP3_JOB_HEADER_CHECKSUM);
    }
    payload_checksum = job_fnv(image + OSP3_JOB_HEADER_SIZE,
                               total_size - OSP3_JOB_HEADER_SIZE);
    if (payload_checksum != job_read32(image + 52)) {
        return job_reject(state, OSP3_JOB_PAYLOAD_CHECKSUM);
    }
    if (job_read64(image + 40) == 0 ||
        job_read64(image + 40) != expected_authority) {
        return job_reject(state, OSP3_JOB_AUTHORITY);
    }
    generation = job_read32(image + 48);
    if (generation < minimum_generation || generation <= replay_floor) {
        return job_reject(state, OSP3_JOB_REPLAY);
    }
    if (job_read64(image + 32) == 0) {
        return job_reject(state, OSP3_JOB_IDENT);
    }

    state->step_count = step_count;
    state->generation = generation;
    state->payload_size = total_size - table_end;
    state->payload_digest = payload_checksum;
    state->job_id = job_read64(image + 32);
    state->authority = job_read64(image + 40);

    for (index = 0; index < step_count; index = index + 1) {
        const osp3_job_u8* step = image + OSP3_JOB_HEADER_SIZE +
                                  index * OSP3_JOB_STEP_SIZE;
        osp3_job_u32 input_start = job_read32(step + 8);
        osp3_job_u32 input_size = job_read32(step + 12);
        osp3_job_u32 output_start = job_read32(step + 16);
        osp3_job_u32 output_size = job_read32(step + 20);
        osp3_job_u32 prior;

        if (job_read32(step) == 0 || (job_read32(step + 4) & ~1U) != 0 ||
            input_size == 0 || output_size == 0 ||
            input_start < table_end || output_start < table_end ||
            input_start > total_size - input_size ||
            output_start > total_size - output_size) {
            return job_reject(state, OSP3_JOB_STEP_RANGE);
        }
        state->input_start[index] = input_start;
        state->input_end[index] = input_start + input_size;
        state->output_start[index] = output_start;
        state->output_end[index] = output_start + output_size;
        if (job_ranges_overlap(input_start, input_start + input_size,
                               output_start, output_start + output_size)) {
            return job_reject(state, OSP3_JOB_STEP_OVERLAP);
        }
        for (prior = 0; prior < index; prior = prior + 1) {
            if (job_ranges_overlap(input_start, input_start + input_size,
                                   state->output_start[prior],
                                   state->output_end[prior]) ||
                job_ranges_overlap(output_start, output_start + output_size,
                                   state->input_start[prior],
                                   state->input_end[prior]) ||
                job_ranges_overlap(output_start, output_start + output_size,
                                   state->output_start[prior],
                                   state->output_end[prior])) {
                return job_reject(state, OSP3_JOB_STEP_OVERLAP);
            }
        }
    }

    state->accepted = 1;
    state->reason = OSP3_JOB_ACCEPT;
    return 1;
}
