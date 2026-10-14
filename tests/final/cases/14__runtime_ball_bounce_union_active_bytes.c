/* Regression intake: source_lane=ad-hoc-program; source_project=Ball Bounce;
   origin=new_gap; failure_kind=runtime_mismatch; severity=critical;
   owner_lane=14-runtime-surface. Polygon carrier padding overlapped the
   active box double, losing 0.5 during a by-value call. */
#include <stdio.h>

typedef struct Vec2 { double x, y; } Vec2;
typedef union Payload {
    struct { double width, height; } box;
    struct { int count; Vec2 vertices[16]; } polygon;
} Payload;
typedef struct Shape { int kind; Vec2 offset; Payload data; } Shape;

static Shape relay(Shape shape) { return shape; }
static int take(Shape shape, Vec2 position, double mass) {
    return shape.kind == 2 && shape.data.box.width == 0.5 &&
           shape.data.box.height == 0.25 && shape.offset.x == 6 &&
           shape.offset.y == 7 && position.x == 1 && position.y == 2 && mass == 3;
}
static Payload choose(int select, Payload a, Payload b) { return select ? a : b; }

static int first = 17, second = 29;
typedef union PointerPayload {
    struct { char tag; int *first; int *second; } padded;
    unsigned char bytes[24];
} PointerPayload;
static PointerPayload pointers = { .padded = { 'Q', &first, &second } };
static PointerPayload pointer_relay(PointerPayload value) { return value; }
static int pointer_take(PointerPayload value) {
    return value.padded.tag == 'Q' && value.padded.first == &first &&
           value.padded.second == &second && *value.padded.first == 17 &&
           *value.padded.second == 29;
}

int main(void) {
    Shape shape = {0}, copied = {0};
    Vec2 position = {1, 2};
    shape.kind = 2; shape.offset.x = 6; shape.offset.y = 7;
    shape.data.box.width = 0.5; shape.data.box.height = 0.25;
    if (!take(shape, position, 3)) return 1;
    copied = relay(shape);
    if (!take(copied, position, 3)) return 2;
    Shape array[2] = {0}; array[1] = copied;
    if (!take(array[1], position, 3)) return 3;
    Payload other = {0}, selected;
    other.box.width = 0.75; other.box.height = 0.125;
    selected = choose(1, shape.data, other);
    if (selected.box.width != 0.5 || selected.box.height != 0.25) return 4;
    selected = choose(0, shape.data, other);
    if (selected.box.width != 0.75 || selected.box.height != 0.125) return 5;
    shape.data.polygon.count = 2;
    shape.data.polygon.vertices[0].x = 9;
    shape.data.polygon.vertices[1].y = 11;
    copied = relay(shape);
    if (copied.data.polygon.count != 2 || copied.data.polygon.vertices[0].x != 9 ||
        copied.data.polygon.vertices[1].y != 11) return 6;
    if (!pointer_take(pointers)) return 7;
    PointerPayload pointer_copy = pointer_relay(pointers);
    if (!pointer_take(pointer_copy)) return 8;
    pointer_copy = pointers;
    if (!pointer_take(pointer_copy)) return 9;
    puts("union active bytes: PASS");
    return 0;
}
