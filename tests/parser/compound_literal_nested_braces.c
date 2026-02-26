typedef struct {
    float x;
    float y;
} P;

typedef struct {
    P pos;
    int color;
    P uv;
} V;

void f(void) {
    P a = {1.0f, 2.0f};
    V verts[1];
    int v = 0;
    int color = 7;
    verts[v++] = (V){a, color, {0.0f, 0.0f}};
}
