#include "app/datalab_runtime_prefs.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void checkpoint(const char *name, const char *detail, unsigned long long value) {
    printf("TRACE|1|%s|detail=%s|value=%llu|result=1\n",
           name,
           detail ? detail : "ok",
           value);
}

static DatalabWorkspaceCustomTheme make_theme(unsigned int base) {
    DatalabWorkspaceCustomTheme theme;
    unsigned char *bytes = (unsigned char *)&theme;
    size_t i = 0u;
    for (i = 0u; i < sizeof(theme); ++i) {
        bytes[i] = (unsigned char)((base + (unsigned int)i * 7u) & 255u);
    }
    return theme;
}

int main(void) {
    char roots[DATALAB_RECENT_INPUT_ROOT_LIMIT][DATALAB_APP_PATH_CAP] = {{0}};
    char files[DATALAB_RECENT_INPUT_FILE_LIMIT][DATALAB_APP_PATH_CAP] = {{0}};
    char pins[DATALAB_RECENT_INPUT_FILE_LIMIT][DATALAB_APP_PATH_CAP] = {{0}};
    char loaded_roots[DATALAB_RECENT_INPUT_ROOT_LIMIT][DATALAB_APP_PATH_CAP] = {{0}};
    char loaded_files[DATALAB_RECENT_INPUT_FILE_LIMIT][DATALAB_APP_PATH_CAP] = {{0}};
    char loaded_pins[DATALAB_RECENT_INPUT_FILE_LIMIT][DATALAB_APP_PATH_CAP] = {{0}};
    char names[DATALAB_CUSTOM_THEME_SLOT_COUNT][DATALAB_CUSTOM_THEME_NAME_CAP] = {{0}};
    char loaded_names[DATALAB_CUSTOM_THEME_SLOT_COUNT][DATALAB_CUSTOM_THEME_NAME_CAP] = {{0}};
    DatalabWorkspaceCustomTheme slots[DATALAB_CUSTOM_THEME_SLOT_COUNT];
    DatalabWorkspaceCustomTheme loaded_slots[DATALAB_CUSTOM_THEME_SLOT_COUNT];
    DatalabWorkspaceCustomTheme active;
    size_t root_count = 0u, file_count = 0u, pin_count = 0u;
    size_t loaded_root_count = 0u, loaded_file_count = 0u, loaded_pin_count = 0u;
    int zoom = 0;
    uint8_t preset = 0u, active_slot = 0u;
    char input_root[DATALAB_APP_PATH_CAP] = {0};
    FILE *canonical = NULL;
    size_t i = 0u;

    datalab_runtime_prefs_clear_diagnostic();
    datalab_recent_input_roots_add(roots, &root_count, DATALAB_RECENT_INPUT_ROOT_LIMIT, "library/root_a/");
    datalab_recent_input_roots_add(roots, &root_count, DATALAB_RECENT_INPUT_ROOT_LIMIT, "library/root_b");
    datalab_recent_input_files_add(files, &file_count, DATALAB_RECENT_INPUT_FILE_LIMIT, "library/root_b/frame_02.png");
    datalab_recent_input_files_add(files, &file_count, DATALAB_RECENT_INPUT_FILE_LIMIT, "library/root_a/frame_01.pack");
    datalab_recent_input_files_add(pins, &pin_count, DATALAB_RECENT_INPUT_FILE_LIMIT, "library/root_b/frame_02.png");
    datalab_recent_input_files_add(pins, &pin_count, DATALAB_RECENT_INPUT_FILE_LIMIT, "library/root_a/frame_01.pack");
    if (root_count != 2u || file_count != 2u || pin_count != 2u) return 11;
    checkpoint("b2_prepared", "unique_mru", (unsigned long long)(root_count + file_count + pin_count));

    for (i = 0u; i < DATALAB_CUSTOM_THEME_SLOT_COUNT; ++i) {
        slots[i] = make_theme(20u + (unsigned int)i * 40u);
    }
    snprintf(names[0], sizeof(names[0]), "Ocean Blue");
    snprintf(names[1], sizeof(names[1]), "Warm/Unsafe");
    snprintf(names[2], sizeof(names[2]), "Graphite");

    if (!datalab_runtime_prefs_save_text_zoom_step(4) ||
        !datalab_runtime_prefs_save_input_root("library/root_a/") ||
        !datalab_runtime_prefs_save_recent_input_roots((const char (*)[DATALAB_APP_PATH_CAP])roots, root_count) ||
        !datalab_runtime_prefs_save_recent_input_files((const char (*)[DATALAB_APP_PATH_CAP])files, file_count) ||
        !datalab_runtime_prefs_save_pinned_input_files((const char (*)[DATALAB_APP_PATH_CAP])pins, pin_count) ||
        !datalab_runtime_prefs_save_theme_preset_id((uint8_t)DATALAB_WORKSPACE_AUTHORING_THEME_CUSTOM) ||
        !datalab_runtime_prefs_save_custom_theme(&slots[2]) ||
        !datalab_runtime_prefs_save_custom_theme_slots(slots, DATALAB_CUSTOM_THEME_SLOT_COUNT) ||
        !datalab_runtime_prefs_save_custom_theme_slot_names((const char (*)[DATALAB_CUSTOM_THEME_NAME_CAP])names,
                                                             DATALAB_CUSTOM_THEME_SLOT_COUNT) ||
        !datalab_runtime_prefs_save_custom_theme_active_slot(2u)) return 12;
    checkpoint("b2_saved", "all_preferences", 10u);

    memset(roots, 0, sizeof(roots));
    memset(files, 0, sizeof(files));
    memset(pins, 0, sizeof(pins));
    memset(names, 0, sizeof(names));
    memset(loaded_slots, 0, sizeof(loaded_slots));
    memset(&active, 0, sizeof(active));
    checkpoint("b2_destroyed", "memory_cleared", 0u);

    if (!datalab_runtime_prefs_load_text_zoom_step(&zoom) || zoom != 4 ||
        !datalab_runtime_prefs_load_input_root(input_root, sizeof(input_root)) ||
        strcmp(input_root, "library/root_a") != 0 ||
        !datalab_runtime_prefs_load_recent_input_roots(loaded_roots, DATALAB_RECENT_INPUT_ROOT_LIMIT, &loaded_root_count) ||
        !datalab_runtime_prefs_load_recent_input_files(loaded_files, DATALAB_RECENT_INPUT_FILE_LIMIT, &loaded_file_count) ||
        !datalab_runtime_prefs_load_pinned_input_files(loaded_pins, DATALAB_RECENT_INPUT_FILE_LIMIT, &loaded_pin_count) ||
        !datalab_runtime_prefs_load_theme_preset_id(&preset) ||
        !datalab_runtime_prefs_load_custom_theme(&active) ||
        !datalab_runtime_prefs_load_custom_theme_slots(loaded_slots, DATALAB_CUSTOM_THEME_SLOT_COUNT) ||
        !datalab_runtime_prefs_load_custom_theme_slot_names(loaded_names, DATALAB_CUSTOM_THEME_SLOT_COUNT) ||
        !datalab_runtime_prefs_load_custom_theme_active_slot(&active_slot)) return 13;
    checkpoint("b2_reloaded", "all_preferences", (unsigned long long)(loaded_root_count + loaded_file_count + loaded_pin_count));

    if (loaded_root_count != 2u || strcmp(loaded_roots[0], "library/root_a") != 0 ||
        strcmp(loaded_roots[1], "library/root_b") != 0 ||
        loaded_file_count != 2u || strcmp(loaded_files[0], "library/root_b/frame_02.png") != 0 ||
        loaded_pin_count != 2u || preset != DATALAB_WORKSPACE_AUTHORING_THEME_CUSTOM || active_slot != 2u ||
        memcmp(&active, &slots[2], sizeof(active)) != 0 ||
        memcmp(loaded_slots, slots, sizeof(slots)) != 0 ||
        strcmp(loaded_names[0], "Ocean Blue") != 0 ||
        strcmp(loaded_names[1], "Warm_Unsafe") != 0 ||
        strcmp(loaded_names[2], "Graphite") != 0 ||
        datalab_runtime_prefs_last_diagnostic()[0] != '\0') return 14;
    checkpoint("b2_compared", "roundtrip_equal", (unsigned long long)active_slot);

    canonical = fopen("prefs.canonical", "wb");
    if (!canonical) return 15;
    fprintf(canonical,
            "zoom=%d\nroot=%s\nrecent_roots=%zu:%s,%s\nrecent_files=%zu:%s,%s\n"
            "pinned=%zu:%s,%s\npreset=%u\nactive_slot=%u\nnames=%s,%s,%s\n"
            "active_clear=%u,%u,%u\n",
            zoom, input_root,
            loaded_root_count, loaded_roots[0], loaded_roots[1],
            loaded_file_count, loaded_files[0], loaded_files[1],
            loaded_pin_count, loaded_pins[0], loaded_pins[1],
            (unsigned int)preset, (unsigned int)active_slot,
            loaded_names[0], loaded_names[1], loaded_names[2],
            (unsigned int)active.clear_r, (unsigned int)active.clear_g, (unsigned int)active.clear_b);
    if (fclose(canonical) != 0) return 16;
    checkpoint("b2_canonical", "artifact_written", 1u);
    checkpoint("b2_shutdown", "state_released", 0u);
    return 0;
}
