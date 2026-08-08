typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long long u64;
extern int printf(const char *, ...);
extern int edu45_scheduler_handoff_valid(unsigned long, unsigned long);
extern int edu45_cross_model_temporal_admission(
    const u8 *, const u8 *, const u8 *, const u8 *, const u8 *,
    const u8 *, const u8 *, const u8 *, const u8 *, u32, u64);
extern void temporal_fixture_cross_generation(
    u8 *, u8 *, u8 *, u8 *, u8 *, u8 *, u8 *);

static void put32(u8 *p, u32 off, u32 v) { p[off]=(u8)v; p[off+1]=(u8)(v>>8); p[off+2]=(u8)(v>>16); p[off+3]=(u8)(v>>24); }
static void put64(u8 *p, u32 off, u64 v) { u32 i; for(i=0;i<8U;i=i+1U)p[off+i]=(u8)(v>>(i*8U)); }
static void zero(u8 *p, u32 n) { u32 i; for(i=0;i<n;i=i+1U)p[i]=0U; }
static u32 fnv(const u8 *p, u32 n) { u32 h=2166136261U,i; for(i=0;i<n;i=i+1U)h=(h^p[i])*16777619U; return h; }
static void seal(u8 *p, u32 count, u32 at) { put32(p,at,fnv(p,count)); }
static void queue_complete(u8 *p) {
    zero(p,512U); p[0]='E';p[1]='D';p[2]='U';p[3]='1';p[4]='5';p[5]='J';
    put32(p,8U,2U);put32(p,12U,4U);put64(p,16U,0xED22000000000001ULL);put32(p,24U,3U);put32(p,32U,2U);put32(p,36U,2U);
    put64(p,40U,0x6EC4E5DB9E1056CFULL);put64(p,48U,0x1E3C373BAF48FAF7ULL);p[56]=1U;p[58]=32U;p[60]=1U;p[61]=12U;
    p[104]=1U;p[106]=1U;p[108]=11U;put64(p,112U,100ULL);put32(p,120U,3U);p[488]=1U;put64(p,64U,0x6EC4E5DB9E1056CFULL);seal(p,508U,508U);
}
static void wire_v7(u8 *p) { zero(p,64U);put64(p,0U,0x0051523132554445ULL);p[8]=7U;p[10]=17U;put64(p,16U,0xED24000000000012ULL);seal(p,60U,60U); }
int main(void) {
    u8 before[320], restarted[320], old_lanes[1024], new_lanes[1024], entries[4096], old_mailbox[112], new_mailbox[112], queue[512], wire[64];
    queue_complete(queue);wire_v7(wire);
    temporal_fixture_cross_generation(before,restarted,old_lanes,new_lanes,entries,old_mailbox,new_mailbox);
    if (!edu45_cross_model_temporal_admission(queue,wire,old_lanes,new_lanes,entries,before,restarted,old_mailbox,new_mailbox,31U,0x201ULL)) return 1;
    if (!edu45_scheduler_handoff_valid(1UL,0UL)) return 1;
    if (edu45_scheduler_handoff_valid(0UL,1UL)) return 2;
    wire[60]^=1U;
    if (edu45_cross_model_temporal_admission(queue,wire,old_lanes,new_lanes,entries,before,restarted,old_mailbox,new_mailbox,31U,0x201ULL)) return 3;
    printf("OS-POST-EDU19 cross-model-temporal-admission result=PASS\n");
    return 0;
}
