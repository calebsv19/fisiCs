typedef int T;

int main(void) {
    int T = 7;
    int expr_size = sizeof T;
    int grouped_expr_size = sizeof(T);
    int type_size = sizeof(int);
    return expr_size + grouped_expr_size + type_size;
}
