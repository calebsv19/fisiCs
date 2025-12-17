// This should trip the expansion depth limit when FISICS_MACRO_EXP_LIMIT is small.
#define M0 0
#define M1 M0
#define M2 M1
#define M3 M2
#define M4 M3
#define M5 M4
#define M6 M5
#define M7 M6
#define M8 M7
#define M9 M8
#define M10 M9
#define M11 M10
#define M12 M11
#define M13 M12
#define M14 M13
#define M15 M14
#define M16 M15
#define M17 M16
#define M18 M17
#define M19 M18
#define M20 M19

int main(void) {
    return M20;
}
