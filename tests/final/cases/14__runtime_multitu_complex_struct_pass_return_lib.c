typedef struct {
    _Complex double left;
    _Complex double right;
    int tag;
} complex_pair_block;

complex_pair_block bridge_mix(complex_pair_block in) {
    complex_pair_block out;
    out.left = in.left + in.right;
    out.right = in.right - (_Complex double)1.0;
    out.tag = in.tag + 9;
    return out;
}
