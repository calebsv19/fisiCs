// SPDX-License-Identifier: Apache-2.0

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int parse_named_int(const char* text, const char* name, int* out_value) {
    const char* start = strstr(text, name);
    char* end = NULL;
    long value = 0;

    if (!start) {
        return 0;
    }
    start += strlen(name);
    if (*start != '=') {
        return 0;
    }

    value = strtol(start + 1, &end, 10);
    if (end == start + 1 || (*end != ';' && *end != '\0')) {
        return 0;
    }

    *out_value = (int)value;
    return 1;
}

static int parse_tag(const char* text, char* out_tag, size_t out_size) {
    const char* start = strstr(text, "tag=");
    size_t used = 0;

    if (!start || out_size == 0) {
        return 0;
    }
    start += 4;

    while (start[used] != '\0' && start[used] != ';' && used + 1 < out_size) {
        if (!isalnum((unsigned char)start[used])) {
            return 0;
        }
        out_tag[used] = start[used];
        used++;
    }
    out_tag[used] = '\0';
    return used > 0;
}

static int tag_checksum(const char* tag) {
    int sum = 0;
    for (size_t i = 0; tag[i] != '\0'; ++i) {
        sum += (unsigned char)tag[i];
    }
    return sum;
}

int main(void) {
    const char* packet = "mass=12;velocity=34;tag=OK42";
    int mass = 0;
    int velocity = 0;
    char tag[8];
    int fields = 0;
    int checksum = 0;

    fields += parse_named_int(packet, "mass", &mass);
    fields += parse_named_int(packet, "velocity", &velocity);
    fields += parse_tag(packet, tag, sizeof(tag));
    checksum = tag_checksum(tag);

    printf("canary libc-string: fields=%d mass=%d velocity=%d tag=%s checksum=%d\n",
           fields,
           mass,
           velocity,
           tag,
           checksum);

    return (fields == 3 &&
            mass == 12 &&
            velocity == 34 &&
            strcmp(tag, "OK42") == 0 &&
            checksum == 256) ? 0 : 1;
}
