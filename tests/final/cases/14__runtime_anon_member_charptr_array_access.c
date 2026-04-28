#include <stdio.h>

typedef struct RuntimeAnonFieldNode RuntimeAnonFieldNode;

struct RuntimeAnonFieldNode {
    struct {
        char* op;
        RuntimeAnonFieldNode* left;
        RuntimeAnonFieldNode* right;
        int is_postfix;
    } expr;
};

static int first_op_char(RuntimeAnonFieldNode* node) {
    return node->expr.op[0];
}

int main(void) {
    char op_buf[] = "+";
    RuntimeAnonFieldNode node;
    node.expr.op = op_buf;
    node.expr.left = 0;
    node.expr.right = 0;
    node.expr.is_postfix = 0;
    printf("%d\n", first_op_char(&node));
    return 0;
}
