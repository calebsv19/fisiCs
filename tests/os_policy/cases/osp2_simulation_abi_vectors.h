// SPDX-License-Identifier: Apache-2.0

#ifndef OSP2_SIMULATION_ABI_VECTORS_H
#define OSP2_SIMULATION_ABI_VECTORS_H

typedef unsigned long osp2_u64;

#define OSP2_SIM_VECTOR_COUNT 38UL
#define OSP2_SIM_CORPUS_ID "sim38-v1"

double osp2_simulate_partition(double, double, double, osp2_u64);
osp2_u64 osp2_reduce_result(osp2_u64, osp2_u64, osp2_u64);

static osp2_u64 osp2_sim_expect_u64(osp2_u64 actual, osp2_u64 expected) {
    return actual == expected ? 0 : 1;
}

static osp2_u64 osp2_sim_expect_double(double actual, double expected) {
    return actual == expected ? 0 : 1;
}

static osp2_u64 osp2_sim_run_vectors(void) {
    osp2_u64 failures = 0;

    failures += osp2_sim_expect_double(
        osp2_simulate_partition(0.0, 0.0, 0.0, 0), 0.0
    );
    failures += osp2_sim_expect_double(
        osp2_simulate_partition(1.0, 0.25, 0.03125, 0), 1.0
    );
    failures += osp2_sim_expect_double(
        osp2_simulate_partition(-1.0, -0.5, 0.125, 0), -1.0
    );
    failures += osp2_sim_expect_double(
        osp2_simulate_partition(0.0, 0.0, 0.0, 1), 0.0
    );
    failures += osp2_sim_expect_double(
        osp2_simulate_partition(0.0, 1.0, 0.0, 1), 1.0
    );
    failures += osp2_sim_expect_double(
        osp2_simulate_partition(1.0, 1.0, 0.0, 1), 2.0
    );
    failures += osp2_sim_expect_double(
        osp2_simulate_partition(0.0, 0.0, 1.0, 1), 1.0
    );
    failures += osp2_sim_expect_double(
        osp2_simulate_partition(1.0, -1.0, 0.0, 1), 0.0
    );
    failures += osp2_sim_expect_double(
        osp2_simulate_partition(0.0, 0.5, 0.25, 1), 0.75
    );
    failures += osp2_sim_expect_double(
        osp2_simulate_partition(0.0, 0.0, 1.0, 2), 3.0
    );
    failures += osp2_sim_expect_double(
        osp2_simulate_partition(0.0, 1.0, 0.0, 2), 2.0
    );
    failures += osp2_sim_expect_double(
        osp2_simulate_partition(1.0, -1.0, 0.5, 2), 0.5
    );
    failures += osp2_sim_expect_double(
        osp2_simulate_partition(0.0, 0.25, 0.125, 4), 2.25
    );
    failures += osp2_sim_expect_double(
        osp2_simulate_partition(1.0, 0.25, 0.03125, 63), 79.75
    );
    failures += osp2_sim_expect_double(
        osp2_simulate_partition(1.0, 0.25, 0.03125, 64), 82.0
    );
    failures += osp2_sim_expect_double(
        osp2_simulate_partition(1.0, 0.25, 0.03125, 65), 84.28125
    );
    failures += osp2_sim_expect_double(
        osp2_simulate_partition(2.0, -0.125, 0.015625, 63), 25.625
    );
    failures += osp2_sim_expect_double(
        osp2_simulate_partition(2.0, -0.125, 0.015625, 64), 26.5
    );
    failures += osp2_sim_expect_double(
        osp2_simulate_partition(2.0, -0.125, 0.015625, 65), 27.390625
    );
    failures += osp2_sim_expect_double(
        osp2_simulate_partition(1.0, 0.25, 0.03125, 64), 82.0
    );
    failures += osp2_sim_expect_double(
        osp2_simulate_partition(1.0, 0.25, 0.03125, 64), 82.0
    );

    failures += osp2_sim_expect_u64(
        osp2_reduce_result(0, 0, 0), 0xD1B54A32D192ED03UL
    );
    failures += osp2_sim_expect_u64(
        osp2_reduce_result(1, 0, 0), 0xD1B54A32D192CD03UL
    );
    failures += osp2_sim_expect_u64(
        osp2_reduce_result(0, 1, 0), 0xD1B54A32D192ED02UL
    );
    failures += osp2_sim_expect_u64(
        osp2_reduce_result(0, 0, 1), 0xD1B54A32D192CD03UL
    );
    failures += osp2_sim_expect_u64(
        osp2_reduce_result(~0UL, 0, 0), 0x2E4AB5CD2E6D12FCUL
    );
    failures += osp2_sim_expect_u64(
        osp2_reduce_result(0, ~0UL, 0), 0x2E4AB5CD2E6D12FCUL
    );
    failures += osp2_sim_expect_u64(
        osp2_reduce_result(0, 0, ~0UL), 0x2E4AB5CD2E6D12FCUL
    );
    failures += osp2_sim_expect_u64(
        osp2_reduce_result(1, 2, 3), 0xD1B54A32D192AD01UL
    );
    failures += osp2_sim_expect_u64(
        osp2_reduce_result(
            0x4054800000000000UL,
            0x403A800000000000UL,
            0x9E3779B97F4A7C15UL
        ),
        0x6EC4E5DB9E1056CFUL
    );
    failures += osp2_sim_expect_u64(
        osp2_reduce_result(0x8000000000000000UL, 0, 0),
        0xD1B54A32D192FD03UL
    );
    failures += osp2_sim_expect_u64(
        osp2_reduce_result(0, 0x8000000000000000UL, 0),
        0x51B54A32D192ED03UL
    );
    failures += osp2_sim_expect_u64(
        osp2_reduce_result(0, 0, 0x8000000000000000UL),
        0xD1B54A32D192FD03UL
    );
    failures += osp2_sim_expect_u64(
        osp2_reduce_result(
            0x0123456789ABCDEFUL,
            0xFEDCBA9876543210UL,
            0x0F0E0D0C0B0A0908UL
        ),
        0x765F60DE7F63FED6UL
    );
    failures += osp2_sim_expect_u64(
        osp2_reduce_result(
            0xAAAAAAAAAAAAAAAAUL,
            0x5555555555555555UL,
            0x1111111111111111UL
        ),
        0x1D7986FE1D5E21CFUL
    );
    failures += osp2_sim_expect_u64(
        osp2_reduce_result(
            0x5555555555555555UL,
            0xAAAAAAAAAAAAAAAAUL,
            0xEEEEEEEEEEEEEEEEUL
        ),
        0xF3976810F3B0CF22UL
    );
    failures += osp2_sim_expect_u64(
        osp2_reduce_result(
            0x4054800000000000UL,
            0x403A800000000000UL,
            0x9E3779B97F4A7C15UL
        ),
        0x6EC4E5DB9E1056CFUL
    );
    failures += osp2_sim_expect_u64(
        osp2_reduce_result(
            0x4054800000000000UL,
            0x403A800000000000UL,
            0x9E3779B97F4A7C15UL
        ),
        0x6EC4E5DB9E1056CFUL
    );

    return failures;
}

#endif
