struct Pair {
    int x;
    int y;
};

int main(void) {
    int scalar_ok = 1;
    int scalar_bad = {1, 2};

    struct Pair p_bad = 5;

    int arr_too_many[2] = {1, 2, 3};
    int arr_too_few[3] = {1};
    int arr_designator[2] = { [2] = 5 };

    int nested[1] = { {1, 2} };

    char label[3] = { "abcd" };

    return scalar_ok + arr_too_many[0] + p_bad.x + arr_too_few[0] + arr_designator[0] + nested[0] + label[0];
}
