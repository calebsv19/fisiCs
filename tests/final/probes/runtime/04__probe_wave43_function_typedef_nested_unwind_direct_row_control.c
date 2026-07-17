typedef int wave43_control_scalar_t;
typedef wave43_control_scalar_t wave43_control_row_t[3];

typedef int wave43_control_route_t(
    int callback(wave43_control_scalar_t wave43_control_scalar_t),
    wave43_control_row_t row,
    wave43_control_scalar_t tail);

wave43_control_route_t wave43_control_route;

int wave43_control_route(int (*callback)(int), int *row, int tail);

int main(void) {
    return 0;
}
