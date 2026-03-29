struct Vec3 {
    int v[3];
};

struct Vec3 abi_make_vec3(int base) {
    struct Vec3 out;
    out.v[0] = base;
    out.v[1] = base + 1;
    out.v[2] = base + 2;
    return out;
}

struct Vec3 abi_rotate_vec3(struct Vec3 in) {
    struct Vec3 out;
    out.v[0] = in.v[1];
    out.v[1] = in.v[2];
    out.v[2] = in.v[0];
    return out;
}

int abi_dot_vec3(struct Vec3 a, struct Vec3 b) {
    return a.v[0] * b.v[0] + a.v[1] * b.v[1] + a.v[2] * b.v[2];
}
