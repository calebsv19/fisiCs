int fi(int x) { return x; }
int fd(double x) { return (int)x; }

int main(void) {
    int (*fp)(int) = fi;
    int (*gp)(double) = fd;
    fp = gp;
    return fp(1);
}
