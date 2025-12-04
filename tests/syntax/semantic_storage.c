int arr[];
extern int arr[];
int arr[5];

extern int ext1;
extern int ext1;

int def1;
int def1;  // redefinition

int foo();
int foo(void);

static int s;
static int s; // internal duplicate should diagnose

register int regvar;
int* p = &regvar; // illegal address-of register

int main(void) {
    return arr[0];
}
