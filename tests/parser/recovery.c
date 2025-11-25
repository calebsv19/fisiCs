int global_missing_semicolon = 5

int main(void) {
    int first = 0
    bad_call(;
    int second = 2;
    if (second > 0) {
        return second
    }

    return second;
}
