// Expect: case label rejected when not an ICE.
int g(int x) {
    switch (x) {
    case (x + 1): // non-ICE
        return 1;
    default:
        return 0;
    }
}
