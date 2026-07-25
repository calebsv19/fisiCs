// SPDX-License-Identifier: Apache-2.0

typedef unsigned long osp2_u64;

#define OSP2_JOB_OK 0UL
#define OSP2_JOB_RESOURCE 2UL
#define OSP2_JOB_MAGIC_ERROR 11UL
#define OSP2_JOB_TYPE_ERROR 12UL
#define OSP2_JOB_CHECKSUM_ERROR 13UL
#define OSP2_JOB_ID_ERROR 14UL
#define OSP2_JOB_RESULT_ERROR 15UL
#define OSP2_JOB_COMPILER_ERROR 16UL

#define OSP2_JOB_MAGIC 0x00424A3331554445UL
#define OSP2_JOB_ID 0xED13000000000001UL
#define OSP2_JOB_RESULT 0x6EC4E5DB9E1056CFUL
#define OSP2_JOB_COMPILER 0x1E3C373BAF48FAF7UL

osp2_u64 osp2_job_admit(
    osp2_u64 magic,
    osp2_u64 version,
    osp2_u64 job_type,
    osp2_u64 computed_checksum,
    osp2_u64 stored_checksum,
    osp2_u64 directory_checksum,
    osp2_u64 job_id,
    osp2_u64 fixed_steps,
    osp2_u64 requested_workers,
    osp2_u64 requested_pages,
    osp2_u64 checkpoint_interval,
    osp2_u64 flags,
    osp2_u64 expected_result,
    osp2_u64 compiler_id,
    osp2_u64 lesson_id,
    osp2_u64 cpu_count,
    osp2_u64 free_pages,
    osp2_u64 xsave_size
) {
    if (magic != OSP2_JOB_MAGIC) {
        return OSP2_JOB_MAGIC_ERROR;
    }
    if (version != 1 || job_type != 1) {
        return OSP2_JOB_TYPE_ERROR;
    }
    if (computed_checksum != stored_checksum || directory_checksum == 0) {
        return OSP2_JOB_CHECKSUM_ERROR;
    }
    if (job_id != OSP2_JOB_ID || fixed_steps != 64) {
        return OSP2_JOB_ID_ERROR;
    }
    if (checkpoint_interval != 64 || flags != 1 ||
        expected_result != OSP2_JOB_RESULT) {
        return OSP2_JOB_RESULT_ERROR;
    }
    if (compiler_id != OSP2_JOB_COMPILER || lesson_id != 2) {
        return OSP2_JOB_COMPILER_ERROR;
    }
    if (requested_workers != 2 || cpu_count < 2 ||
        requested_pages != 2 || free_pages < 2 || xsave_size < 832) {
        return OSP2_JOB_RESOURCE;
    }
    return OSP2_JOB_OK;
}
