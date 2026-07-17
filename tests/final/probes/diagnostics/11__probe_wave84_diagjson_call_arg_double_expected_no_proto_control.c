#line 19401 "virtual_wave84_call_arg_double_expected_no_proto.c"
typedef int (*Wave84NoPrototypeFn)();
typedef int (*Wave84DoubleFn)(double value);

int wave84_accept_double(Wave84DoubleFn callback);
extern Wave84NoPrototypeFn wave84_no_proto_callback;

int main(void) {
    return wave84_accept_double(wave84_no_proto_callback);
}
