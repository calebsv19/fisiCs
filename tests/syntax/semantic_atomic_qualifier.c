typedef _Atomic(_Bool) atomic_bool_local;
typedef _Atomic(unsigned long) atomic_ulong_local;

typedef struct {
    _Atomic unsigned long head;
    _Atomic unsigned long tail;
} RingBufferAtomicFields;

int main(void) {
    RingBufferAtomicFields rb = {0};
    atomic_bool_local ready = 0;
    atomic_ulong_local frames = 1ul;
    return (int)(rb.head + rb.tail + frames + (unsigned long)ready);
}
