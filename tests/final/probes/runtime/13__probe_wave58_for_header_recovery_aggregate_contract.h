#ifndef FISICS_PROBE_WAVE58_FOR_HEADER_RECOVERY_AGGREGATE_CONTRACT_H
#define FISICS_PROBE_WAVE58_FOR_HEADER_RECOVERY_AGGREGATE_CONTRACT_H

struct wave58_payload {
    long lane[4];
    struct {
        int left;
        int right;
    } meta;
};

struct wave58_payload wave58_build_payload(int seed);

#endif
