typedef int wave43_scalar_t;
typedef wave43_scalar_t wave43_row_t[3];

typedef int wave43_route_t(
    int callback(wave43_scalar_t wave43_scalar_t),
    wave43_row_t rows[2],
    wave43_scalar_t tail);

wave43_route_t wave43_route;

int wave43_route(int (*callback)(int), int (*rows)[3], int tail);

int main(void) {
    return 0;
}
