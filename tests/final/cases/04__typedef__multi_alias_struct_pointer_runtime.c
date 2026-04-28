typedef struct {
    int x;
} Box, *BoxPtr;

static int box_value(BoxPtr ptr) {
    return ptr ? ptr->x : -1;
}

int main(void) {
    Box value = { 7 };
    BoxPtr ptr = &value;
    return box_value(ptr) != 7;
}
