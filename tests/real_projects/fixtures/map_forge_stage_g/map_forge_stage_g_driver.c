#include "app/app_pins.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint64_t pin_digest(const MapForgePinsFile *pins) {
    uint64_t hash = UINT64_C(1469598103934665603);
    size_t i = 0;
    const unsigned char *cursor = NULL;
    if (!pins) return 0;
#define HASH_TEXT(value) do { \
    cursor = (const unsigned char *)(value); \
    while (*cursor) { hash ^= (uint64_t)*cursor++; hash *= UINT64_C(1099511628211); } \
    hash ^= UINT64_C(255); hash *= UINT64_C(1099511628211); \
} while (0)
    HASH_TEXT(pins->map_region);
    for (i = 0; i < pins->pin_count; ++i) {
        char numeric[128];
        const MapForgePin *pin = &pins->pins[i];
        HASH_TEXT(pin->id);
        HASH_TEXT(pin->name);
        HASH_TEXT(pin->type);
        HASH_TEXT(pin->color);
        snprintf(numeric, sizeof(numeric), "%.6f,%.6f,%d", pin->lat, pin->lon, pin->private_flag ? 1 : 0);
        HASH_TEXT(numeric);
    }
#undef HASH_TEXT
    return hash;
}

static void checkpoint(const char *name, const MapForgePinsFile *pins, const char *detail) {
    printf("TRACE|1|%s|count=%zu|digest=%016llx|detail=%s\n",
           name,
           pins ? pins->pin_count : 0u,
           (unsigned long long)pin_digest(pins),
           detail ? detail : "ok");
}

static int write_canonical(const char *path, const MapForgePinsFile *pins) {
    FILE *file = fopen(path, "wb");
    size_t i = 0;
    if (!file || !pins) return 0;
    fprintf(file, "version=%u\nregion=%s\ncount=%zu\n", pins->version, pins->map_region, pins->pin_count);
    for (i = 0; i < pins->pin_count; ++i) {
        const MapForgePin *pin = &pins->pins[i];
        fprintf(file,
                "order=%zu|id=%s|name=%s|type=%s|color=%s|lat=%.6f|lon=%.6f|private=%d\n",
                i,
                pin->id,
                pin->name,
                pin->type,
                pin->color,
                pin->lat,
                pin->lon,
                pin->private_flag ? 1 : 0);
    }
    return fclose(file) == 0;
}

static int run_bite_1(void) {
    const char *input = getenv("MAPFORGE_STAGEG_PINS_INPUT");
    const char *malformed = getenv("MAPFORGE_STAGEG_MALFORMED_INPUT");
    MapForgePinsFile pins;
    MapForgePinsFile reloaded;
    MapForgePinsFile fallback;
    MapForgePinsFile invalid;
    MapForgePin added;
    RegionInfo region = {0};
    char error[256];
    char resolved[MAPFORGE_PIN_PATH_CAPACITY];
    bool from_file = false;
    bool from_legacy = false;
    uint64_t saved_digest = 0;
    size_t last = 0;

    map_forge_pins_file_init(&pins);
    map_forge_pins_file_init(&reloaded);
    map_forge_pins_file_init(&fallback);
    map_forge_pins_file_init(&invalid);
    if (!input || !malformed || !map_forge_pins_load(input, &pins, error, sizeof(error))) return 11;
    checkpoint("b1_loaded", &pins, "seattle_example");
    if (!map_forge_pins_find_by_id_const(&pins, "demo_start") ||
        !map_forge_pins_find_by_name_const(&pins, "Demo Goal")) return 12;
    checkpoint("b1_lookup", &pins, "id_and_exact_name");

    memset(&added, 0, sizeof(added));
    snprintf(added.id, sizeof(added.id), "stageg_mid");
    snprintf(added.name, sizeof(added.name), "Stage G Midpoint");
    snprintf(added.type, sizeof(added.type), "waypoint");
    snprintf(added.color, sizeof(added.color), "amber");
    added.lat = 47.615500;
    added.lon = -122.337200;
    added.private_flag = true;
    if (!map_forge_pins_upsert(&pins, &added, error, sizeof(error))) return 13;
    added.lon = -122.337100;
    if (!map_forge_pins_upsert(&pins, &added, error, sizeof(error))) return 14;
    checkpoint("b1_upserted", &pins, "insert_then_update");

    last = pins.pin_count - 1u;
    if (!map_forge_pins_move(&pins, 0u, last) || !map_forge_pins_move(&pins, last, 0u)) return 15;
    checkpoint("b1_reordered", &pins, "forward_then_reverse");
    saved_digest = pin_digest(&pins);
    if (!map_forge_pins_save("pins.saved.json", &pins, error, sizeof(error))) return 16;
    checkpoint("b1_saved", &pins, "raw_json");

    map_forge_pins_file_free(&pins);
    if (!map_forge_pins_load("pins.saved.json", &reloaded, error, sizeof(error)) ||
        pin_digest(&reloaded) != saved_digest) return 17;
    checkpoint("b1_reloaded", &reloaded, "digest_match");
    if (!map_forge_pins_remove_by_id(&reloaded, "stageg_mid") ||
        !map_forge_pins_save("pins.removed.json", &reloaded, error, sizeof(error))) return 18;
    map_forge_pins_file_free(&pins);
    if (!map_forge_pins_load("pins.removed.json", &pins, error, sizeof(error)) ||
        map_forge_pins_find_by_id_const(&pins, "stageg_mid")) return 19;
    checkpoint("b1_removed", &pins, "remove_save_reload");

    region.name = "seattle";
    if (!map_forge_pins_load_preferred_region_file(&region,
                                                    &fallback,
                                                    resolved,
                                                    sizeof(resolved),
                                                    &from_file,
                                                    &from_legacy,
                                                    error,
                                                    sizeof(error)) ||
        !from_file || !from_legacy || fallback.pin_count == 0u) return 20;
    checkpoint("b1_fallback", &fallback, "legacy_selected");
    if (map_forge_pins_load(malformed, &invalid, error, sizeof(error)) ||
        map_forge_pins_move(&pins, pins.pin_count, 0u)) return 21;
    checkpoint("b1_invalid", &pins, "malformed_and_bounds_rejected");
    if (!write_canonical("pins.canonical", &pins)) return 22;

    map_forge_pins_file_free(&pins);
    map_forge_pins_file_free(&reloaded);
    map_forge_pins_file_free(&fallback);
    map_forge_pins_file_free(&invalid);
    checkpoint("b1_shutdown", NULL, "all_state_freed");
    return 0;
}

int main(int argc, char **argv) {
    if (argc == 3 && strcmp(argv[1], "--bite") == 0 && strcmp(argv[2], "1") == 0) {
        return run_bite_1();
    }
    return 64;
}
