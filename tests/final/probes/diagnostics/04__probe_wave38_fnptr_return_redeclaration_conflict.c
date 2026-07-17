typedef int (*probe_return_int)(int value);
typedef int (*probe_return_double)(double value);

probe_return_int probe_factory(void);

#line 38005 "virtual_decl_wave38_fnptr_return_conflict.c"
probe_return_double probe_factory(void) {
    return (probe_return_double)0;
}
