#define INNER(x) ((x) + 1)
#define WRAP(y) (INNER(y) * INNER(BASE))
#define BASE 3

int main() {
    return WRAP(2);
}
