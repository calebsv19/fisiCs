union Word {
    int i;
    unsigned int u;
};

static union Word step(union Word w) {
    w.u += 3u;
    return w;
}

int main(void) {
    union Word w;
    w.i = 10;
    w = step(w);
    return (int)w.u;
}
