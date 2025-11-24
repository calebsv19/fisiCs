int main(void) {
    const int c = 0;
    int array[2] = {1, 2};
    int *ptr = array;

    int value = c;
    c = 5;            // const assignment
    ++(ptr + 1);      // increment rvalue pointer expression

    return value;
}
