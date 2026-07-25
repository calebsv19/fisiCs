// SPDX-License-Identifier: Apache-2.0

#ifndef OSP2_JOB_ADMISSION_VECTORS_H
#define OSP2_JOB_ADMISSION_VECTORS_H

typedef unsigned long osp2_u64;

#define OSP2_JOB_OK 0UL
#define OSP2_JOB_RESOURCE 2UL
#define OSP2_JOB_MAGIC_ERROR 11UL
#define OSP2_JOB_TYPE_ERROR 12UL
#define OSP2_JOB_CHECKSUM_ERROR 13UL
#define OSP2_JOB_ID_ERROR 14UL
#define OSP2_JOB_RESULT_ERROR 15UL
#define OSP2_JOB_COMPILER_ERROR 16UL
#define OSP2_JOB_VECTOR_COUNT 27UL
#define OSP2_JOB_CORPUS_ID "job27-v1"

#define OSP2_JOB_MAGIC 0x00424A3331554445UL
#define OSP2_JOB_ID 0xED13000000000001UL
#define OSP2_JOB_RESULT 0x6EC4E5DB9E1056CFUL
#define OSP2_JOB_COMPILER 0x1E3C373BAF48FAF7UL
#define OSP2_JOB_BASE \
    OSP2_JOB_MAGIC, 1, 1, 0xA5A55A5AUL, 0xA5A55A5AUL, 1, \
    OSP2_JOB_ID, 64, 2, 2, 64, 1, OSP2_JOB_RESULT, \
    OSP2_JOB_COMPILER, 2, 2, 2, 832

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
);

static osp2_u64 osp2_job_expect(osp2_u64 actual, osp2_u64 expected) {
    return actual == expected ? 0 : 1;
}

static osp2_u64 osp2_job_run_vectors(void) {
    osp2_u64 failures = 0;

    failures += osp2_job_expect(osp2_job_admit(OSP2_JOB_BASE), OSP2_JOB_OK);
    failures += osp2_job_expect(
        osp2_job_admit(
            0, 1, 1, 1, 1, 1, OSP2_JOB_ID, 64, 2, 2, 64, 1,
            OSP2_JOB_RESULT, OSP2_JOB_COMPILER, 2, 2, 2, 832
        ),
        OSP2_JOB_MAGIC_ERROR
    );
    failures += osp2_job_expect(
        osp2_job_admit(
            OSP2_JOB_MAGIC, 2, 1, 1, 1, 1, OSP2_JOB_ID, 64, 2, 2, 64, 1,
            OSP2_JOB_RESULT, OSP2_JOB_COMPILER, 2, 2, 2, 832
        ),
        OSP2_JOB_TYPE_ERROR
    );
    failures += osp2_job_expect(
        osp2_job_admit(
            OSP2_JOB_MAGIC, 1, 2, 1, 1, 1, OSP2_JOB_ID, 64, 2, 2, 64, 1,
            OSP2_JOB_RESULT, OSP2_JOB_COMPILER, 2, 2, 2, 832
        ),
        OSP2_JOB_TYPE_ERROR
    );
    failures += osp2_job_expect(
        osp2_job_admit(
            OSP2_JOB_MAGIC, 1, 1, 1, 2, 1, OSP2_JOB_ID, 64, 2, 2, 64, 1,
            OSP2_JOB_RESULT, OSP2_JOB_COMPILER, 2, 2, 2, 832
        ),
        OSP2_JOB_CHECKSUM_ERROR
    );
    failures += osp2_job_expect(
        osp2_job_admit(
            OSP2_JOB_MAGIC, 1, 1, 1, 1, 0, OSP2_JOB_ID, 64, 2, 2, 64, 1,
            OSP2_JOB_RESULT, OSP2_JOB_COMPILER, 2, 2, 2, 832
        ),
        OSP2_JOB_CHECKSUM_ERROR
    );
    failures += osp2_job_expect(
        osp2_job_admit(
            OSP2_JOB_MAGIC, 1, 1, 1, 1, 1, 0, 64, 2, 2, 64, 1,
            OSP2_JOB_RESULT, OSP2_JOB_COMPILER, 2, 2, 2, 832
        ),
        OSP2_JOB_ID_ERROR
    );
    failures += osp2_job_expect(
        osp2_job_admit(
            OSP2_JOB_MAGIC, 1, 1, 1, 1, 1, OSP2_JOB_ID, 63, 2, 2, 64, 1,
            OSP2_JOB_RESULT, OSP2_JOB_COMPILER, 2, 2, 2, 832
        ),
        OSP2_JOB_ID_ERROR
    );
    failures += osp2_job_expect(
        osp2_job_admit(
            OSP2_JOB_MAGIC, 1, 1, 1, 1, 1, OSP2_JOB_ID, 64, 2, 2, 63, 1,
            OSP2_JOB_RESULT, OSP2_JOB_COMPILER, 2, 2, 2, 832
        ),
        OSP2_JOB_RESULT_ERROR
    );
    failures += osp2_job_expect(
        osp2_job_admit(
            OSP2_JOB_MAGIC, 1, 1, 1, 1, 1, OSP2_JOB_ID, 64, 2, 2, 64, 0,
            OSP2_JOB_RESULT, OSP2_JOB_COMPILER, 2, 2, 2, 832
        ),
        OSP2_JOB_RESULT_ERROR
    );
    failures += osp2_job_expect(
        osp2_job_admit(
            OSP2_JOB_MAGIC, 1, 1, 1, 1, 1, OSP2_JOB_ID, 64, 2, 2, 64, 1,
            0, OSP2_JOB_COMPILER, 2, 2, 2, 832
        ),
        OSP2_JOB_RESULT_ERROR
    );
    failures += osp2_job_expect(
        osp2_job_admit(
            OSP2_JOB_MAGIC, 1, 1, 1, 1, 1, OSP2_JOB_ID, 64, 2, 2, 64, 1,
            OSP2_JOB_RESULT, 0, 2, 2, 2, 832
        ),
        OSP2_JOB_COMPILER_ERROR
    );
    failures += osp2_job_expect(
        osp2_job_admit(
            OSP2_JOB_MAGIC, 1, 1, 1, 1, 1, OSP2_JOB_ID, 64, 2, 2, 64, 1,
            OSP2_JOB_RESULT, OSP2_JOB_COMPILER, 1, 2, 2, 832
        ),
        OSP2_JOB_COMPILER_ERROR
    );

    failures += osp2_job_expect(
        osp2_job_admit(
            OSP2_JOB_MAGIC, 1, 1, 1, 1, 1, OSP2_JOB_ID, 64, 1, 2, 64, 1,
            OSP2_JOB_RESULT, OSP2_JOB_COMPILER, 2, 2, 2, 832
        ),
        OSP2_JOB_RESOURCE
    );
    failures += osp2_job_expect(
        osp2_job_admit(
            OSP2_JOB_MAGIC, 1, 1, 1, 1, 1, OSP2_JOB_ID, 64, 3, 2, 64, 1,
            OSP2_JOB_RESULT, OSP2_JOB_COMPILER, 2, 3, 2, 832
        ),
        OSP2_JOB_RESOURCE
    );
    failures += osp2_job_expect(
        osp2_job_admit(
            OSP2_JOB_MAGIC, 1, 1, 1, 1, 1, OSP2_JOB_ID, 64, 2, 2, 64, 1,
            OSP2_JOB_RESULT, OSP2_JOB_COMPILER, 2, 1, 2, 832
        ),
        OSP2_JOB_RESOURCE
    );
    failures += osp2_job_expect(
        osp2_job_admit(
            OSP2_JOB_MAGIC, 1, 1, 1, 1, 1, OSP2_JOB_ID, 64, 2, 1, 64, 1,
            OSP2_JOB_RESULT, OSP2_JOB_COMPILER, 2, 2, 2, 832
        ),
        OSP2_JOB_RESOURCE
    );
    failures += osp2_job_expect(
        osp2_job_admit(
            OSP2_JOB_MAGIC, 1, 1, 1, 1, 1, OSP2_JOB_ID, 64, 2, 3, 64, 1,
            OSP2_JOB_RESULT, OSP2_JOB_COMPILER, 2, 2, 3, 832
        ),
        OSP2_JOB_RESOURCE
    );
    failures += osp2_job_expect(
        osp2_job_admit(
            OSP2_JOB_MAGIC, 1, 1, 1, 1, 1, OSP2_JOB_ID, 64, 2, 2, 64, 1,
            OSP2_JOB_RESULT, OSP2_JOB_COMPILER, 2, 2, 1, 832
        ),
        OSP2_JOB_RESOURCE
    );
    failures += osp2_job_expect(
        osp2_job_admit(
            OSP2_JOB_MAGIC, 1, 1, 1, 1, 1, OSP2_JOB_ID, 64, 2, 2, 64, 1,
            OSP2_JOB_RESULT, OSP2_JOB_COMPILER, 2, 2, 2, 831
        ),
        OSP2_JOB_RESOURCE
    );

    failures += osp2_job_expect(
        osp2_job_admit(
            OSP2_JOB_MAGIC, 1, 1, 1, 1, 1, OSP2_JOB_ID, 64, 2, 2, 64, 1,
            OSP2_JOB_RESULT, OSP2_JOB_COMPILER, 2, 8, 2, 832
        ),
        OSP2_JOB_OK
    );
    failures += osp2_job_expect(
        osp2_job_admit(
            OSP2_JOB_MAGIC, 1, 1, 1, 1, 1, OSP2_JOB_ID, 64, 2, 2, 64, 1,
            OSP2_JOB_RESULT, OSP2_JOB_COMPILER, 2, 2, 64, 832
        ),
        OSP2_JOB_OK
    );
    failures += osp2_job_expect(
        osp2_job_admit(
            OSP2_JOB_MAGIC, 1, 1, 1, 1, 1, OSP2_JOB_ID, 64, 2, 2, 64, 1,
            OSP2_JOB_RESULT, OSP2_JOB_COMPILER, 2, 2, 2, 4096
        ),
        OSP2_JOB_OK
    );

    failures += osp2_job_expect(
        osp2_job_admit(
            0, 1, 1, 1, 2, 0, 0, 0, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0
        ),
        OSP2_JOB_MAGIC_ERROR
    );
    failures += osp2_job_expect(
        osp2_job_admit(
            OSP2_JOB_MAGIC, 1, 1, 1, 2, 1, OSP2_JOB_ID, 64, 3, 3, 64, 1,
            OSP2_JOB_RESULT, OSP2_JOB_COMPILER, 2, 0, 0, 0
        ),
        OSP2_JOB_CHECKSUM_ERROR
    );
    failures += osp2_job_expect(
        osp2_job_admit(
            OSP2_JOB_MAGIC, 1, 1, 1, 1, 1, OSP2_JOB_ID, 64, 3, 3, 64, 1,
            OSP2_JOB_RESULT, 0, 2, 0, 0, 0
        ),
        OSP2_JOB_COMPILER_ERROR
    );
    failures += osp2_job_expect(
        osp2_job_admit(
            OSP2_JOB_MAGIC, 1, 1, 1, 1, 1, OSP2_JOB_ID, 64, 2, 2, 64, 1,
            OSP2_JOB_RESULT, OSP2_JOB_COMPILER, 2, 1, 1, 831
        ),
        OSP2_JOB_RESOURCE
    );
    return failures;
}

#endif
