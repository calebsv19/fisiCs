struct Bucket10HeaderStruct {
    int left;
    int right;
};

struct Bucket10HeaderStruct bucket10_header_struct;

void bucket10_header_struct_prime(int left, int right);
int bucket10_header_struct_shift_left(int delta);
int bucket10_header_struct_shift_right(int delta);
int bucket10_header_struct_total(void);
