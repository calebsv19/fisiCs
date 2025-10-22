typedef int MyAlias;

int main(void) {
    MyAlias value = 7;
    int casted = (MyAlias)value;
    int grouped = (value);
    return casted + grouped;
}
