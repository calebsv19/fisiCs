#define STR(x) #x
const char* s = STR(a + b);
int main(void) { return s[0]; }
