int wave86_for_init_vla(int n) {
    goto wave86_for_done;
    for (int wave86_row[n]; n; --n) {
wave86_for_done:
        return 0;
    }
    return 1;
}

int main(void) {
    return wave86_for_init_vla(4);
}
