#include "15__probe_abi_wave129_units_tagged_payload_preservation.h"
#include <string.h>

#ifndef __FISICS__
static double fisics_convert_unit(double value, const char* target_unit) {
    if (strcmp(target_unit, "meter") == 0) return value * 0.3048;
    if (strcmp(target_unit, "joule") == 0) return value * 3600.0;
    return value;
}
#endif

static unsigned rotl32(unsigned value, unsigned shift) {
    shift &= 31u;
    if (shift == 0u) {
        return value;
    }
    return (value << shift) | (value >> (32u - shift));
}

struct Wave129AbiUnitPayload wave129_abi_seed(double feet, double wh, unsigned seed) {
    double distance
        [[fisics::dim(length)]]
        [[fisics::unit(foot)]] = feet;
    double distance_m
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = fisics_convert_unit(distance, "meter");

    double reserve
        [[fisics::dim(energy)]]
        [[fisics::unit(watt_hour)]] = wh;
    double reserve_j
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = fisics_convert_unit(reserve, "joule");

    struct Wave129AbiUnitPayload payload;
    payload.lanes[0] = distance_m;
    payload.lanes[1] = distance_m * 0.5;
    payload.lanes[2] = distance_m * 3.0 + distance_m * 0.125;
    payload.bits.named.lo = seed * 17u + 5u;
    payload.bits.named.hi = rotl32(seed ^ 0x45d9f3bu ^ (unsigned)reserve_j, seed + 7u);
    payload.bits.named.tag = seed * 2654435761u;
    payload.bits.named.mask = payload.bits.named.lo ^ payload.bits.named.hi ^ payload.bits.named.tag;
    payload.route = seed % 3u;
    return payload;
}

struct Wave129AbiUnitPayload wave129_abi_mix(struct Wave129AbiUnitPayload payload, unsigned step) {
    unsigned slot = (payload.route + step) % 3u;
    payload.lanes[slot] += payload.lanes[(slot + 1u) % 3u] * 0.125 + (double)(step & 7u);
    payload.bits.words[slot & 3u] ^= rotl32(payload.bits.named.mask + step, slot + 5u);
    payload.bits.named.mask = payload.bits.named.lo ^ rotl32(payload.bits.named.hi, step + 3u) ^ payload.bits.named.tag;
    payload.route = (payload.route + step + (payload.bits.named.mask & 3u)) % 3u;
    return payload;
}

unsigned wave129_abi_fold(struct Wave129AbiUnitPayload payload, unsigned salt) {
    unsigned acc = salt ^ payload.bits.named.mask ^ payload.route;
    unsigned i;

    for (i = 0u; i < 3u; ++i) {
        unsigned scaled = (unsigned)(payload.lanes[i] * (double)(100 + i * 37u));
        acc ^= scaled + payload.bits.words[i];
        acc = rotl32(acc, i * 7u + 5u);
    }

    return acc ^ payload.bits.words[3] ^ (acc >> 13u);
}
