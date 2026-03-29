#include <stdio.h>

struct Vec3 {
    int v[3];
};

struct Vec3 abi_make_vec3(int base);
struct Vec3 abi_rotate_vec3(struct Vec3 in);
int abi_dot_vec3(struct Vec3 a, struct Vec3 b);

int main(void) {
    struct Vec3 a = abi_make_vec3(2);
    struct Vec3 b = abi_rotate_vec3(a);
    int dot = abi_dot_vec3(a, b);
    printf("%d %d %d %d\n", b.v[0], b.v[1], b.v[2], dot);
    return 0;
}
