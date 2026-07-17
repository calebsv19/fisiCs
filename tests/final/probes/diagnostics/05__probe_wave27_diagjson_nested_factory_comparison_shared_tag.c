#line 19201 "virtual_wave27_nested_factory_comparison_shared_tag.c"
struct SharedPayload {
    int value;
};

typedef struct SharedPayload (*SharedCallback)(struct SharedPayload);
typedef SharedCallback (*SharedFactory)(SharedCallback);
typedef SharedFactory LeftFactory;
typedef SharedFactory RightFactory;

int main(void) {
    LeftFactory lhs = 0;
    RightFactory rhs = 0;
    return (lhs == rhs) && !(lhs != rhs) ? 0 : 1;
}
