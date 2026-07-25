// SPDX-License-Identifier: Apache-2.0

#ifndef OSP3_RAW_JOB_POLICY_H
#define OSP3_RAW_JOB_POLICY_H

typedef unsigned char osp3_job_u8;
typedef unsigned int osp3_job_u32;
typedef unsigned long osp3_job_u64;

#define OSP3_JOB_MAX_STEPS 4U

enum Osp3JobReason {
    OSP3_JOB_ACCEPT = 0,
    OSP3_JOB_SHORT = 1,
    OSP3_JOB_IDENT = 2,
    OSP3_JOB_VERSION = 3,
    OSP3_JOB_GEOMETRY = 4,
    OSP3_JOB_HEADER_CHECKSUM = 5,
    OSP3_JOB_PAYLOAD_CHECKSUM = 6,
    OSP3_JOB_AUTHORITY = 7,
    OSP3_JOB_REPLAY = 8,
    OSP3_JOB_STEP_RANGE = 9,
    OSP3_JOB_STEP_OVERLAP = 10
};

struct Osp3JobState {
    osp3_job_u32 accepted;
    osp3_job_u32 reason;
    osp3_job_u32 step_count;
    osp3_job_u32 generation;
    osp3_job_u32 payload_size;
    osp3_job_u32 payload_digest;
    osp3_job_u64 job_id;
    osp3_job_u64 authority;
    osp3_job_u32 input_start[OSP3_JOB_MAX_STEPS];
    osp3_job_u32 input_end[OSP3_JOB_MAX_STEPS];
    osp3_job_u32 output_start[OSP3_JOB_MAX_STEPS];
    osp3_job_u32 output_end[OSP3_JOB_MAX_STEPS];
};

int osp3_raw_job_admit(
    const osp3_job_u8* image,
    osp3_job_u64 image_size,
    osp3_job_u64 expected_authority,
    osp3_job_u32 minimum_generation,
    osp3_job_u32 replay_floor,
    struct Osp3JobState* state
);

#endif
