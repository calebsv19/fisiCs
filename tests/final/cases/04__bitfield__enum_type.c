enum BitfieldKind {
    BK_ZERO,
    BK_ONE
};

struct EnumBitfield {
    enum BitfieldKind kind : 1;
};
