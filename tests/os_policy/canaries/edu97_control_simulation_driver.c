#include <stdio.h>
typedef unsigned char u8; typedef unsigned int u32; typedef unsigned long long u64;
extern u64 edu21_control_validate_request(const u8 *bytes);
static void w16(u8 *b, unsigned o, unsigned v) { b[o]=(u8)v; b[o+1]=(u8)(v>>8); }
static void w32(u8 *b, unsigned o, u32 v) { unsigned i=0; while(i<4){b[o+i]=(u8)(v>>(i*8));++i;} }
static void w64(u8 *b, unsigned o, u64 v) { unsigned i=0; while(i<8){b[o+i]=(u8)(v>>(i*8));++i;} }
static u32 fnv(const u8 *b) { u32 v=0x811C9DC5U; unsigned i=0; while(i<60){v=(v^b[i])*0x01000193U;++i;} return v; }
static void request75(u8 *b, unsigned version) {
  unsigned i=0; while(i<64){b[i]=0;++i;} b[0]='E';b[1]='D';b[2]='U';b[3]='2';b[4]='1';b[5]='R';b[6]='Q';
  w16(b,8,version);b[10]=75;w16(b,12,32);w64(b,16,41);w16(b,24,1);w16(b,26,2);w32(b,28,4);w32(b,32,0x9191);w64(b,40,0x22);w64(b,48,0x33);w32(b,60,fnv(b));
}
int main(void) {
  u8 b[64]; u64 v30,v31,bad,old,checksum;
  request75(b,30);v30=edu21_control_validate_request(b);request75(b,31);v31=edu21_control_validate_request(b);
  request75(b,30);w32(b,28,3);w32(b,60,fnv(b));bad=edu21_control_validate_request(b);
  request75(b,29);old=edu21_control_validate_request(b);request75(b,31);b[60]^=1;checksum=edu21_control_validate_request(b);
  if(v30||v31||bad!=1||old!=3||checksum!=2){puts("OS-DEV-EDU97 control+simulation result=FAIL");return 1;}
  puts("OS-DEV-EDU97 source=control+simulation policy=durable-snapshot result=PASS");return 0;
}
