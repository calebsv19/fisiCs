struct Record {
    int id;
};

int main(void) {
    struct Record value = {.missing = 1};
    return value.id;
}
