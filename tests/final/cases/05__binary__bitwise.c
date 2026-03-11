int main(void) {
    unsigned a = 0x55u;
    unsigned b = 0x0fu;
    unsigned c = (a & b) ^ (a | b);
    return c == 0x5au ? 0 : 1;
}
