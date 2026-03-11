int main(void) {
    int x = 7;
    void *vp = &x;
    int *ip = vp;
    return *ip;
}
