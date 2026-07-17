struct ProbePair {
    int x;
    int y;
};

int main(void) {
    struct ProbePair pair = { .x = 1, .y = };
    return pair.x + follow_on_after_bad_initializer;
}
