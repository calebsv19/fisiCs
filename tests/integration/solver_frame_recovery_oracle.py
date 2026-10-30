"""Exact local Euler-Bernoulli load/recovery and spring oracle for fixtures."""
from fractions import Fraction as F
from run_units_frame_local_contracts import beam_matrix
from run_units_frame_boundary_contracts import INPUTS, BASES

SECTIONS=[row+[F(z)] for row,z in zip(INPUTS,['.002','.0006666666666666667','.00003'])]
BASES_RECOVERY=[BASES[0],BASES[3],BASES[4]]
Q=[[F(x) for x in row] for row in [['.001','-.002','.005','.003','.004','-.006'],['-.002','.003','-.004','-.001','.002','.007']]]
NODAL=[F(x) for x in ['1','-2','3','4','5','-6']]

def add(a,b):return [(a[i] if i<len(a) else F(0))+(b[i] if i<len(b) else F(0)) for i in range(max(len(a),len(b)))]
def scale(a,s):return [x*s for x in a]
def mul(a,b):
    c=[F(0)]*(len(a)+len(b)-1)
    for i,x in enumerate(a):
        for j,y in enumerate(b):c[i+j]+=x*y
    return c
def derivative(a):return [i*x for i,x in enumerate(a)][1:]
def integrate(a):return sum(x/F(i+1) for i,x in enumerate(a))
def evaluate(a,x):return sum(v*x**i for i,v in enumerate(a))
def mv(k,q):return [sum(x*y for x,y in zip(row,q)) for row in k]
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def to_global(p,basis):
    c,s=basis;out=list(p)
    for a in [0,3]:out[a]=c*p[a]-s*p[a+1];out[a+1]=s*p[a]+c*p[a+1]
    return out

def recover_q(k,f,q,mask):
    q=list(q);release=[i for i,flag in [(2,1),(5,2)] if mask&flag]
    for i in release:q[i]=F(0)
    rhs=[a-b for a,b in zip(f,mv(k,q))]
    if len(release)==1:q[release[0]]=rhs[release[0]]/k[release[0]][release[0]]
    elif len(release)==2:
        a,b=release;det=k[a][a]*k[b][b]-k[a][b]*k[b][a]
        q[a]=(k[b][b]*rhs[a]-k[a][b]*rhs[b])/det
        q[b]=(k[a][a]*rhs[b]-k[b][a]*rhs[a])/det
    return q

def field_values(section,line,q):
    E,A,I,L,Z=section;ea=E*A;ei=E*I
    # Hermite interpolation plus the zero-end-displacement particular solution.
    u=add([q[0],q[3]-q[0],F(0)],scale([F(0),F(1),F(-1)],line[0]*L*L/(2*ea)))
    v=[F(0)]*5
    for weight,basis in [(q[1],[1,0,-3,2]),(q[2]*L,[0,1,-2,1]),(q[4],[0,0,3,-2]),(q[5]*L,[0,0,-1,1])]:v=add(v,scale(basis,weight))
    v=add(v,scale([0,0,1,-2,1],line[1]*L**4/(24*ei)))
    n=scale(derivative(u),ea/L);moment=scale(derivative(derivative(v)),ei/L**2)
    shear=scale(derivative(moment),1/L)
    tension=F(0);compression=F(0)
    for sign in [-1,1]:
        p=add(scale(n,1/A),scale(moment,F(sign)/Z));points=[F(0),F(1)]
        if p[2]:
            xi=-p[1]/(2*p[2])
            if 0<xi<1:points.append(xi)
        for xi in points:
            stress=evaluate(p,xi);tension=max(tension,stress);compression=max(compression,-stress)
    maximum_shear=F(3,2)*max(abs(evaluate(shear,F(0))),abs(evaluate(shear,F(1))))/A
    energy=L*(integrate(mul(n,n))/ea+integrate(mul(moment,moment))/ei)/2
    work=L*(line[0]*integrate(u)+line[1]*integrate(v))
    return u+v+n+moment+shear+[tension,compression,maximum_shear,energy,work]

def expected_values():
    expected={}
    def record(kind,id,values):
        for i,v in enumerate(values):expected[kind,id,i]=F(v)
    for si,section in enumerate(SECTIONS):
        E,A,I,L,Z=section;k=beam_matrix(E,A,I,L)
        for mask in range(4):
            for bi,basis in enumerate(BASES_RECOVERY):
                id=(si*4+mask)*3+bi;c,s=basis
                gx=600*A*F('.5')+12;gy=600*A*F('-9.81')-34
                line=[c*gx+s*gy-7,-s*gx+c*gy+11]
                f=[line[0]*L/2,line[1]*L/2,line[1]*L**2/12,line[0]*L/2,line[1]*L/2,-line[1]*L**2/12]
                rotations=recover_q(k,f,[F(0)]*6,mask);correction=mv(k,rotations)
                condensed=[a-b for a,b in zip(f,correction)]
                for r,flag in [(2,1),(5,2)]:
                    if mask&flag:condensed[r]=F(0)
                world=to_global(condensed,basis);combined=add(world,NODAL)
                record('B',id,line+f+condensed+world+combined)
                for pattern,q0 in enumerate(Q):
                    q=recover_q(k,f,q0,mask);end=[a-b for a,b in zip(mv(k,q),f)]
                    fields=field_values(section,line,q)
                    end_work=dot(q,end);supplied_work=dot(to_global(q,basis),NODAL)
                    assert 2*fields[-2]==end_work+fields[-1]
                    record('R',2*id+pattern,q+end+fields+[end_work,supplied_work])
    for mask in range(8):
        k=[[F(0)]*6 for _ in range(6)]
        for dof,stiffness in enumerate([120000,30000,800]):
            if mask&(1<<dof):
                for end1,sign1 in [(0,-1),(3,1)]:
                    for end2,sign2 in [(0,-1),(3,1)]:k[dof+end1][dof+end2]=F(stiffness*sign1*sign2)
        for pattern,q in enumerate(Q):
            id=mask*2+pattern;delta=[q[i+3]-q[i] for i in range(3)]
            force=[F(stiffness)*delta[i] if mask&(1<<i) else F(0) for i,stiffness in enumerate([120000,30000,800])]
            energy=dot(delta,force)/2;end=mv(k,q);work=dot(q,end);assert work==2*energy
            record('J',id,delta+force+[energy,work]+end)
            for i in range(6):
                for j in range(6):expected['K',id,i,j]=k[i][j]
    record('A',0,[106000,94000,1000,-1000,2000,F(1,10),F(1,5),3000000])
    assert len(expected)==4192,len(expected)
    return expected
