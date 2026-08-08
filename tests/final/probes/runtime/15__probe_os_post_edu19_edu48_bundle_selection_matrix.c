typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long long u64;
extern int puts(const char *);
extern int edu48_frozen_program_valid(const u8 *, u64, u32);
extern u64 edu48_frozen_program_result(const u8 *, u64, u32);
extern u64 edu12_reduce_result(u64, u64, u64);

static void put16(u8 *p, u32 at, u32 v) { p[at]=(u8)v; p[at+1U]=(u8)(v>>8U); }
static void put32(u8 *p, u32 at, u32 v) { u32 i; for(i=0U;i<4U;i=i+1U)p[at+i]=(u8)(v>>(i*8U)); }
static void put64(u8 *p, u32 at, u64 v) { u32 i; for(i=0U;i<8U;i=i+1U)p[at+i]=(u8)(v>>(i*8U)); }
static void clear(u8 *p, u32 n) { u32 i; for(i=0U;i<n;i=i+1U)p[i]=0U; }

static void valid_workload(u8 *p) {
    u64 one;
    clear(p,104U);
    p[0]='E';p[1]='D';p[2]='U';p[3]='3';p[4]='2';p[5]='W';p[6]='1';
    put16(p,8U,1U);put16(p,10U,1U);put32(p,12U,104U);
    put64(p,16U,0x3FF0000000000000ULL);put64(p,24U,0x4000000000000000ULL);put64(p,32U,0x3FE0000000000000ULL);
    put64(p,40U,0ULL);put64(p,48U,0x3FF0000000000000ULL);put64(p,56U,0x3FF0000000000000ULL);
    put64(p,64U,3ULL);put64(p,72U,0x0123456789ABCDEFULL);
    put64(p,80U,0x4024000000000000ULL);put64(p,88U,0x4022000000000000ULL);
    one=edu12_reduce_result(0x4024000000000000ULL,0x4022000000000000ULL,0x0123456789ABCDEFULL);
    put64(p,96U,one);
}

int main(void) {
    u8 p[104];
    u64 one;
    u64 two;
    valid_workload(p);
    one=edu12_reduce_result(0x4024000000000000ULL,0x4022000000000000ULL,0x0123456789ABCDEFULL);
    two=edu12_reduce_result(0x400E800000000000ULL,0x4008000000000000ULL,0x0123456789ABCDEFULL);
    if (!edu48_frozen_program_valid(p,104ULL,1U)) return 1;
    if (!edu48_frozen_program_valid(p,104ULL,2U)) return 2;
    if (edu48_frozen_program_result(p,104ULL,1U)!=one) return 3;
    if (edu48_frozen_program_result(p,104ULL,2U)!=two) return 4;
    if (edu48_frozen_program_valid(p,104ULL,0U)) return 5;
    if (edu48_frozen_program_valid(p,104ULL,3U)) return 6;
    if (edu48_frozen_program_result(p,104ULL,3U)!=0ULL) return 7;
    p[96]^=1U;
    if (edu48_frozen_program_valid(p,104ULL,1U)) return 8;
    valid_workload(p);p[72]^=1U;
    if (edu48_frozen_program_valid(p,104ULL,2U)) return 9;
    valid_workload(p);p[16]=0U;p[17]=0U;p[18]=0U;p[19]=0U;p[20]=0U;p[21]=0U;p[22]=0xF0U;p[23]=0x7FU;
    if (edu48_frozen_program_valid(p,104ULL,1U)) return 10;
    if (edu48_frozen_program_valid((const u8 *)0,104ULL,1U)) return 11;
    if (edu48_frozen_program_valid(p,103ULL,1U)) return 12;
    puts("OS-POST-EDU19 edu48-bundle-selection vectors=12 result=PASS");
    return 0;
}
