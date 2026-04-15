#include <stdio.h>

#include "drawing_program/drawing_program_color_model.h"
#include "drawing_program/drawing_program_document.h"
#include "drawing_program/drawing_program_editor_state.h"
#include "drawing_program/drawing_program_viewport.h"

static int expect_true(int condition, const char *label) {
    if (condition) {
        return 1;
    }
    fprintf(stderr, "drawing_program_stageb_color_viewport_subset: assertion failed (%s)\n", label);
    return 0;
}

int main(void) {
    static DrawingProgramDocument doc;
    DrawingProgramEditorState editor;
    DrawingProgramViewportTransform transform;
    DrawingProgramScreenPoint screen;
    DrawingProgramSamplePoint sample;
    uint8_t default_index;
    uint8_t default_value;
    uint8_t r = 0u;
    uint8_t g = 0u;
    uint8_t b = 0u;

    doc.logical_width = 64u;
    doc.logical_height = 32u;
    doc.sample_density = 2u;
    doc.layer_count = 1u;
    doc.layers[0].layer_id = 1u;

    default_index = drawing_program_color_default_index();
    default_value = drawing_program_color_value_from_index(default_index);
    drawing_program_color_rgb_from_sample(default_value, &r, &g, &b);
    if (!expect_true(r == default_value && g == default_value && b == default_value, "color_rgb_round_trip")) {
        return 1;
    }

    drawing_program_editor_state_init(&editor, &doc);
    editor.viewport.pan_x = 10.0f;
    editor.viewport.pan_y = 20.0f;
    editor.viewport.zoom = 2.0f;

    drawing_program_viewport_transform_from_state(&editor, &doc, &transform);
    screen.x = 42.0f;
    screen.y = 38.0f;
    if (!drawing_program_screen_to_sample(transform, screen, &sample)) {
        fprintf(stderr, "drawing_program_stageb_color_viewport_subset: screen_to_sample failed\n");
        return 1;
    }
    if (!expect_true(sample.x == 32u && sample.y == 18u, "viewport_sample_mapping")) {
        return 1;
    }

    puts("drawing_program_stageb_color_viewport_subset: success");
    return 0;
}
