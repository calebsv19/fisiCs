#include <stdio.h>
typedef struct S { unsigned int shard, checkpoint, window, lane_a, lane_b; } S;
typedef struct A { unsigned int checkpoint[4], window[4], lane_a[4], lane_b[4]; } A;
void seed12(S* out,unsigned int shard,unsigned int checkpoint,unsigned int window,unsigned int lane_a,unsigned int lane_b);
void encode12(const S* s,unsigned int wire[5]);
void decode12(S* s,const unsigned int wire[5]);
void clear12(A* a);
void absorb12(A* a,const S* s);
unsigned int sig12(const A* a);
int main(void){ S s0_old,s0_new,s1_old,s1_new,s2_old,s2_new,s3_old,s3_new,d; A c,r; unsigned int w0_old[5],w0_new[5],w1_old[5],w1_new[5],w2_old[5],w2_new[5],w3_old[5],w3_new[5]; seed12(&s0_old,0u,3u,2u,5u,6u); seed12(&s0_new,0u,6u,5u,9u,11u); seed12(&s1_old,1u,2u,1u,4u,7u); seed12(&s1_new,1u,7u,6u,10u,12u); seed12(&s2_old,2u,4u,3u,8u,5u); seed12(&s2_new,2u,8u,7u,14u,15u); seed12(&s3_old,3u,5u,2u,3u,9u); seed12(&s3_new,3u,9u,8u,16u,13u); encode12(&s0_old,w0_old); encode12(&s0_new,w0_new); encode12(&s1_old,w1_old); encode12(&s1_new,w1_new); encode12(&s2_old,w2_old); encode12(&s2_new,w2_new); encode12(&s3_old,w3_old); encode12(&s3_new,w3_new); clear12(&c); absorb12(&c,&s0_new); absorb12(&c,&s1_new); absorb12(&c,&s2_new); absorb12(&c,&s3_new); clear12(&r); decode12(&d,w0_old); absorb12(&r,&d); decode12(&d,w2_new); absorb12(&r,&d); decode12(&d,w1_old); absorb12(&r,&d); decode12(&d,w3_new); absorb12(&r,&d); decode12(&d,w0_new); absorb12(&r,&d); decode12(&d,w2_old); absorb12(&r,&d); decode12(&d,w1_new); absorb12(&r,&d); decode12(&d,w3_old); absorb12(&r,&d); { unsigned int sc=sig12(&c), sr=sig12(&r); printf("%u %u %u\n", sc, sr, (sc==sr)?1u:0u); } return 0; }
