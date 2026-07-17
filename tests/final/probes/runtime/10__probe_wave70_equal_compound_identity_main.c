#include <stdio.h>

struct wave70_payload {
    int left;
    int right;
};

extern struct wave70_payload *wave70_external_first;
extern struct wave70_payload *wave70_external_second;
struct wave70_payload *wave70_owner_local(void);
struct wave70_payload *wave70_peer_local(void);
int wave70_owner_mutate(void);
int wave70_peer_mutate(void);

static int all_distinct(struct wave70_payload *first,
                        struct wave70_payload *second,
                        struct wave70_payload *owner_local,
                        struct wave70_payload *peer_local) {
    return first != second &&
           first != owner_local &&
           first != peer_local &&
           second != owner_local &&
           second != peer_local &&
           owner_local != peer_local;
}

int main(void) {
    struct wave70_payload *first = wave70_external_first;
    struct wave70_payload *second = wave70_external_second;
    struct wave70_payload *owner_local = wave70_owner_local();
    struct wave70_payload *peer_local = wave70_peer_local();
    int distinct = all_distinct(first, second, owner_local, peer_local);
    int mutated = wave70_owner_mutate() && wave70_peer_mutate();
    int stable = first == wave70_external_first &&
                 second == wave70_external_second &&
                 owner_local == wave70_owner_local() &&
                 peer_local == wave70_peer_local();

    printf("%d %d %d %d %d %d %d %d %d %d\n",
           distinct,
           stable,
           first->left,
           first->right,
           second->left,
           second->right,
           owner_local->left,
           owner_local->right,
           peer_local->left,
           peer_local->right);

    return distinct && mutated && stable &&
                   first->left == 18 && first->right == 23 &&
                   second->left == 17 && second->right == 25 &&
                   owner_local->left == 20 && owner_local->right == 26 &&
                   peer_local->left == 21 && peer_local->right == 31
               ? 0
               : 1;
}
