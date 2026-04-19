typedef struct Pair {
    int x;
    int y;
} Pair;

typedef struct Bucket {
    Pair pairs[4];
    long total;
    double weight;
} Bucket;

typedef union Payload {
    Bucket bucket;
    unsigned char bytes[64];
} Payload;

typedef struct Node {
    int id;
    Payload payload;
    struct Node* next;
} Node;

static Bucket buckets[4] = {
    { { {1, 2}, {3, 4}, {5, 6}, {7, 8} }, 10, 1.0 },
    { { {2, 3}, {4, 5}, {6, 7}, {8, 9} }, 20, 2.0 },
    { { {3, 4}, {5, 6}, {7, 8}, {9, 10} }, 30, 3.0 },
    { { {4, 5}, {6, 7}, {8, 9}, {10, 11} }, 40, 4.0 }
};

static Node node0 = { 0, { .bucket = { { {1, 1}, {2, 2}, {3, 3}, {4, 4} }, 11, 1.5 } }, 0 };
static Node node1 = { 1, { .bucket = { { {5, 5}, {6, 6}, {7, 7}, {8, 8} }, 22, 2.5 } }, &node0 };
static Node node2 = { 2, { .bucket = { { {9, 9}, {10, 10}, {11, 11}, {12, 12} }, 33, 3.5 } }, &node1 };

static Node* head = &node2;

int main(void) {
    return head->next->payload.bucket.pairs[1].x + buckets[2].pairs[3].y;
}
