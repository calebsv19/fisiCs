// Ensure VLA parameters parse and decay to pointer in calls.
int takes_array(int n, int a[n]) { return a[0] + a[n - 1]; }

int main(void) {
    int n = 3;
    int vla[n];
    vla[0] = 5;
    vla[1] = 6;
    vla[2] = 7;
    return takes_array(n, vla);
}
