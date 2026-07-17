#define W31_PAIR(a, b) ((a) + (b))
#define W31_WRAP(x) W31_PAIR(x)
#define W31_STR2(x) #x
#define W31_STR(x) W31_STR2(x)

#line 1330 "virtual_wave31_diagjson_text_parity.c"
const char *wave31_diagjson_text_parity = W31_STR(W31_PAIR(left, right));
int wave31_diagjson_text_parity_error = W31_WRAP(9);
