struct Row {
    int x;
    int y;
};

int main(void) {
    int mode = 0;
    switch (mode {
        case 0:
            mode = 1;
            break;
        default:
            break;
    }
    {
        struct Row row = {2, 7};
        struct Row copy = row;
        int i;
        for (i = 0; i < 2; ++i) {
            copy.y += i;
        }
        return copy.x + copy.y;
    }
}
