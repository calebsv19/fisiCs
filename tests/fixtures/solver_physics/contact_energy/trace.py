"""Independent scalar account/coverage observer for the seven fixed three-body cases.
Not a replacement for the independent sticky-cluster trajectory test or authentication.
"""
import math

def validate(text):
 ticks={};expected={};seen={};rows={};groups=0
 for line in text.splitlines():
  t=line.split()
  if not t or t[0] not in ('T','G'):continue
  c,k=map(int,t[1:3]);assert 0<=c<7 and 1<=k<=2000
  key=(c,k)
  if t[0]=='T':
   assert len(t)==16 and k==ticks.get(c,0)+1;ticks[c]=k
   before,loss=map(float,t[3:5]);n,ng=map(int,t[5:7]);v=list(map(float,t[7:]));assert all(map(math.isfinite,[before,loss,*v]));assert loss>=0
   assert abs(sum(.5*v[j]*v[j+2]**2 for j in (0,3,6))-before+loss)<=2e-10
   expected[key]=(ng,n,loss);rows[key]=[]
  else:
   assert key in expected and key==(c,ticks[c]);assert int(t[3])==seen.get(key,0);seen[key]=seen.get(key,0)+1;groups+=1
   before,after,loss,dp=map(float,t[4:8]);n=int(t[8]);assert n in (1,2) and len(t)==18+5*n
   v=list(map(float,t[9:18]));assert all(map(math.isfinite,[before,after,loss,dp,*v]));m=v[::3];old=v[1::3];new=v[2::3]
   assert all(.25<=x<=4 for x in m)
   assert abs(sum(.5*m[i]*old[i]**2 for i in range(3))-before)<=2e-10
   assert abs(sum(.5*m[i]*new[i]**2 for i in range(3))-after)<=2e-10
   assert abs(before-after-loss)<=2e-10 and loss>=-1e-10*max(1,before)
   transfer=[0.,0.,0.]
   for i in range(n):
    a,b=map(int,t[18+5*i:20+5*i]);j,lo,hi=map(float,t[20+5*i:23+5*i]);assert 0<=a<3 and 0<=b<3 and a!=b;assert all(map(math.isfinite,[j,lo,hi]));assert j>=0 and 0<=lo<=hi<=.001 and hi-lo<=4e-12;transfer[a]-=j;transfer[b]+=j
   assert all(abs(m[i]*(new[i]-old[i])-transfer[i])<=2e-10 for i in range(3));assert abs(dp)<=2e-10
   rows[key].append((n,max(0,loss)))
 assert ticks==dict.fromkeys(range(7),2000)
 for key,(ng,n,loss) in expected.items():
  assert seen.get(key,0)==ng and sum(x[0] for x in rows[key])==n
  assert abs(sum(x[1] for x in rows[key])-loss)<=2e-10
 return {'passed':True,'accepted_ticks':len(expected),'accepted_groups':groups,'event_rows':sum(v[1] for v in expected.values())}
