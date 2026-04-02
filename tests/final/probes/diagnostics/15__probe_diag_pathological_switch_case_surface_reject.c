int run_switch(int x, int y) {
    switch (x) {
        case 0:
            return x + 1;
        case (y + 2):
            return y + 3;
        default:
            return x - y;
    }
}

int main(void) {
    return run_switch(2, 5);
}
