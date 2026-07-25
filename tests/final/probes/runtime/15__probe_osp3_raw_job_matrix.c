// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>

#include "15__probe_osp3_raw_job_policy.h"

#ifndef OSP3_JOB_MODE
#define OSP3_JOB_MODE 0
#endif
#ifndef OSP3_JOB_SEED
#define OSP3_JOB_SEED 0x7f4a7c15U
#endif
#ifndef OSP3_JOB_CASE_BUDGET
#define OSP3_JOB_CASE_BUDGET 256U
#endif

#define JOB_IMAGE_CAPACITY 512U
#define JOB_TOTAL_SIZE 256U
#define JOB_AUTHORITY 0x61d0f3a55ac33e19UL

struct JobStats {
    unsigned cases;
    unsigned accepted;
    unsigned rejected;
    unsigned failures;
    unsigned digest;
};

static void write32(unsigned char* p, unsigned value) {
    p[0] = (unsigned char)value;
    p[1] = (unsigned char)(value >> 8);
    p[2] = (unsigned char)(value >> 16);
    p[3] = (unsigned char)(value >> 24);
}

static void write64(unsigned char* p, unsigned long value) {
    write32(p, (unsigned)value);
    write32(p + 4, (unsigned)(value >> 32));
}

static unsigned fnv(const unsigned char* p, unsigned count) {
    unsigned value = 0x811c9dc5U;
    unsigned index;
    for (index = 0; index < count; index = index + 1) {
        value = (value ^ p[index]) * 0x01000193U;
    }
    return value;
}

static void zero_bytes(unsigned char* p, unsigned count) {
    unsigned index;
    for (index = 0; index < count; index = index + 1) p[index] = 0;
}

static void seal(unsigned char* p) {
    write32(p + 52, fnv(p + 64, JOB_TOTAL_SIZE - 64));
    write32(p + 60, fnv(p, 60));
}

static void make_job(unsigned char* p) {
    unsigned index;
    zero_bytes(p, JOB_IMAGE_CAPACITY);
    write64(p, 0x31474b50424f4a46UL);
    write32(p + 8, 1);
    write32(p + 12, 64);
    write32(p + 16, JOB_TOTAL_SIZE);
    write32(p + 20, 2);
    write32(p + 24, 1);
    write64(p + 32, 0x1020304050607080UL);
    write64(p + 40, JOB_AUTHORITY);
    write32(p + 48, 9);
    write32(p + 64, 1);
    write32(p + 68, 0);
    write32(p + 72, 112);
    write32(p + 76, 16);
    write32(p + 80, 128);
    write32(p + 84, 16);
    write32(p + 88, 2);
    write32(p + 92, 1);
    write32(p + 96, 144);
    write32(p + 100, 16);
    write32(p + 104, 160);
    write32(p + 108, 16);
    for (index = 112; index < JOB_TOTAL_SIZE; index = index + 1) {
        p[index] = (unsigned char)(index * 13U + 7U);
    }
    seal(p);
}

static unsigned state_is_reset(const struct Osp3JobState* state) {
    unsigned index;
    if (state->accepted || state->step_count || state->generation ||
        state->payload_size || state->payload_digest || state->job_id ||
        state->authority) return 0;
    for (index = 0; index < OSP3_JOB_MAX_STEPS; index = index + 1) {
        if (state->input_start[index] || state->input_end[index] ||
            state->output_start[index] || state->output_end[index]) return 0;
    }
    return 1;
}

static void mix(struct JobStats* stats, unsigned value) {
    stats->digest = (stats->digest ^ value) * 0x01000193U;
}

static void record_case(
    struct JobStats* stats,
    const unsigned char* image,
    unsigned long image_size,
    unsigned long authority,
    unsigned minimum_generation,
    unsigned replay_floor,
    unsigned expected_accept,
    unsigned expected_reason
) {
    struct Osp3JobState state;
    int accepted = osp3_raw_job_admit(image, image_size, authority,
                                      minimum_generation, replay_floor, &state);
    stats->cases++;
    if (accepted) stats->accepted++; else stats->rejected++;
    if ((unsigned)accepted != expected_accept || state.reason != expected_reason)
        stats->failures++;
    if (!accepted && !state_is_reset(&state)) stats->failures++;
    if (accepted && (state.step_count == 0 || state.job_id == 0 ||
                     state.authority != authority)) stats->failures++;
    mix(stats, (unsigned)accepted | (state.reason << 1) |
               ((state.step_count << 8) ^ state.payload_digest ^
                state.generation));
}

static void record_observation(
    struct JobStats* stats,
    const unsigned char* image,
    unsigned long image_size
) {
    struct Osp3JobState state;
    int accepted = osp3_raw_job_admit(image, image_size, JOB_AUTHORITY, 7, 6,
                                      &state);
    stats->cases++;
    if (accepted) stats->accepted++; else stats->rejected++;
    if ((!accepted && !state_is_reset(&state)) ||
        (accepted && (!state.step_count || state.authority != JOB_AUTHORITY)))
        stats->failures++;
    mix(stats, (unsigned)accepted |
               ((state.reason << 1) ^ state.payload_digest ^
                state.generation));
}

static void run_mode(unsigned mode, unsigned seed, unsigned budget,
                     struct JobStats* stats) {
    unsigned char image[JOB_IMAGE_CAPACITY];
    unsigned index;
    make_job(image);

    if (mode == 0) {
        for (index = 0; index < 16; index++) {
            write64(image + 32, 0x1020304050607080UL + index);
            write32(image + 48, 9 + index);
            seal(image);
            record_case(stats, image, JOB_TOTAL_SIZE, JOB_AUTHORITY, 7, 6,
                        1, OSP3_JOB_ACCEPT);
        }
    } else if (mode == 1) {
        for (index = 0; index < JOB_TOTAL_SIZE; index++) {
            record_case(stats, image, index, JOB_AUTHORITY, 7, 6, 0,
                        index < 64 ? OSP3_JOB_SHORT : OSP3_JOB_GEOMETRY);
        }
        record_case(stats, image, JOB_TOTAL_SIZE, JOB_AUTHORITY, 7, 6, 1,
                    OSP3_JOB_ACCEPT);
    } else if (mode == 2) {
        image[0] ^= 1; image[8] = 2; image[120] ^= 1;
        record_case(stats, image, JOB_TOTAL_SIZE, JOB_AUTHORITY, 7, 6, 0,
                    OSP3_JOB_IDENT);
        make_job(image); image[8] = 2; image[120] ^= 1;
        record_case(stats, image, JOB_TOTAL_SIZE, JOB_AUTHORITY, 7, 6, 0,
                    OSP3_JOB_VERSION);
        make_job(image); write32(image + 16, 511); image[120] ^= 1;
        record_case(stats, image, JOB_TOTAL_SIZE, JOB_AUTHORITY, 7, 6, 0,
                    OSP3_JOB_GEOMETRY);
        make_job(image); image[59] ^= 1; image[120] ^= 1;
        record_case(stats, image, JOB_TOTAL_SIZE, JOB_AUTHORITY, 7, 6, 0,
                    OSP3_JOB_HEADER_CHECKSUM);
        make_job(image); image[120] ^= 1;
        record_case(stats, image, JOB_TOTAL_SIZE, 0, 99, 99, 0,
                    OSP3_JOB_PAYLOAD_CHECKSUM);
        make_job(image);
        record_case(stats, image, JOB_TOTAL_SIZE, 0, 99, 99, 0,
                    OSP3_JOB_AUTHORITY);
    } else if (mode == 3) {
        record_case(stats, image, JOB_TOTAL_SIZE, JOB_AUTHORITY, 7, 6, 1,
                    OSP3_JOB_ACCEPT);
        record_case(stats, image, JOB_TOTAL_SIZE, JOB_AUTHORITY ^ 1UL, 7, 6, 0,
                    OSP3_JOB_AUTHORITY);
        record_case(stats, image, JOB_TOTAL_SIZE, JOB_AUTHORITY, 10, 6, 0,
                    OSP3_JOB_REPLAY);
        record_case(stats, image, JOB_TOTAL_SIZE, JOB_AUTHORITY, 7, 9, 0,
                    OSP3_JOB_REPLAY);
        write32(image + 48, 0); seal(image);
        record_case(stats, image, JOB_TOTAL_SIZE, JOB_AUTHORITY, 0, 0, 0,
                    OSP3_JOB_REPLAY);
        make_job(image); write64(image + 40, 0); seal(image);
        record_case(stats, image, JOB_TOTAL_SIZE, 0, 7, 6, 0,
                    OSP3_JOB_AUTHORITY);
    } else if (mode == 4) {
        write32(image + 72, 0); seal(image);
        record_case(stats, image, JOB_TOTAL_SIZE, JOB_AUTHORITY, 7, 6, 0,
                    OSP3_JOB_STEP_RANGE);
        make_job(image); write32(image + 76, 0); seal(image);
        record_case(stats, image, JOB_TOTAL_SIZE, JOB_AUTHORITY, 7, 6, 0,
                    OSP3_JOB_STEP_RANGE);
        make_job(image); write32(image + 72, 250); seal(image);
        record_case(stats, image, JOB_TOTAL_SIZE, JOB_AUTHORITY, 7, 6, 0,
                    OSP3_JOB_STEP_RANGE);
        make_job(image); write32(image + 80, 120); seal(image);
        record_case(stats, image, JOB_TOTAL_SIZE, JOB_AUTHORITY, 7, 6, 0,
                    OSP3_JOB_STEP_OVERLAP);
        make_job(image); write32(image + 104, 120); seal(image);
        record_case(stats, image, JOB_TOTAL_SIZE, JOB_AUTHORITY, 7, 6, 0,
                    OSP3_JOB_STEP_OVERLAP);
        make_job(image); write32(image + 96, 128); seal(image);
        record_case(stats, image, JOB_TOTAL_SIZE, JOB_AUTHORITY, 7, 6, 0,
                    OSP3_JOB_STEP_OVERLAP);
    } else if (mode == 5) {
        for (index = 0; index < 64; index++) {
            make_job(image);
            if (index & 1U) {
                write32(image + 104, 120); seal(image);
                record_case(stats, image, JOB_TOTAL_SIZE, JOB_AUTHORITY, 7, 6,
                            0, OSP3_JOB_STEP_OVERLAP);
            } else {
                record_case(stats, image, JOB_TOTAL_SIZE, JOB_AUTHORITY, 7, 6,
                            1, OSP3_JOB_ACCEPT);
            }
        }
    } else {
        unsigned value = seed;
        for (index = 0; index < budget; index++) {
            unsigned offset;
            make_job(image);
            value = value * 1664525U + 1013904223U;
            offset = value % JOB_TOTAL_SIZE;
            image[offset] ^= (unsigned char)(1U << ((value >> 24) & 7U));
            if ((index & 3U) == 0) seal(image);
            record_observation(stats, image, JOB_TOTAL_SIZE);
        }
    }
}

int main(void) {
    struct JobStats stats = {0, 0, 0, 0, 0x811c9dc5U};
    run_mode(OSP3_JOB_MODE, OSP3_JOB_SEED, OSP3_JOB_CASE_BUDGET, &stats);
    printf("OSP3 raw-job mode=%u seed=%08x budget=%u cases=%u accept=%u "
           "reject=%u failures=%u digest=%u\n",
           (unsigned)OSP3_JOB_MODE, (unsigned)OSP3_JOB_SEED,
           (unsigned)OSP3_JOB_CASE_BUDGET, stats.cases, stats.accepted,
           stats.rejected, stats.failures, stats.digest);
    return stats.failures == 0 ? 0 : 1;
}
