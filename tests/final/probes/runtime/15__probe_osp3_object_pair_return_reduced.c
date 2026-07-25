typedef unsigned long osp3_u64;

struct osp3_pair_return {
    osp3_u64 value;
    osp3_u64 generation;
};

struct osp3_pair_return osp3_object_pair_return_reduced(
    osp3_u64 seed,
    osp3_u64 generation
) {
    struct osp3_pair_return result;
    result.value = seed ^ 0xd1b54a32d192ed03UL;
    result.generation = generation + 1;
    return result;
}
