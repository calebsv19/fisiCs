#include <stdio.h>

typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} ProbeColor;

int probe_sum_color(void *renderer, void *font, const char *text, ProbeColor color, int *out);

int main(void) {
    ProbeColor color = {1, 2, 3, 4};
    int out = -1;
    int rc = probe_sum_color((void *)1, (void *)2, "probe", color, &out);
    printf("rc=%d out=%d\n", rc, out);
    return 0;
}
