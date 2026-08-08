/*
 * Source-derived from os-dev simulation_kernel.c at immutable EDU-48 tag
 * edu-48-signed-simulation-bundle-execution-abi (commit a5c7c4d).
 *
 * EDU-48 itself adds host-side signed-bundle policy. This narrow C model
 * preserves the frozen program-one/program-two selection boundary only; it
 * does not model signature verification, artifact storage, or guest loading.
 */
typedef unsigned char edu48_u8;
typedef unsigned int edu48_u32;
typedef unsigned long long edu48_u64;

extern int edu32_workload_valid(const edu48_u8 *, edu48_u64);
extern edu48_u64 edu12_reduce_result(edu48_u64, edu48_u64, edu48_u64);

static edu48_u64 edu48_read64(const edu48_u8 *bytes) {
    edu48_u64 value = 0ULL;
    edu48_u32 index = 0U;
    while (index < 8U) {
        value |= ((edu48_u64)bytes[index]) << (index * 8U);
        index = index + 1U;
    }
    return value;
}

static double edu48_from_bits(edu48_u64 bits) {
    union { edu48_u64 bits; double value; } conversion;
    conversion.bits = bits;
    return conversion.value;
}

static edu48_u64 edu48_to_bits(double value) {
    union { edu48_u64 bits; double value; } conversion;
    conversion.value = value;
    return conversion.bits;
}

static double edu48_damped_partition(double position, double velocity,
                                     double acceleration, edu48_u64 steps) {
    edu48_u64 step = 0ULL;
    while (step < steps) {
        velocity = (velocity + acceleration) * 0.5;
        position = position + velocity;
        step = step + 1ULL;
    }
    return position;
}

int edu48_frozen_program_valid(const edu48_u8 *bytes, edu48_u64 length,
                               edu48_u32 program_id) {
    if (program_id != 1U && program_id != 2U) {
        return 0;
    }
    return edu32_workload_valid(bytes, length);
}

edu48_u64 edu48_frozen_program_result(const edu48_u8 *bytes,
                                      edu48_u64 length,
                                      edu48_u32 program_id) {
    edu48_u64 steps;
    edu48_u64 seed;
    edu48_u64 result0;
    edu48_u64 result1;
    if (!edu48_frozen_program_valid(bytes, length, program_id)) {
        return 0ULL;
    }
    if (program_id == 1U) {
        return edu48_read64(bytes + 96U);
    }
    steps = edu48_read64(bytes + 64U);
    seed = edu48_read64(bytes + 72U);
    result0 = edu48_to_bits(edu48_damped_partition(
        edu48_from_bits(edu48_read64(bytes + 16U)),
        edu48_from_bits(edu48_read64(bytes + 24U)),
        edu48_from_bits(edu48_read64(bytes + 32U)), steps));
    result1 = edu48_to_bits(edu48_damped_partition(
        edu48_from_bits(edu48_read64(bytes + 40U)),
        edu48_from_bits(edu48_read64(bytes + 48U)),
        edu48_from_bits(edu48_read64(bytes + 56U)), steps));
    return edu12_reduce_result(result0, result1, seed);
}
