#include <stdio.h>

typedef struct ProbeColor {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} ProbeColor;

typedef struct ProbeRect {
    int x;
    int y;
    int w;
    int h;
} ProbeRect;

int probe_external_small_struct_bridge(const char *text, ProbeColor color, ProbeRect *rect);

int main(void) {
    ProbeRect rect = {10, 20, 30, 40};
    ProbeColor color = {230, 1, 2, 3};
    int rc = probe_external_small_struct_bridge("hello", color, &rect);
    printf("rc=%d rect.x=%d\n", rc, rect.x);
    return 0;
}
