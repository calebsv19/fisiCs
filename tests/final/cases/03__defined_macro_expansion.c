#define CHECK(x) defined(x)

#if CHECK(FOO)
#error "defined() should see FOO as undefined"
#endif

#define FOO 1

#if !CHECK(FOO)
#error "defined() should see FOO as defined"
#endif

int main(void) { return 0; }
