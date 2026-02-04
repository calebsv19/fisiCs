int add(int a, int b);
int add(int a, int b) { return a + b; }
int main(void) {
    int ok = add(1, 2);
    int bad = add(1);
    return ok + bad;
}
