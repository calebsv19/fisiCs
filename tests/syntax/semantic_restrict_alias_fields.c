struct Pair {
    int cond;
    int mutex;
};

int wait_pair(int *restrict cond, int *restrict mutex) {
    return *cond + *mutex;
}

int main(void) {
    struct Pair p = {1, 2};
    return wait_pair(&p.cond, &p.mutex);
}
