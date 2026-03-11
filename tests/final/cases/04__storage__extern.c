extern int g_extern;
int g_extern = 11;

int read_extern(void) {
    return g_extern;
}
