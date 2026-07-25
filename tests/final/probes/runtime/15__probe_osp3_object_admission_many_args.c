typedef unsigned long osp3_u64;

osp3_u64 osp3_object_admission_many_args(
    osp3_u64 magic,
    osp3_u64 version,
    osp3_u64 kind,
    osp3_u64 checksum,
    osp3_u64 stored_checksum,
    osp3_u64 job_id,
    osp3_u64 workers,
    osp3_u64 pages,
    osp3_u64 flags,
    osp3_u64 result,
    osp3_u64 compiler_id,
    osp3_u64 lesson_id,
    osp3_u64 cpu_count,
    osp3_u64 free_pages,
    osp3_u64 xsave_size,
    osp3_u64 queue_epoch,
    osp3_u64 owner,
    osp3_u64 generation
) {
    if (magic != 0x4f5350334a4f4231UL) return 1;
    if (version != 3 || kind != 1) return 2;
    if (checksum != stored_checksum || checksum == 0) return 3;
    if (job_id == 0 || workers == 0 || pages == 0) return 4;
    if (flags != 1 || result == 0 || compiler_id == 0 || lesson_id != 3) return 5;
    if (workers > cpu_count || pages > free_pages || xsave_size < 832) return 6;
    if (queue_epoch == 0 || owner == 0 || generation == 0) return 7;
    return 0;
}
