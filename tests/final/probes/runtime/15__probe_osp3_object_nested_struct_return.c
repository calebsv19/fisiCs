typedef unsigned long osp3_u64;

struct osp3_pair {
    osp3_u64 value;
    osp3_u64 generation;
};

struct osp3_envelope {
    struct osp3_pair left;
    struct osp3_pair right;
    osp3_u64 status;
};

struct osp3_envelope osp3_object_nested_struct_return(
    osp3_u64 seed,
    osp3_u64 generation
) {
    struct osp3_envelope result;
    result.left.value = seed;
    result.left.generation = generation;
    result.right.value = seed ^ 0xd1b54a32d192ed03UL;
    result.right.generation = generation + 1;
    result.status = seed == 0 ? 1 : 0;
    return result;
}
