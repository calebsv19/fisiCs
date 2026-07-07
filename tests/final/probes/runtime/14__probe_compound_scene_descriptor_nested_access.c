#include <stdio.h>

enum {
    MAX_VERTICES = 4,
    MAX_PARTS = 2,
    MAX_BODIES = 4,
    MAX_COMPOUNDS = 2
};

typedef enum ShapeKind {
    SHAPE_INVALID = 0,
    SHAPE_BOX = 2,
    SHAPE_POLYGON = 3
} ShapeKind;

typedef struct Vec2 {
    double x;
    double y;
} Vec2;

typedef struct Shape {
    ShapeKind kind;
    Vec2 offset;
    union {
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

typedef struct CompoundPart {
    Shape shape;
} CompoundPart;

typedef struct Compound {
    int part_count;
    CompoundPart parts[MAX_PARTS];
} Compound;

typedef struct Descriptor {
    int body_id;
    const char *shape_family;
    Shape exact_convex_shape;
    int exact_compound_index;
    int compound_part_count;
} Descriptor;

typedef struct Scene {
    int body_count;
    int compound_body_count;
    Descriptor bodies[MAX_BODIES];
    Compound compounds[MAX_COMPOUNDS];
} Scene;

typedef struct Body {
    int id;
    Shape shape;
} Body;

static Vec2 vec2(double x, double y) {
    Vec2 out;
    out.x = x;
    out.y = y;
    return out;
}

static Shape polygon_shape(double bias) {
    Shape shape = {0};
    shape.kind = SHAPE_POLYGON;
    shape.offset = vec2(bias, bias * 0.5);
    shape.data.polygon.vertex_count = 4;
    shape.data.polygon.vertices[0] = vec2(-1.0 + bias, -0.5);
    shape.data.polygon.vertices[1] = vec2(1.0 + bias, -0.5);
    shape.data.polygon.vertices[2] = vec2(1.0 + bias, 0.5);
    shape.data.polygon.vertices[3] = vec2(-1.0 + bias, 0.5);
    return shape;
}

static Shape box_shape(double half_width, double half_height) {
    Shape shape = {0};
    shape.kind = SHAPE_BOX;
    shape.offset = vec2(0.0, 0.0);
    shape.data.box.half_width = half_width;
    shape.data.box.half_height = half_height;
    return shape;
}

static int shape_valid(const Shape *shape) {
    if (!shape) {
        return 0;
    }
    if (shape->kind == SHAPE_BOX) {
        return shape->data.box.half_width > 0.0 &&
               shape->data.box.half_height > 0.0;
    }
    if (shape->kind == SHAPE_POLYGON) {
        return shape->data.polygon.vertex_count == 4 &&
               shape->data.polygon.vertices[0].x < shape->data.polygon.vertices[1].x &&
               shape->data.polygon.vertices[2].y > shape->data.polygon.vertices[1].y;
    }
    return 0;
}

static int compound_valid(const Compound *compound) {
    if (!compound || compound->part_count <= 0 || compound->part_count > MAX_PARTS) {
        return 0;
    }
    for (int i = 0; i < compound->part_count; ++i) {
        if (!shape_valid(&compound->parts[i].shape)) {
            return 0;
        }
    }
    return 1;
}

static const Compound *compound_for_body(const Scene *scene, int body_index) {
    if (!scene || body_index < 0 || body_index >= scene->body_count) {
        return 0;
    }

    const Descriptor *descriptor = &scene->bodies[body_index];
    if (descriptor->exact_compound_index < 0 ||
        descriptor->exact_compound_index >= scene->compound_body_count ||
        descriptor->compound_part_count <= 0) {
        return 0;
    }

    const Compound *compound = &scene->compounds[descriptor->exact_compound_index];
    return compound_valid(compound) ? compound : 0;
}

static int descriptor_has_compound_source(const Scene *scene, int body_index) {
    if (!scene || body_index < 0 || body_index >= scene->body_count) {
        return 0;
    }

    const Descriptor *descriptor = &scene->bodies[body_index];
    return descriptor->exact_compound_index >= 0 &&
           descriptor->exact_compound_index < scene->compound_body_count &&
           descriptor->compound_part_count > 0 &&
           compound_valid(&scene->compounds[descriptor->exact_compound_index]);
}

static int body_with_shape(const Body *source, const Shape *shape, Body *out_body) {
    if (!source || !shape || !out_body || !shape_valid(shape)) {
        return 0;
    }
    *out_body = *source;
    out_body->shape = *shape;
    return shape_valid(&out_body->shape);
}

static int collect_compound_pair(
    const Body *body_a,
    const Body *body_b,
    const Compound *compound_a,
    const Compound *compound_b,
    int *part_tests,
    int *part_contacts) {
    if (!body_a || !body_b || !part_tests || !part_contacts ||
        (!compound_a && !compound_b)) {
        return 0;
    }

    int tests = 0;
    int contacts = 0;
    int count_a = compound_a ? compound_a->part_count : 1;
    int count_b = compound_b ? compound_b->part_count : 1;
    for (int part_a = 0; part_a < count_a; ++part_a) {
        const Shape *shape_a = compound_a ? &compound_a->parts[part_a].shape : &body_a->shape;
        Body part_body_a;
        if (!body_with_shape(body_a, shape_a, &part_body_a)) {
            return 0;
        }

        for (int part_b = 0; part_b < count_b; ++part_b) {
            const Shape *shape_b = compound_b ? &compound_b->parts[part_b].shape : &body_b->shape;
            Body part_body_b;
            if (!body_with_shape(body_b, shape_b, &part_body_b)) {
                return 0;
            }

            ++tests;
            if (part_body_a.shape.kind == SHAPE_POLYGON &&
                part_body_b.shape.kind == SHAPE_POLYGON &&
                part_body_a.shape.data.polygon.vertex_count == 4 &&
                part_body_b.shape.data.polygon.vertex_count == 4 &&
                part_body_a.shape.data.polygon.vertices[2].x >
                    part_body_b.shape.data.polygon.vertices[0].x) {
                ++contacts;
            }
        }
    }

    *part_tests = tests;
    *part_contacts = contacts;
    return 1;
}

static Scene make_scene(void) {
    Scene scene = {0};
    scene.body_count = 4;
    scene.compound_body_count = 2;

    scene.compounds[0].part_count = 2;
    scene.compounds[0].parts[0].shape = polygon_shape(0.25);
    scene.compounds[0].parts[1].shape = polygon_shape(0.50);

    scene.compounds[1].part_count = 2;
    scene.compounds[1].parts[0].shape = polygon_shape(-0.10);
    scene.compounds[1].parts[1].shape = polygon_shape(0.10);

    scene.bodies[0].body_id = 2401;
    scene.bodies[0].shape_family = "box";
    scene.bodies[0].exact_convex_shape = box_shape(1.0, 1.0);
    scene.bodies[0].exact_compound_index = -1;
    scene.bodies[0].compound_part_count = 0;

    scene.bodies[1].body_id = 2403;
    scene.bodies[1].shape_family = "generated_compound";
    scene.bodies[1].exact_convex_shape = box_shape(1.5, 1.5);
    scene.bodies[1].exact_compound_index = 0;
    scene.bodies[1].compound_part_count = scene.compounds[0].part_count;

    scene.bodies[2].body_id = 2406;
    scene.bodies[2].shape_family = "generated_compound";
    scene.bodies[2].exact_convex_shape = box_shape(1.5, 1.5);
    scene.bodies[2].exact_compound_index = 1;
    scene.bodies[2].compound_part_count = scene.compounds[1].part_count;

    scene.bodies[3].body_id = 2404;
    scene.bodies[3].shape_family = "box";
    scene.bodies[3].exact_convex_shape = box_shape(0.5, 0.5);
    scene.bodies[3].exact_compound_index = -1;
    scene.bodies[3].compound_part_count = 0;
    return scene;
}

int main(void) {
    Scene scene = make_scene();
    Body bodies[MAX_BODIES];
    for (int i = 0; i < scene.body_count; ++i) {
        bodies[i].id = scene.bodies[i].body_id;
        bodies[i].shape = scene.bodies[i].exact_convex_shape;
    }

    int has_a = descriptor_has_compound_source(&scene, 1);
    int has_b = descriptor_has_compound_source(&scene, 2);
    const Compound *compound_a = compound_for_body(&scene, 1);
    const Compound *compound_b = compound_for_body(&scene, 2);
    int part_tests = 0;
    int part_contacts = 0;
    int ok = collect_compound_pair(
        &bodies[1],
        &bodies[2],
        compound_a,
        compound_b,
        &part_tests,
        &part_contacts);

    int checksum =
        has_a * 10000 +
        has_b * 1000 +
        ok * 100 +
        part_tests * 10 +
        part_contacts;

    printf("%d %d %d %d %d %d %.2f %.2f\n",
           has_a,
           has_b,
           ok,
           part_tests,
           part_contacts,
           checksum,
           scene.compounds[1].parts[1].shape.data.polygon.vertices[2].x,
           bodies[2].shape.data.box.half_width);
    return 0;
}
