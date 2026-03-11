int main(int n) {
    goto after_vla;
    int vla[n];
after_vla:
    return 0;
}
