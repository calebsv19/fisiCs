typedef int T;

int main(void) {
    int cast_value = (T)1.75;
    {
        int T = 4;
        int grouped_value = (T) + 1;
        return cast_value + grouped_value;
    }
}
