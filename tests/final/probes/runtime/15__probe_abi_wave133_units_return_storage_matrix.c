#include <stdio.h>
#include <string.h>

#ifndef __FISICS__
static double fisics_convert_unit(double value, const char* target_unit) {
    if (strcmp(target_unit, "meter") == 0) return value * 0.3048;
    if (strcmp(target_unit, "joule") == 0) return value * 3600.0;
    if (strcmp(target_unit, "coulomb") == 0) return value * 3.6;
    if (strcmp(target_unit, "second") == 0) return value * 0.001;
    return value;
}
#endif

union Wave133StoragePayload {
    unsigned words[5];
    struct {
        unsigned lane;
        unsigned route;
        unsigned tag;
        unsigned mix;
        unsigned owner;
    } named;
};

struct Wave133StorageCell {
    double meters;
    double joules;
    double coulombs;
    double seconds;
    union Wave133StoragePayload payload;
};

struct Wave133StorageFrame {
    struct Wave133StorageCell cells[2][2];
    union Wave133StoragePayload footer;
    unsigned epoch;
};

static unsigned rotl32(unsigned value, unsigned shift) {
    shift &= 31u;
    if (shift == 0u) {
        return value;
    }
    return (value << shift) | (value >> (32u - shift));
}

static struct Wave133StorageCell make_cell(double feet,
                                           double watt_hours,
                                           double milliamp_hours,
                                           double millis,
                                           unsigned seed) {
    double distance_ft
        [[fisics::dim(length)]]
        [[fisics::unit(foot)]] = feet;
    double distance_m
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = fisics_convert_unit(distance_ft, "meter");

    double reserve_wh
        [[fisics::dim(energy)]]
        [[fisics::unit(watt_hour)]] = watt_hours;
    double reserve_j
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = fisics_convert_unit(reserve_wh, "joule");

    double charge_mah
        [[fisics::dim(charge)]]
        [[fisics::unit(milliampere_hour)]] = milliamp_hours;
    double charge_c
        [[fisics::dim(charge)]]
        [[fisics::unit(coulomb)]] = fisics_convert_unit(charge_mah, "coulomb");

    double window_ms
        [[fisics::dim(time)]]
        [[fisics::unit(millisecond)]] = millis;
    double window_s
        [[fisics::dim(time)]]
        [[fisics::unit(second)]] = fisics_convert_unit(window_ms, "second");

    struct Wave133StorageCell cell;
    cell.meters = distance_m;
    cell.joules = reserve_j;
    cell.coulombs = charge_c;
    cell.seconds = window_s;
    cell.payload.named.lane = seed + (unsigned)(distance_m * 1024.0);
    cell.payload.named.route = (seed ^ (unsigned)(reserve_j)) & 3u;
    cell.payload.named.tag = rotl32(seed + (unsigned)(charge_c * 16.0), seed + 3u);
    cell.payload.named.mix = seed * 2654435761u + (unsigned)(window_s * 1000000.0);
    cell.payload.named.owner = rotl32(cell.payload.named.lane ^ cell.payload.named.tag ^ cell.payload.named.mix, 11u);
    return cell;
}

static struct Wave133StorageFrame build_frame(unsigned seed) {
    struct Wave133StorageFrame frame;
    unsigned row;
    unsigned col;

    frame.epoch = seed ^ 0x9e3779b9u;
    for (row = 0u; row < 2u; ++row) {
        for (col = 0u; col < 2u; ++col) {
            unsigned cell_seed = seed + row * 37u + col * 19u;
            frame.cells[row][col] =
                make_cell(6.5 + (double)(row * 2u + col) * 0.75,
                          1.125 + (double)(row + col) * 0.3125,
                          160.0 + (double)(cell_seed & 31u) * 3.0,
                          700.0 + (double)(row * 17u + col * 11u),
                          cell_seed);
            frame.epoch ^= frame.cells[row][col].payload.named.owner;
        }
    }
    frame.footer = frame.cells[1][0].payload;
    frame.footer.words[2] ^= frame.cells[0][1].payload.named.mix;
    frame.footer.words[4] += frame.cells[1][1].payload.named.owner;
    return frame;
}

static struct Wave133StorageFrame fold_frame(struct Wave133StorageFrame frame, unsigned step) {
    unsigned src_row = (frame.epoch + step) & 1u;
    unsigned src_col = (frame.footer.named.route + step) & 1u;
    unsigned dst_row = src_row ^ 1u;
    unsigned dst_col = src_col ^ 1u;
    struct Wave133StorageCell carry = frame.cells[src_row][src_col];
    struct Wave133StorageCell next = frame.cells[dst_row][dst_col];

    next.meters += carry.meters * 0.25 + (double)((step & 3u) + 1u) * 0.0625;
    next.joules += carry.joules * 0.125 + next.meters * (double)(step + 1u);
    next.coulombs += carry.coulombs * 0.5 + (double)(src_col + 1u) * 0.25;
    next.seconds += carry.seconds * 0.25 + (double)(step & 7u) * 0.00025;
    next.payload.words[step % 5u] ^= rotl32(carry.payload.named.owner + frame.epoch, step + 5u);
    next.payload.named.owner = rotl32(next.payload.named.owner ^ next.payload.named.mix ^ step, step + 7u);

    frame.cells[dst_row][dst_col] = next;
    frame.footer.words[(step + 2u) % 5u] ^= next.payload.named.owner + carry.payload.named.tag;
    frame.epoch = rotl32(frame.epoch ^ next.payload.named.owner ^ frame.footer.named.mix, step + 3u);
    return frame;
}

static unsigned frame_digest(struct Wave133StorageFrame frame, unsigned salt) {
    unsigned acc = salt ^ frame.epoch ^ frame.footer.named.owner;
    unsigned row;
    unsigned col;

    for (row = 0u; row < 2u; ++row) {
        for (col = 0u; col < 2u; ++col) {
            struct Wave133StorageCell cell = frame.cells[row][col];
            unsigned dm = (unsigned)(cell.meters * 1000.0);
            unsigned rj = (unsigned)(cell.joules);
            unsigned cc = (unsigned)(cell.coulombs * 32.0);
            unsigned ts = (unsigned)(cell.seconds * 1000000.0);
            acc ^= dm + rotl32(rj, row + 5u) + rotl32(cc, col + 9u);
            acc = rotl32(acc + ts + cell.payload.words[(row * 2u + col) % 5u], row * 7u + col + 6u);
        }
    }

    return acc ^ frame.footer.words[0] ^ rotl32(frame.footer.words[3], 17u);
}

int main(void) {
    struct Wave133StorageFrame frames[3];
    unsigned acc = 0x243f6a88u;
    unsigned i;

    frames[0] = build_frame(23u);
    frames[1] = build_frame(59u);
    frames[2] = build_frame(101u);

    for (i = 0u; i < 10u; ++i) {
        unsigned slot = (frames[i % 3u].epoch + i) % 3u;
        frames[slot] = fold_frame(frames[(slot + 1u) % 3u], i + 6u);
        acc ^= frame_digest(frames[slot], rotl32(0x85ebca6bu + i * 131u, i + 4u));
        acc = rotl32(acc + frames[slot].footer.named.owner, 8u);
    }

    printf("%u %u %u\n",
           acc,
           frame_digest(frames[0], 0x9e3779b9u),
           frame_digest(frames[2], 0xc2b2ae35u));
    return 0;
}
