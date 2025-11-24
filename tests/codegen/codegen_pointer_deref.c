int main(void) {
    int buffer[3] = {10, 20, 30};
    int *ptr = &buffer[1];
    int value = *ptr;
    return value;
}
