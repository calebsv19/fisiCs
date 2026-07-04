#include <stdio.h>
typedef struct S { unsigned int shard, lane, checkpoint, watermark, payload; } S;
typedef struct A { unsigned int checkpoint[4], watermark[4], payload[4]; } A;
void seed13(S* out,unsigned int shard,unsigned int lane,unsigned int checkpoint,unsigned int watermark,unsigned int payload);
void encode13(const S* s,unsigned int wire[5]);
void decode13(S* s,const unsigned int wire[5]);
void clear13(A* a);
void absorb13(A* a,const S* s);
unsigned int sig13(const A* a);
int main(void){ S s0_old,s0_new,s1_old,s1_new,s2_old,s2_new,s3_old,s3_new,d; A canonical,replayed; unsigned int w0_old[5],w0_new[5],w1_old[5],w1_new[5],w2_old[5],w2_new[5],w3_old[5],w3_new[5]; seed13(&s0_old,0u,0u,4u,2u,5u); seed13(&s0_new,0u,0u,8u,7u,11u); seed13(&s1_old,1u,1u,3u,1u,6u); seed13(&s1_new,1u,1u,9u,8u,13u); seed13(&s2_old,2u,2u,5u,3u,7u); seed13(&s2_new,2u,2u,10u,9u,17u); seed13(&s3_old,3u,3u,6u,4u,8u); seed13(&s3_new,3u,3u,11u,10u,19u); encode13(&s0_old,w0_old); encode13(&s0_new,w0_new); encode13(&s1_old,w1_old); encode13(&s1_new,w1_new); encode13(&s2_old,w2_old); encode13(&s2_new,w2_new); encode13(&s3_old,w3_old); encode13(&s3_new,w3_new); clear13(&canonical); absorb13(&canonical,&s0_new); absorb13(&canonical,&s1_new); absorb13(&canonical,&s2_new); absorb13(&canonical,&s3_new); clear13(&replayed); decode13(&d,w2_old); absorb13(&replayed,&d); decode13(&d,w0_new); absorb13(&replayed,&d); decode13(&d,w1_old); absorb13(&replayed,&d); decode13(&d,w3_new); absorb13(&replayed,&d); decode13(&d,w2_new); absorb13(&replayed,&d); decode13(&d,w0_old); absorb13(&replayed,&d); decode13(&d,w1_new); absorb13(&replayed,&d); decode13(&d,w3_old); absorb13(&replayed,&d); decode13(&d,w1_old); absorb13(&replayed,&d); decode13(&d,w0_old); absorb13(&replayed,&d); { unsigned int sc=sig13(&canonical), sr=sig13(&replayed); printf("%u %u %u\n", sc, sr, (sc==sr)?1u:0u); } return 0; }
