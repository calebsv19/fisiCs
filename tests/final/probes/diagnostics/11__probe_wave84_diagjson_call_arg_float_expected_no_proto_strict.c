#line 19401 "virtual_wave84_call_arg_float_expected_no_proto.c"
typedef int (*Wave84NoPrototypeFn)();
typedef int (*Wave84FloatFn)(float value);

int wave84_accept_float(Wave84FloatFn callback);
extern Wave84NoPrototypeFn wave84_no_proto_callback;

int main(void) {
    return wave84_accept_float(wave84_no_proto_callback);
}
