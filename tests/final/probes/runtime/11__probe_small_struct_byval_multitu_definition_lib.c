typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} ProbeColor;

int probe_sum_color(void *renderer, void *font, const char *text, ProbeColor color, int *out) {
    (void)renderer;
    (void)font;
    (void)text;
    if (!out) {
        return 0;
    }
    *out = (int)color.r + (int)color.g + (int)color.b + (int)color.a;
    return *out;
}
