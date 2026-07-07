#include <math.h>
#include <stdio.h>

enum {
    MAX_VERTICES = 16,
    SHAPE_INVALID = 0,
    SHAPE_CIRCLE = 1,
    SHAPE_BOX = 2,
    SHAPE_POLYGON = 3
};

typedef struct Vec2 {
    double x;
    double y;
} Vec2;

typedef struct Shape {
    int kind;
    Vec2 offset;
    union {
        struct {
            double radius;
        } circle;
        struct {
            double half_width;
            double half_height;
        } box;
        struct {
            int vertex_count;
            Vec2 vertices[MAX_VERTICES];
        } polygon;
    } data;
} Shape;

typedef struct Body {
    int id;
    Shape shape;
    Vec2 position;
} Body;

static Vec2 make_vec2(double x, double y) {
    Vec2 value = {x, y};
    return value;
}

static Shape make_box_offset(double half_width, double half_height, Vec2 offset) {
    Shape shape = {0};
    shape.kind = SHAPE_BOX;
    shape.offset = offset;
    shape.data.box.half_width = half_width;
    shape.data.box.half_height = half_height;
    return shape;
}

static Shape make_box(double half_width, double half_height) {
    return make_box_offset(half_width, half_height, make_vec2(0.0, 0.0));
}

static int shape_valid(const Shape* shape) {
    if (shape == 0 || shape->kind != SHAPE_BOX) {
        return 0;
    }
    return isfinite(shape->offset.x) &&
           isfinite(shape->offset.y) &&
           isfinite(shape->data.box.half_width) &&
           shape->data.box.half_width > 0.0 &&
           isfinite(shape->data.box.half_height) &&
           shape->data.box.half_height > 0.0;
}

int main(void) {
    const double min_x = -0.46115887271855299;
    const double max_x = 0.42319702365921336;
    const double min_y = -0.21607987828244779;
    const double max_y = 0.20693475222791907;
    const double half_width = (max_x - min_x) * 0.5;
    const double half_height = (max_y - min_y) * 0.5;

    Shape direct = make_box_offset(half_width, half_height, make_vec2(0.0, 0.0));
    Shape nested = make_box(half_width, half_height);
    Body body = {0};
    body.id = 2406;
    body.shape = make_box(half_width, half_height);
    body.position = make_vec2(4.867751, 4.436642);

    printf("%.17g %.17g %.17g %.17g %d %d %d\n",
           half_width,
           half_height,
           direct.data.box.half_width,
           nested.data.box.half_width,
           shape_valid(&direct),
           shape_valid(&nested),
           shape_valid(&body.shape));
    printf("%.17g %.17g %.17g %.17g %.17g %.17g\n",
           direct.data.box.half_width,
           direct.data.box.half_height,
           nested.data.box.half_width,
           nested.data.box.half_height,
           body.shape.data.box.half_width,
           body.shape.data.box.half_height);

    if (direct.data.box.half_width != half_width ||
        direct.data.box.half_height != half_height ||
        nested.data.box.half_width != half_width ||
        nested.data.box.half_height != half_height ||
        body.shape.data.box.half_width != half_width ||
        body.shape.data.box.half_height != half_height ||
        !shape_valid(&body.shape)) {
        return 1;
    }
    return 0;
}
