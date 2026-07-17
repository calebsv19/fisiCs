#include <stdbool.h>
#include <stdio.h>

typedef struct Wave56Vec3 {
    double x;
    double y;
    double z;
} Wave56Vec3;

typedef struct Wave56Shape {
    int kind;
    Wave56Vec3 offset;
    union {
        struct { double radius; } sphere;
        struct { Wave56Vec3 half_extents; } box;
        struct { Wave56Vec3 normal; double offset; } plane;
    } data;
} Wave56Shape;

typedef struct Wave56Body {
    int id;
    Wave56Shape shape;
    Wave56Vec3 position;
    double orientation[4];
    Wave56Vec3 velocity;
    Wave56Vec3 angular_velocity;
    double mass;
    double inverse_mass;
    Wave56Vec3 inertia;
    Wave56Vec3 inverse_inertia;
    double material[2];
    bool is_static;
} Wave56Body;

int main(void) {
    Wave56Body source = {
        .id = 10403,
        .shape = {
            .kind = 2,
            .offset = {0.25, -0.50, 0.75},
            .data.box.half_extents = {0.40, 0.30, 0.20},
        },
        .position = {1.25, 2.50, -3.75},
        .orientation = {1.0, 0.1, 0.2, 0.3},
        .velocity = {4.0, -5.0, 6.0},
        .angular_velocity = {-0.7, 0.8, -0.9},
        .mass = 1.9,
        .inverse_mass = 1.0 / 1.9,
        .inertia = {0.11, 0.22, 0.33},
        .inverse_inertia = {9.0, 4.5, 3.0},
        .material = {0.04, 0.72},
        .is_static = false,
    };
    Wave56Body scratch[3] = {0};
    Wave56Body* source_ptr = &source;
    scratch[1] = *source_ptr;

    printf("%zu %d %d %.2f %.2f %.2f %.2f %.2f %.2f %d\n",
        sizeof(Wave56Body), scratch[1].id, scratch[1].shape.kind,
        scratch[1].shape.data.box.half_extents.z,
        scratch[1].position.y, scratch[1].orientation[3],
        scratch[1].mass, scratch[1].inverse_inertia.y,
        scratch[1].material[1], scratch[1].is_static ? 1 : 0);
    return 0;
}
