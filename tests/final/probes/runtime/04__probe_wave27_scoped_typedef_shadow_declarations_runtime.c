extern int printf(const char*, ...);

typedef int wave27_shadow_t;

static int wave27_scoped_typedef_shadow_declarations(int cols) {
    wave27_shadow_t outer = 5;
    int total = outer;

    {
        typedef struct {
            int value;
        } wave27_shadow_t;

        wave27_shadow_t object = {17};
        total += object.value;

        {
            typedef int wave27_shadow_t[cols];
            wave27_shadow_t row;
            for (int i = 0; i < cols; ++i) {
                row[i] = object.value + i;
            }
            total += row[cols - 1] + (int)sizeof(row);
        }
    }

    {
        wave27_shadow_t after = 9;
        total += after;
    }

    return total;
}

int main(void) {
    printf("%d\n", wave27_scoped_typedef_shadow_declarations(4));
    return 0;
}
