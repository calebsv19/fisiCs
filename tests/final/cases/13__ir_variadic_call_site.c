int tag_only(int tag, ...) {
    return tag;
}

int main(void) {
    return tag_only(7, 1, 2, 3, 4);
}
