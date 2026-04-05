#include "UI/shared_theme_font_adapter.h"

#include "core_theme.h"

#include <stdio.h>
#include <stdlib.h>

static void dump_color(const char* name, SDL_Color c) {
    printf("%s=%u,%u,%u,%u\n", name, (unsigned)c.r, (unsigned)c.g, (unsigned)c.b, (unsigned)c.a);
}

int main(void) {
    LineDrawing3dThemePalette palette = {0};
    CoreThemePreset preset = {0};
    CoreThemeColor raw = {0};

    setenv("LINE_DRAWING3D_USE_SHARED_THEME", "1", 1);
    setenv("LINE_DRAWING3D_THEME_PRESET", "standard_grey", 1);

    if (!line_drawing3d_shared_theme_resolve_palette(&palette)) {
        puts("resolve_palette_failed");
        return 2;
    }

    if (core_theme_get_preset(CORE_THEME_PRESET_IDE_GRAY, &preset).code != CORE_OK) {
        puts("core_theme_get_preset_failed");
        return 3;
    }
    if (core_theme_get_color(&preset, CORE_THEME_COLOR_TEXT_PRIMARY, &raw).code != CORE_OK) {
        puts("core_theme_get_color_failed");
        return 4;
    }

    dump_color("palette.button_fill", palette.button_fill);
    dump_color("palette.button_border", palette.button_border);
    dump_color("palette.button_text", palette.button_text);
    dump_color("palette.text_primary", palette.text_primary);
    dump_color("palette.text_muted", palette.text_muted);

    printf("core.text_primary=%u,%u,%u,%u\n",
           (unsigned)raw.r, (unsigned)raw.g, (unsigned)raw.b, (unsigned)raw.a);

    return 0;
}
