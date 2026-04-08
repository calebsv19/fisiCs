static int storage[3] = {1, 2, 3};

int (*ret_array(void))[3] {
    return &storage;
}

int (*(*factory(void))(void))[3] {
    return ret_array;
}

int main(void) {
    int (*(*fp)(void))[3] = factory();
    return fp == 0;
}
