enum { WAVE44_INNER_BOUND = 3 };

int wave44_bound_conflict(int (*rows)[WAVE44_INNER_BOUND]);
int wave44_bound_conflict(int (*rows)[sizeof(char) + 3]);

int main(void) {
    return 0;
}
