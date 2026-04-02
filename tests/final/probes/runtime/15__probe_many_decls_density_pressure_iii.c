#include <stdio.h>

typedef struct {
    unsigned a;
    unsigned b;
    unsigned c;
    unsigned d;
    unsigned e;
} Pack5;

static unsigned fold10(
    unsigned a, unsigned b, unsigned c, unsigned d, unsigned e,
    unsigned f, unsigned g, unsigned h, unsigned i, unsigned j,
    unsigned salt
) {
    unsigned acc = 2166136261u ^ salt;
    acc ^= a + 3u * b + 5u * c + 7u * d + 11u * e;
    acc *= 16777619u;
    acc ^= f + 13u * g + 17u * h + 19u * i + 23u * j;
    acc *= 16777619u;
    acc ^= acc >> 15u;
    return acc;
}

int main(void) {
    unsigned v00 = 1u,   v01 = 2u,   v02 = 3u,   v03 = 4u,   v04 = 5u;
    unsigned v05 = 6u,   v06 = 7u,   v07 = 8u,   v08 = 9u,   v09 = 10u;
    unsigned v10 = 11u,  v11 = 12u,  v12 = 13u,  v13 = 14u,  v14 = 15u;
    unsigned v15 = 16u,  v16 = 17u,  v17 = 18u,  v18 = 19u,  v19 = 20u;
    unsigned v20 = 21u,  v21 = 22u,  v22 = 23u,  v23 = 24u,  v24 = 25u;
    unsigned v25 = 26u,  v26 = 27u,  v27 = 28u,  v28 = 29u,  v29 = 30u;
    unsigned v30 = 31u,  v31 = 32u,  v32 = 33u,  v33 = 34u,  v34 = 35u;
    unsigned v35 = 36u,  v36 = 37u,  v37 = 38u,  v38 = 39u,  v39 = 40u;
    unsigned v40 = 41u,  v41 = 42u,  v42 = 43u,  v43 = 44u,  v44 = 45u;
    unsigned v45 = 46u,  v46 = 47u,  v47 = 48u,  v48 = 49u,  v49 = 50u;
    unsigned v50 = 51u,  v51 = 52u,  v52 = 53u,  v53 = 54u,  v54 = 55u;
    unsigned v55 = 56u,  v56 = 57u,  v57 = 58u,  v58 = 59u,  v59 = 60u;
    unsigned v60 = 61u,  v61 = 62u,  v62 = 63u,  v63 = 64u,  v64 = 65u;
    unsigned v65 = 66u,  v66 = 67u,  v67 = 68u,  v68 = 69u,  v69 = 70u;
    unsigned v70 = 71u,  v71 = 72u,  v72 = 73u,  v73 = 74u,  v74 = 75u;
    unsigned v75 = 76u,  v76 = 77u,  v77 = 78u,  v78 = 79u,  v79 = 80u;
    unsigned v80 = 81u,  v81 = 82u,  v82 = 83u,  v83 = 84u,  v84 = 85u;
    unsigned v85 = 86u,  v86 = 87u,  v87 = 88u,  v88 = 89u,  v89 = 90u;
    unsigned v90 = 91u,  v91 = 92u,  v92 = 93u,  v93 = 94u,  v94 = 95u;
    unsigned v95 = 96u,  v96 = 97u,  v97 = 98u,  v98 = 99u,  v99 = 100u;

    Pack5 p0 = {3u, 5u, 7u, 11u, 13u};
    Pack5 p1 = {17u, 19u, 23u, 29u, 31u};
    unsigned acc = 0u;

    acc ^= fold10(v00, v01, v02, v03, v04, v05, v06, v07, v08, v09, 3u);
    acc ^= fold10(v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, 5u);
    acc ^= fold10(v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, 7u);
    acc ^= fold10(v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, 11u);
    acc ^= fold10(v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, 13u);
    acc ^= fold10(v50, v51, v52, v53, v54, v55, v56, v57, v58, v59, 17u);
    acc ^= fold10(v60, v61, v62, v63, v64, v65, v66, v67, v68, v69, 19u);
    acc ^= fold10(v70, v71, v72, v73, v74, v75, v76, v77, v78, v79, 23u);
    acc ^= fold10(v80, v81, v82, v83, v84, v85, v86, v87, v88, v89, 29u);
    acc ^= fold10(v90, v91, v92, v93, v94, v95, v96, v97, v98, v99, 31u);

    acc ^= fold10(p0.a, p0.b, p0.c, p0.d, p0.e, p1.a, p1.b, p1.c, p1.d, p1.e, 37u);
    printf("%u\n", acc);
    return 0;
}
