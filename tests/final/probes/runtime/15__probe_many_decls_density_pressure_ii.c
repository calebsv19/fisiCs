#include <stdio.h>

typedef struct {
    unsigned a;
    unsigned b;
    unsigned c;
    unsigned d;
} Pack;

static unsigned fold8(
    unsigned a, unsigned b, unsigned c, unsigned d,
    unsigned e, unsigned f, unsigned g, unsigned h,
    unsigned salt
) {
    unsigned acc = 2166136261u ^ salt;
    acc ^= a + 3u * b + 5u * c + 7u * d;
    acc *= 16777619u;
    acc ^= e + 11u * f + 13u * g + 17u * h;
    acc *= 16777619u;
    acc ^= (acc >> 13);
    return acc;
}

int main(void) {
    unsigned v00 = 1u,  v01 = 2u,  v02 = 3u,  v03 = 4u,  v04 = 5u,  v05 = 6u,  v06 = 7u,  v07 = 8u;
    unsigned v08 = 9u,  v09 = 10u, v10 = 11u, v11 = 12u, v12 = 13u, v13 = 14u, v14 = 15u, v15 = 16u;
    unsigned v16 = 17u, v17 = 18u, v18 = 19u, v19 = 20u, v20 = 21u, v21 = 22u, v22 = 23u, v23 = 24u;
    unsigned v24 = 25u, v25 = 26u, v26 = 27u, v27 = 28u, v28 = 29u, v29 = 30u, v30 = 31u, v31 = 32u;
    unsigned v32 = 33u, v33 = 34u, v34 = 35u, v35 = 36u, v36 = 37u, v37 = 38u, v38 = 39u, v39 = 40u;
    unsigned v40 = 41u, v41 = 42u, v42 = 43u, v43 = 44u, v44 = 45u, v45 = 46u, v46 = 47u, v47 = 48u;
    unsigned v48 = 49u, v49 = 50u, v50 = 51u, v51 = 52u, v52 = 53u, v53 = 54u, v54 = 55u, v55 = 56u;
    unsigned v56 = 57u, v57 = 58u, v58 = 59u, v59 = 60u, v60 = 61u, v61 = 62u, v62 = 63u, v63 = 64u;
    Pack p0 = {3u, 5u, 7u, 11u};
    Pack p1 = {13u, 17u, 19u, 23u};
    unsigned acc = 0u;

    acc ^= fold8(v00, v01, v02, v03, v04, v05, v06, v07, 3u);
    acc ^= fold8(v08, v09, v10, v11, v12, v13, v14, v15, 5u);
    acc ^= fold8(v16, v17, v18, v19, v20, v21, v22, v23, 7u);
    acc ^= fold8(v24, v25, v26, v27, v28, v29, v30, v31, 11u);
    acc ^= fold8(v32, v33, v34, v35, v36, v37, v38, v39, 13u);
    acc ^= fold8(v40, v41, v42, v43, v44, v45, v46, v47, 17u);
    acc ^= fold8(v48, v49, v50, v51, v52, v53, v54, v55, 19u);
    acc ^= fold8(v56, v57, v58, v59, v60, v61, v62, v63, 23u);

    {
        unsigned t00 = 101u, t01 = 103u, t02 = 107u, t03 = 109u;
        unsigned t04 = 113u, t05 = 127u, t06 = 131u, t07 = 137u;
        unsigned t08 = 139u, t09 = 149u, t10 = 151u, t11 = 157u;
        unsigned t12 = 163u, t13 = 167u, t14 = 173u, t15 = 179u;

        acc ^= fold8(t00, t01, t02, t03, t04, t05, t06, t07, 29u);
        acc ^= fold8(t08, t09, t10, t11, t12, t13, t14, t15, 31u);
    }

    acc ^= fold8(p0.a, p0.b, p0.c, p0.d, p1.a, p1.b, p1.c, p1.d, 37u);
    printf("%u\n", acc);
    return 0;
}
