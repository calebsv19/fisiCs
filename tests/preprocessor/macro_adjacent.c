#define F (1 << 0)     /* object-like; whitespace before '(' */
#define G() 123        /* function-like, zero parameters */
#define H ()           /* object-like; parens separated by space */

int use_macros(void) {
    int x = F;
    int y = G();
    int z = H;         /* should not be treated as function-like */
    return x + y + z;
}

int main(void) {
    return use_macros();
}
