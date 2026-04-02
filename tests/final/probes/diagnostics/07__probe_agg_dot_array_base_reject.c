union U {
    int raw[2];
};

int main(void) {
    union U u = {{1, 2}};
    return u.raw.missing;
}
