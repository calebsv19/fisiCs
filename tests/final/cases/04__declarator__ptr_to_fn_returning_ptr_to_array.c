static int storage[3] = {1, 2, 3};

int (*ret_array(void))[3] {
    return &storage;
}

int (*(*fp)(void))[3] = ret_array;

int main(void) {
    return (*fp())[2] - 3;
}
