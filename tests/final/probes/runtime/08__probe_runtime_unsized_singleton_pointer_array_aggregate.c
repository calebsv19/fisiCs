#include <stdio.h>
#include <string.h>

typedef struct Descriptor {
    const char *mode_id;
    const char *field_id;
    unsigned capabilities;
    const char *const *aliases;
    unsigned long alias_count;
} Descriptor;

static const char *const k_fuel_aliases[] = {"fuel"};
static const char *const k_heat_aliases[] = {"temperature", "thermal"};

static const Descriptor k_descriptors[] = {
    {"fire_spread", "fuel_amount", 31u, k_fuel_aliases, 1u},
    {"fire_spread", "heat", 17u, k_heat_aliases, 2u}
};

int main(void) {
    unsigned score;

    if (sizeof(k_fuel_aliases) / sizeof(k_fuel_aliases[0]) != 1u ||
        k_descriptors[0].mode_id == 0 ||
        k_descriptors[0].aliases == 0 ||
        k_descriptors[1].aliases == 0) {
        puts("null");
        return 1;
    }
    score = (unsigned)strlen(k_descriptors[0].mode_id) +
            (unsigned)strlen(k_descriptors[0].aliases[0]) +
            (unsigned)strlen(k_descriptors[1].aliases[1]) +
            k_descriptors[0].capabilities +
            k_descriptors[1].capabilities +
            (unsigned)k_descriptors[0].alias_count +
            (unsigned)k_descriptors[1].alias_count;
    printf("%u %s %s %s\n",
           score,
           k_descriptors[0].mode_id,
           k_descriptors[0].aliases[0],
           k_descriptors[1].aliases[1]);
    return 0;
}
