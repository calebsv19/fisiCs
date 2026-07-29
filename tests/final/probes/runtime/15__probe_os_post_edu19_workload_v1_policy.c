/*
 * Source: os-dev simulation_kernel.c at immutable tag
 * edu-32-generation-pinned-workload-materialization, commit 274f955.
 * Origin SHA-256: 7c1d57bb3020705a4796fc7a96b0153536deb4daac9dd4a9dfb9663a88ccec15.
 * The implementation below is unchanged from that snapshot.
 */
typedef unsigned long long edu12_u64;
typedef unsigned int edu32_u32;
typedef unsigned short edu32_u16;
typedef unsigned char edu32_u8;

double edu12_simulate_partition(double position,
                                double velocity,
                                double acceleration,
                                edu12_u64 steps) {
    edu12_u64 step = 0ULL;
    while (step < steps) {
        velocity = velocity + acceleration;
        position = position + velocity;
        step = step + 1ULL;
    }
    return position;
}

edu12_u64 edu12_reduce_result(edu12_u64 left_bits,
                              edu12_u64 right_bits,
                              edu12_u64 seed) {
    edu12_u64 value = seed ^ left_bits;
    value = (value << 13ULL) | (value >> 51ULL);
    value = value + right_bits;
    return value ^ 0xD1B54A32D192ED03ULL;
}

#define EDU32_WORKLOAD_BYTES 104U
#define EDU32_WORKLOAD_STEPS_MAX 65536ULL

static edu32_u16 edu32_read16(const edu32_u8 *bytes) {
    return (edu32_u16)((edu32_u16)bytes[0]
        | ((edu32_u16)bytes[1] << 8U));
}

static edu32_u32 edu32_read32(const edu32_u8 *bytes) {
    return (edu32_u32)bytes[0]
        | ((edu32_u32)bytes[1] << 8U)
        | ((edu32_u32)bytes[2] << 16U)
        | ((edu32_u32)bytes[3] << 24U);
}

static edu12_u64 edu32_read64(const edu32_u8 *bytes) {
    edu12_u64 value = 0ULL;
    unsigned int index = 0U;
    while (index < 8U) {
        value |= ((edu12_u64)bytes[index]) << (index * 8U);
        index = index + 1U;
    }
    return value;
}

static int edu32_finite_double_bits(edu12_u64 bits) {
    return (bits & 0x7FF0000000000000ULL) != 0x7FF0000000000000ULL;
}

static double edu32_double_from_bits(edu12_u64 bits) {
    union {
        edu12_u64 bits;
        double value;
    } conversion;
    conversion.bits = bits;
    return conversion.value;
}

static edu12_u64 edu32_double_to_bits(double value) {
    union {
        edu12_u64 bits;
        double value;
    } conversion;
    conversion.value = value;
    return conversion.bits;
}

/*
 * Validate the complete workload-v1 object and its independently published
 * expected results. Inputs: bytes points to length readable bytes. Output:
 * 1 for a valid, self-consistent object; 0 otherwise. No state is retained.
 */
int edu32_workload_valid(const edu32_u8 *bytes, edu12_u64 length) {
    edu12_u64 position0_bits;
    edu12_u64 velocity0_bits;
    edu12_u64 acceleration0_bits;
    edu12_u64 position1_bits;
    edu12_u64 velocity1_bits;
    edu12_u64 acceleration1_bits;
    edu12_u64 steps;
    edu12_u64 seed;
    edu12_u64 result0;
    edu12_u64 result1;

    if (bytes == (const edu32_u8 *)0 || length != EDU32_WORKLOAD_BYTES) {
        return 0;
    }
    if (bytes[0] != 'E' || bytes[1] != 'D' || bytes[2] != 'U'
            || bytes[3] != '3' || bytes[4] != '2' || bytes[5] != 'W'
            || bytes[6] != '1' || bytes[7] != 0U
            || edu32_read16(bytes + 8U) != 1U
            || edu32_read16(bytes + 10U) != 1U
            || edu32_read32(bytes + 12U) != EDU32_WORKLOAD_BYTES) {
        return 0;
    }

    position0_bits = edu32_read64(bytes + 16U);
    velocity0_bits = edu32_read64(bytes + 24U);
    acceleration0_bits = edu32_read64(bytes + 32U);
    position1_bits = edu32_read64(bytes + 40U);
    velocity1_bits = edu32_read64(bytes + 48U);
    acceleration1_bits = edu32_read64(bytes + 56U);
    steps = edu32_read64(bytes + 64U);
    seed = edu32_read64(bytes + 72U);
    if (!edu32_finite_double_bits(position0_bits)
            || !edu32_finite_double_bits(velocity0_bits)
            || !edu32_finite_double_bits(acceleration0_bits)
            || !edu32_finite_double_bits(position1_bits)
            || !edu32_finite_double_bits(velocity1_bits)
            || !edu32_finite_double_bits(acceleration1_bits)
            || steps == 0ULL || steps > EDU32_WORKLOAD_STEPS_MAX) {
        return 0;
    }

    result0 = edu32_double_to_bits(edu12_simulate_partition(
        edu32_double_from_bits(position0_bits),
        edu32_double_from_bits(velocity0_bits),
        edu32_double_from_bits(acceleration0_bits),
        steps));
    result1 = edu32_double_to_bits(edu12_simulate_partition(
        edu32_double_from_bits(position1_bits),
        edu32_double_from_bits(velocity1_bits),
        edu32_double_from_bits(acceleration1_bits),
        steps));
    if (result0 != edu32_read64(bytes + 80U)
            || result1 != edu32_read64(bytes + 88U)) {
        return 0;
    }
    return edu12_reduce_result(result0, result1, seed)
        == edu32_read64(bytes + 96U);
}
