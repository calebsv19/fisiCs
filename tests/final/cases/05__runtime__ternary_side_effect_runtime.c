int main(void) {
    int i = 0;
    int v = i++ ? i++ : i++;
    return (i == 2 && v == 1) ? 0 : 1;
}
