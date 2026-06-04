#include <stdio.h>

int main(void) {
    char buf[32];
    char prefix[4];
    int value = -1;
    int n = snprintf(buf, sizeof(buf), "%s-%03d", "id", 7);
    int matched = sscanf(buf, "%2[a-z]-%d", prefix, &value);

    printf("stdio-format n=%d matched=%d prefix=%s value=%d text=%s\n",
           n,
           matched,
           prefix,
           value,
           buf);
    return n == 6 && matched == 2 && prefix[0] == 'i' && prefix[1] == 'd' &&
                   prefix[2] == '\0' && value == 7
               ? 0
               : 1;
}
