int main(void) {
    unsigned int u = 0xffffffffu;
    unsigned int v = u >> 31;
    return (int)v;
}
