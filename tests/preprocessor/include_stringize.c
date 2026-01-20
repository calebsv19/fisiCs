#define STR2(x) #x
#define STR(x) STR2(x)
#define HDR shadow.h

#include STR(HDR)

int main(void) {
    return SHADOW_VAL == 99 ? 0 : 1;
}
