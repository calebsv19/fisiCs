#include "config/config_file_io.h"

#include <json-c/json.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void) {
    int ok = 1;
    char root[256];
    char nested_dir[512];
    char json_path[512];
    char fallback_path[512];
    char legacy_path[512];
    const char *selected = NULL;
    FILE *fp = NULL;
    struct json_object *parsed = NULL;
    struct json_object *value = NULL;

    (void)snprintf(root, sizeof(root), "/tmp/raycfg-stageb-%ld", (long)getpid());

    (void)snprintf(nested_dir, sizeof(nested_dir), "%s/config/runtime", root);
    if (!config_io_ensure_directory_exists(nested_dir)) ok = 0;
    if (!config_io_directory_exists(nested_dir)) ok = 0;

    (void)snprintf(json_path, sizeof(json_path), "%s/config/runtime/settings.json", root);
    if (!config_io_ensure_parent_directory_for_file(json_path)) ok = 0;
    if (!config_io_directory_exists(nested_dir)) ok = 0;

    fp = fopen(json_path, "w");
    if (!fp) return 1;
    fputs("{\"version\":1,\"name\":\"ray\"}\n", fp);
    fclose(fp);

    (void)snprintf(fallback_path, sizeof(fallback_path), "%s/missing/fallback.json", root);
    (void)snprintf(legacy_path, sizeof(legacy_path), "%s/missing/legacy.json", root);
    fp = config_io_open_read_with_fallback(fallback_path, json_path, legacy_path, &selected);
    if (!fp) {
        ok = 0;
    } else {
        parsed = config_io_parse_json_file(fp, "settings", false);
    }

    if (!parsed) {
        ok = 0;
    } else if (!json_object_object_get_ex(parsed, "version", &value) ||
               json_object_get_int(value) != 1) {
        ok = 0;
    }
    if (!selected || strcmp(selected, json_path) != 0) ok = 0;
    if (parsed) json_object_put(parsed);

    printf("selected=%s ok=%d\n", selected ? selected : "(null)", ok);
    return ok ? 0 : 1;
}
