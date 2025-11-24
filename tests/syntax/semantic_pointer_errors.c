int main(void) {
    int values[3] = {1, 2, 3};
    float f = 1.5f;
    int *p = values;
    int *q = values + 1;

    int sum = p + q;      // invalid: pointer + pointer
    int diff = p - (int)f;  // invalid: pointer - scalar
    int cmp = p < f;        // invalid: pointer compare scalar

    return sum + diff + cmp;
}
