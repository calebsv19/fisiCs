typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long long u64;
extern int printf(const char *, ...);
extern int edu48_frozen_program_valid(const u8 *, u64, u32);
extern u64 edu48_frozen_program_result(const u8 *, u64, u32);
extern u64 edu12_reduce_result(u64, u64, u64);
static void p16(u8 *p,u32 a,u32 v){p[a]=(u8)v;p[a+1U]=(u8)(v>>8U);}
static void p32(u8 *p,u32 a,u32 v){u32 i;for(i=0U;i<4U;i=i+1U)p[a+i]=(u8)(v>>(i*8U));}
static void p64(u8 *p,u32 a,u64 v){u32 i;for(i=0U;i<8U;i=i+1U)p[a+i]=(u8)(v>>(i*8U));}
int main(void){
 u8 p[104]={0};u64 one,two;
 p[0]='E';p[1]='D';p[2]='U';p[3]='3';p[4]='2';p[5]='W';p[6]='1';p16(p,8U,1U);p16(p,10U,1U);p32(p,12U,104U);
 p64(p,16U,0x3FF0000000000000ULL);p64(p,24U,0x4000000000000000ULL);p64(p,32U,0x3FE0000000000000ULL);p64(p,40U,0ULL);p64(p,48U,0x3FF0000000000000ULL);p64(p,56U,0x3FF0000000000000ULL);p64(p,64U,3ULL);p64(p,72U,0x0123456789ABCDEFULL);p64(p,80U,0x4024000000000000ULL);p64(p,88U,0x4022000000000000ULL);
 one=edu12_reduce_result(0x4024000000000000ULL,0x4022000000000000ULL,0x0123456789ABCDEFULL);two=edu12_reduce_result(0x400E800000000000ULL,0x4008000000000000ULL,0x0123456789ABCDEFULL);p64(p,96U,one);
 if(!edu48_frozen_program_valid(p,104ULL,1U)||!edu48_frozen_program_valid(p,104ULL,2U))return 1;
 if(edu48_frozen_program_result(p,104ULL,1U)!=one||edu48_frozen_program_result(p,104ULL,2U)!=two)return 2;
 if(edu48_frozen_program_valid(p,104ULL,3U)||edu48_frozen_program_result(p,104ULL,3U)!=0ULL)return 3;
 printf("OS-POST-EDU19 edu48-frozen-program-selection result=PASS\n");return 0;
}
