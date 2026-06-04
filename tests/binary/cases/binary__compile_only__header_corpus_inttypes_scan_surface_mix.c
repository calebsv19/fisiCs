#include <inttypes.h>
#include <stdio.h>

int main(void) {
    intmax_t signed_value = 0;
    uintmax_t unsigned_value = 0;
    int matched = sscanf("-77 ff", "%" SCNdMAX " %" SCNxMAX, &signed_value, &unsigned_value);
    return matched == 2 && signed_value < 0 && unsigned_value == 255u ? 0 : 1;
}
