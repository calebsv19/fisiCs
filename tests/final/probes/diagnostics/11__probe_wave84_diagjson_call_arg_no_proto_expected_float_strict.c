#line 19401 "virtual_wave84_call_arg_no_proto_expected_float.c"
typedef int (*Wave84NoPrototypeFn)();
typedef int (*Wave84FloatFn)(float value);

int wave84_accept_no_proto(Wave84NoPrototypeFn callback);
extern Wave84FloatFn wave84_float_callback;

int main(void) {
    return wave84_accept_no_proto(wave84_float_callback);
}
