#line 14101 "virtual_fn_multitu_fnptr_assign_diagjson_wave20_main.c"
int wf20_fi(int x);
int wf20_fd(double x);

int main(void) {
    int (*fp)(int) = wf20_fi;
    int (*gp)(double) = wf20_fd;
    fp = gp;
    return fp(1);
}
