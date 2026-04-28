typedef int Arr[1];

Arr * (pick_storage)(void);

static Arr storage = { 9 };

Arr *pick_storage(void) {
    return &storage;
}

int main(void) {
    Arr *ptr = pick_storage();
    return (*ptr)[0] != 9;
}
