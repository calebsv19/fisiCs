struct P { int a; int b; };

int use_auto(void) {
    struct P *p = &(struct P){3, 4};
    return p->a + p->b;
}

int main(void) {
    struct P *p = &(struct P){1, 2};
    return p->a + p->b + use_auto();
}
