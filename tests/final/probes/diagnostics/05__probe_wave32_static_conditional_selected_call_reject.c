int wave32_probe_value(void);

#line 32004 "wave32_static_selected_call.c"
static int wave32_static_value = 0 ? 7 : wave32_probe_value();

int main(void) {
    return wave32_static_value;
}
