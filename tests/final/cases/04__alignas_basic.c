_Alignas(16) int aligned_global = 4;

int main(void) {
    return aligned_global - 4;
}
