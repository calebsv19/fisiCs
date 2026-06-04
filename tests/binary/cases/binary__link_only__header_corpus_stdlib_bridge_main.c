int wave26_stdlib_bridge_score(const char *text);

int main(void) {
    return wave26_stdlib_bridge_score("-42xyz") == 45 ? 0 : 1;
}
