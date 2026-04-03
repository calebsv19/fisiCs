#define TOK_FLAG_CONST 0x01u
#define TOK_FLAG_VOLATILE 0x02u
#define TOK_FLAG_SIGNED 0x04u
#define TOK_FLAG_UNSIGNED 0x08u

#define FOLD2(a, b) ((a) | (b))
#define FOLD3(a, b, c) FOLD2(FOLD2((a), (b)), (c))

typedef struct {
    unsigned mask;
    unsigned rank;
} QualPack;

static unsigned pack_rank(unsigned mask) {
    unsigned r = 0u;
    if (mask & TOK_FLAG_CONST) r += 1u;
    if (mask & TOK_FLAG_VOLATILE) r += 2u;
    if (mask & TOK_FLAG_SIGNED) r += 4u;
    if (mask & TOK_FLAG_UNSIGNED) r += 8u;
    return r;
}

QualPack make_qual_pack(unsigned is_signed, unsigned is_const) {
    QualPack q;
    q.mask = FOLD3(is_const ? TOK_FLAG_CONST : 0u,
                   TOK_FLAG_VOLATILE,
                   is_signed ? TOK_FLAG_SIGNED : TOK_FLAG_UNSIGNED);
    q.rank = pack_rank(q.mask);
    return q;
}
