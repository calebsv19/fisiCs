typedef int (*arr3_t)[3];
typedef arr3_t (*maker_t)(void);

static int storage[3] = {1, 2, 3};

arr3_t ret_array(void) {
    return &storage;
}

maker_t factory(void) {
    return ret_array;
}

int main(void) {
    maker_t fp = factory();
    return fp == 0;
}
