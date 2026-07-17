_Pragma("STDC FP_CONTRACT OFF") enum { pragma_direct_value = 39 };

int main(void) {
    return pragma_direct_value == 39 ? 0 : 1;
}
