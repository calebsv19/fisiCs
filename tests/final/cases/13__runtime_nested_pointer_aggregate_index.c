#include <stdio.h>

typedef unsigned long size_t;

typedef struct NestedIndexVec3 {
    double x;
    double y;
    double z;
} NestedIndexVec3;

typedef struct NestedIndexFace {
    size_t vertex_indices[3];
    NestedIndexVec3 outward_normal;
} NestedIndexFace;

typedef struct NestedIndexAsset {
    NestedIndexVec3* local_vertices;
    NestedIndexFace* faces;
} NestedIndexAsset;

static NestedIndexVec3 passthrough(NestedIndexVec3 value) {
    return value;
}

const char* nested_index_label(int value);

const char* nested_index_label(int value) {
    return value ? "nested-index" : "unexpected";
}

static NestedIndexVec3 select_vertex(const NestedIndexAsset* asset,
                                     size_t face_index,
                                     int vertex_index) {
    return passthrough(asset->local_vertices[
        asset->faces[face_index].vertex_indices[vertex_index]]);
}

int main(void) {
    NestedIndexVec3 vertices[3] = {
        {1.0, 2.0, 3.0},
        {4.0, 5.0, 6.0},
        {7.0, 8.0, 9.0},
    };
    NestedIndexFace faces[1] = {{{2, 0, 1}, {0.0, 0.0, 1.0}}};
    NestedIndexAsset asset = {vertices, faces};
    NestedIndexVec3 selected = select_vertex(&asset, 0, 0);

    printf("%s %.1f %.1f %.1f\n",
           nested_index_label(1), selected.x, selected.y, selected.z);
    return 0;
}
