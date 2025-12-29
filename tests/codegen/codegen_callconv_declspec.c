#if defined(_MSC_VER) || defined(__clang__) || defined(__GNUC__)
#define API_EXPORT __declspec(dllexport)
#else
#define API_EXPORT
#endif

int __attribute__((stdcall)) cc_sum(int a, int b) {
    return a + b;
}

API_EXPORT int exported_global = 42;

int main(void) {
    return cc_sum(1, 2) + exported_global;
}
