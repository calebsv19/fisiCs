int wave27_errno_bridge_score(const char *path);

int main(void) {
    return wave27_errno_bridge_score("wave27_missing.file") > 0 ? 0 : 1;
}
