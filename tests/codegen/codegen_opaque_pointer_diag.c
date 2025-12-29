// Intentional opaque-pointer exercise: we bitcast an int to an opaque pointer type
// and then try to dereference it. Codegen should not crash; it should emit a
// diagnostic and produce a stub return.

// We don't have a real opaque pointer type handy, so we use a forward-declared
// struct and only ever operate on it through a void* cast at the IR level.

typedef struct OpaqueOpaque OpaqueOpaque;

int main(void) {
    /* Force an opaque pointer compare; LLVM opaque pointers lose element type,
     * so codegen must not crash when attempting to inspect it. */
    OpaqueOpaque* p = (OpaqueOpaque*)(intptr_t)0x1234;
    return p == (OpaqueOpaque*)0 ? 0 : 1;
}
