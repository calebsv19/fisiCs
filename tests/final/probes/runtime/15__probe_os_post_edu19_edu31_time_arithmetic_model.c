/*
 * Compiler-side arithmetic mirror derived from immutable os-dev tag
 * edu-31-calibrated-monotonic-time, commit
 * 0d10b3df7dbd0c758e336d8f56f538e2b6afb0d3.
 *
 * Authoritative implementation: time64.asm. EDU-31 generated C only changes
 * Wire-v7 admission. This hardware-blind model probes calibration bounds,
 * divide-first conversion, one-wrap subtraction, and monotonic rejection.
 * CPUID, PIT, LFENCE/RDTSC, BSP scope, and publication remain OS-owned.
 */
typedef unsigned int edu31_u32;
typedef unsigned long long edu31_u64;

#define EDU31_U64_MAX (~0ULL)

enum {
    EDU31_PIT_INPUT_HZ = 1193182,
    EDU31_PIT_DIVISOR = 11931,
    EDU31_CALIBRATION_TICKS = 8,
    EDU31_MIN_HZ = 1000000,
    EDU31_NSEC_PER_SEC = 1000000000,
    EDU31_SNAPSHOT_FLAGS = 0x00070101
};

edu31_u64 edu31_calibration_window_hz(edu31_u64 tsc_delta) {
    edu31_u64 denominator =
        (edu31_u64)EDU31_PIT_DIVISOR * EDU31_CALIBRATION_TICKS;
    if (tsc_delta == 0 ||
        tsc_delta > EDU31_U64_MAX / EDU31_PIT_INPUT_HZ) return 0;
    return (tsc_delta * EDU31_PIT_INPUT_HZ) / denominator;
}

edu31_u64 edu31_calibration_pair_hz(
    edu31_u64 first_hz, edu31_u64 second_hz) {
    edu31_u64 difference;
    edu31_u64 sum;
    edu31_u64 average;
    if (first_hz >= second_hz) difference = first_hz - second_hz;
    else difference = second_hz - first_hz;
    if (difference > (first_hz >> 1)) return 0;
    sum = first_hz + second_hz;
    if (sum < first_hz) return 0;
    average = sum >> 1;
    if (average < EDU31_MIN_HZ || average > 10000000000ULL) return 0;
    return average;
}

edu31_u64 edu31_delta_to_ns(
    edu31_u64 delta, edu31_u64 frequency_hz) {
    edu31_u64 seconds;
    edu31_u64 remainder;
    edu31_u64 fractional;
    edu31_u64 whole;
    if (frequency_hz < EDU31_MIN_HZ ||
        frequency_hz > 10000000000ULL) return EDU31_U64_MAX;
    seconds = delta / frequency_hz;
    remainder = delta % frequency_hz;
    if (remainder > EDU31_U64_MAX / EDU31_NSEC_PER_SEC) {
        return EDU31_U64_MAX;
    }
    fractional =
        (remainder * EDU31_NSEC_PER_SEC) / frequency_hz;
    if (seconds > EDU31_U64_MAX / EDU31_NSEC_PER_SEC) {
        return EDU31_U64_MAX;
    }
    whole = seconds * EDU31_NSEC_PER_SEC;
    if (fractional > EDU31_U64_MAX - whole) return EDU31_U64_MAX;
    return whole + fractional;
}

edu31_u64 edu31_raw_delta(
    edu31_u64 current_tsc, edu31_u64 epoch_tsc) {
    return current_tsc - epoch_tsc;
}

int edu31_read_admissible(
    edu31_u64 last_ticks,
    edu31_u64 last_ns,
    edu31_u64 current_ticks,
    edu31_u64 current_ns) {
    return current_ticks >= last_ticks && current_ns >= last_ns;
}

edu31_u32 edu31_snapshot_flags(void) {
    return EDU31_SNAPSHOT_FLAGS;
}
