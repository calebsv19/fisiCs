typedef struct wave49_window wave49_window;
typedef struct wave49_font_impl wave49_font;

int wave49_window_ready(wave49_window *window);
int wave49_font_ready(wave49_font *font, const char *path, int size);

int wave49_window_ready(wave49_window *window) {
    return window != 0;
}

int wave49_font_ready(wave49_font *font, const char *path, int size) {
    return font != 0 && path != 0 && size > 0;
}

int main(void) {
    return 0;
}
