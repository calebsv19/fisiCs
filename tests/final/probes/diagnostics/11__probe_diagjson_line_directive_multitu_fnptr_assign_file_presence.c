#line 14501 "virtual_probe_fn_multitu_fnptr_assign_diagjson_main.c"
int probe_wf20_fi(int x);
int probe_wf20_fd(double x);

int main(void) {
    int (*fp)(int) = probe_wf20_fi;
    int (*gp)(double) = probe_wf20_fd;
    fp = gp;
    return fp(1);
}
