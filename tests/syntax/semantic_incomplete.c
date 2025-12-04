struct Node;

struct Node* ok_ptr;
struct Node ok_obj;           // invalid: incomplete object
struct Node arr[2];           // invalid: array of incomplete

int bad_sizeof(void) {
    return sizeof(struct Node); // invalid: sizeof incomplete
}

struct Node {
    int value;
};

int good(void) {
    struct Node n;
    return sizeof(n);
}
