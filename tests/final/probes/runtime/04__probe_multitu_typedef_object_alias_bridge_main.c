extern int printf(const char*, ...);

typedef unsigned wave15_word_t;
extern wave15_word_t wave15_multitu_object_alias_bridge(void);

int main(void) {
    printf("%u\n", wave15_multitu_object_alias_bridge());
    return 0;
}
