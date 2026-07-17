typedef int Wave42BlockStaticFunction(int value);

int main(void) {
    static Wave42BlockStaticFunction wave42_block_static;
    return 0;
}
