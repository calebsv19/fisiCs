"""Finite resolved-C storage-effect interpreter. No runtime instrumentation."""
from dataclasses import dataclass, replace
from itertools import product
import math,re
from decimal import Decimal,InvalidOperation
from collections import Counter
class Checks:
 def __init__(self):self.total=0;self.sites=Counter()
 def add(self,n,op):
  self.total+=1;self.sites[(n.get('file'),n.get('line'),n.get('column'),op)]+=1
 def __len__(self):return self.total
ZERO=(0,0,0);M=(1,0,0);L=(0,1,0);TIME=(0,0,1);ENERGY=(1,2,-2)
ENERGY_BLOCK_HELPERS=frozenset(f'cs_de_{family}_{block}' for family in ('kinetic','damping') for block in ('tt','tr','rt','rr'))
class Refusal(Exception):pass
from functools import lru_cache

@lru_cache(maxsize=4096)
def literal(s):
 if not s.lower().startswith('0x') and s[-1:] in ('f','F'):raise Refusal('float literal narrowing is outside native proof')
 s=re.sub(r'[uUlL]+$','',s) if s.lower().startswith('0x') else re.sub(r'[uUlLfF]+$','',s)
 if re.fullmatch(r'0[xX][0-9a-fA-F]+',s):return int(s,16)
 if re.fullmatch(r'0[0-7]+',s):return int(s,8)
 if re.fullmatch(r'[0-9]+',s):return int(s,10)
 try:
  number=Decimal(s)
  if not number.is_finite():raise Refusal('nonfinite floating literal')
  # Never turn a nonzero spelling into a structural zero through host float
  # underflow. Long-double and target rounding are not evaluated here.
  return V(c=0 if number.is_zero() else None)
 except InvalidOperation:raise Refusal('unsupported numeric token')

class Env(dict):
 def __init__(self,*args,types=None):super().__init__(*args);self.types=dict(types or {})
def coerce(dtype,value):
 if dtype.get('long_double'):raise Refusal('long-double storage/return conversion outside native proof')
 if dtype.get('pointer') or dtype.get('dims') or isinstance(value,(Array,Record)):return value
 primitive=dtype.get('primitive')
 if dtype.get('name'):
  if dtype['name']=='size_t':primitive='int'
  elif dtype['name']=='bool' and isinstance(value,bool):return value
  else:raise Refusal('unresolved scalar typedef conversion')
 if primitive=='_Bool':
  if isinstance(value,(int,bool)):return bool(value)
  if isinstance(value,Idx):return Idx(tuple(sorted({bool(i) for i in value.values})))
  if isinstance(value,V) and value.dim==ZERO and value.c is not None:return bool(value.c)
  raise Refusal('unresolved C boolean conversion')
 if primitive=='float':raise Refusal('non-double floating conversion is outside native proof')
 if primitive=='double':
  if value is None:return None
  v=val(value)
  # Quantity values are not a floating-point evaluator. Only structural zero is
  # retained; all nonzero numeric values are unknown for dimensional proof.
  return replace(v,c=None) if v.c is not None and v.c!=0 else v
 if dtype.get('narrow') or primitive=='char':raise Refusal('narrow integer conversion is outside finite index proof')
 if primitive=='int' and value is not None:
  if isinstance(value,Idx):
   if any(i<0 or i>4096 for i in value.values):raise Refusal('bounded integer set conversion required')
   return value
  if isinstance(value,V):
   if value.dim!=ZERO or value.c is None:raise Refusal('integer initialization loses quantity/index facts')
   value=int(value.c)
  if not isinstance(value,(int,bool)) or value<0 or value>4096:raise Refusal('nonnegative bounded integer conversion required')
 return value
class Returned(Exception):
 def __init__(self,value):self.value=value
@dataclass(frozen=True)
class V:
 dim:tuple=ZERO
 c:object=None
 deps:frozenset=frozenset()
 coeff:object=None
 rate:object=None
 dead:bool=False
 @property
 def inactive(self):return self.zero or self.dead
 @property
 def zero(self):return self.c==0

def val(x):
 if isinstance(x,V):return x
 if isinstance(x,(int,float,bool)):return V(c=x)
 raise Refusal('non-scalar operand')
def role(i):return int(i%3==2)
def coefficient(i,j):return (1,role(i)+role(j),0)
def indices(x):
 if isinstance(x,Idx):return x.values
 if isinstance(x,int) and not isinstance(x,bool):return (x,)
 raise Refusal('index is not a bounded integer')
@dataclass(frozen=True)
class Idx:
 values:tuple
class Array:
 def __init__(self,dims,name,layout=None,uniform=None,dtype=None):
  self.dims=tuple(dims);self.name=name;self.dtype=dtype;self.index_contract=None;self.active_bound=None;self.data={};self.write_sites={};self.layout=layout;self.uniform=uniform
 def expected(self,ix):return self.layout(ix) if self.layout else None
 def read(self,ix):
  if self.active_bound is not None and any(i<0 or i>=self.active_bound for i in ix):raise Refusal('inactive DOF read '+self.name+str(ix))
  for i,n in zip(ix,self.dims):
   if i<0 or i>=n:raise Refusal('out-of-bounds '+self.name+str(ix))
  if len(ix)<len(self.dims):return Slice(self,ix)
  if ix in self.data:return self.data[ix]
  if self.uniform:return self.uniform(ix)
  raise Refusal('read before definite write '+self.name+str(ix))
 def write(self,ix,value,engine,node):
  engine.last_site=node
  if self.active_bound is not None and any(i<0 or i>=self.active_bound for i in ix):raise Refusal('inactive DOF write '+self.name+str(ix))
  if len(ix)!=len(self.dims) or any(i<0 or i>=n for i,n in zip(ix,self.dims)):raise Refusal('write bounds '+self.name+str(ix))
  if self.dtype:value=coerce(self.dtype,value)
  if self.index_contract and any(not self.index_contract(ix,i) for i in indices(value)):raise Refusal('active DOF map bound/role contradiction '+self.name+str(ix))
  expected=self.expected(ix)
  if expected:
   dim,cr=expected[:2];dof=expected[2] if len(expected)==3 else None;v=val(value)
   if v.zero and v.dim==ZERO:
    # Explicit layout-zero admission, never neighboring scalar inference.
    engine.zeros.add((self.name,ix));v=replace(v,dim=dim,coeff=cr,rate=dof)
   if v.dim!=dim:raise Refusal('dimension contradiction '+self.name+str(ix)+': '+str(v.dim)+' expected '+str(dim))
   if dof is not None and v.rate!=dof:raise Refusal('generalized quantity role contradiction '+self.name+str(ix))
   if cr is not None and v.coeff is not None and cr!=v.coeff:raise Refusal('row/column role contradiction '+self.name+str(ix))
   value=v if v.coeff==cr else replace(v,coeff=cr)
  self.data[ix]=value;self.write_sites[ix]=node;engine.check(node,'write',self.name)
class Slice:
 def __init__(self,array,prefix):self.array=array;self.prefix=prefix
class Record:
 def __init__(self,name,fields):self.name=name;self.fields=fields
 def get(self,k):
  if k not in self.fields:raise Refusal('unadmitted field '+self.name+'.'+k)
  return self.fields[k]
class Engine:
 def __init__(self,functions):
  self.functions=functions;self.arithmetic_cache={};self.arithmetic_reuses=0;self.scalar_cache={};self.scalar_reuses=0;self.matrix_cache={};self.matrix_reuses=0;self.active_function=None;self.last_site=None;self.layout_bindings={};self.momentum_roles=set();self.rows=Checks();self.zeros=set();self.calls=[];self.steps=0;self.depth=0;self.reads=[]
 def check(self,n,op,detail):
  self.rows.add(n,op)
 def tick(self):
  self.steps+=1
  if self.steps>2000000:raise Refusal('analysis resource limit')
 def arithmetic(self,op,a,b,n):
  self.last_site=n
  # Only immutable, effect-free scalar/index operations may be reused. Matrix
  # and generalized-rate contractions must execute their source checks each time.
  pure=lambda v:isinstance(v,(int,bool)) or isinstance(v,V) and v.coeff is None and v.rate is None
  if pure(a) and pure(b):
   key=(op,a,b,n.get('tag')=='unary')
   if key in self.arithmetic_cache:
    self.arithmetic_reuses+=1;return self.arithmetic_cache[key]
   result=self._arithmetic(op,a,b,n)
   if len(self.arithmetic_cache)<4096:self.arithmetic_cache[key]=result
   return result
  return self._arithmetic(op,a,b,n)
 def _arithmetic(self,op,a,b,n):
  self.last_site=n
  if isinstance(a,Idx) or isinstance(b,Idx):
   aa=indices(a);bb=indices(b);out=[]
   for x,y in product(aa,bb):
    z=self.arithmetic(op,x,y,n)
    if not isinstance(z,int) or abs(z)>4096:raise Refusal('bounded index arithmetic exceeded')
    out.append(z)
   return Idx(tuple(sorted(set(out))))
  if isinstance(a,(int,bool)) and isinstance(b,(int,bool)):
   if n.get('tag')!='unary' and any(x<0 for x in (a,b)):raise Refusal('negative operand outside nonnegative C index proof')
   if any(abs(x)>4096 for x in (a,b)):raise Refusal('bounded C integer operand exceeded')
   if op in ['+','-','*','/','%']:
    if op in ['/','%'] and not b:raise Refusal('division by zero')
    result={'+':lambda:a+b,'-':lambda:a-b,'*':lambda:a*b,'/':lambda:int(a/b),'%':lambda:a-int(a/b)*b}[op]()
    if abs(result)>4096 or (result<0 and n.get('tag')!='unary'):raise Refusal('bounded C integer range/underflow exceeded')
    return result
   if op in ['<','<=','>','>=','==','!=','&&','||']:
    return {'<':lambda:a<b,'<=':lambda:a<=b,'>':lambda:a>b,'>=':lambda:a>=b,'==':lambda:a==b,'!=':lambda:a!=b,'&&':lambda:bool(a and b),'||':lambda:bool(a or b)}[op]()
  if a is None or b is None:
   if op in ('==','!=') and a is None and b is None:return op=='=='
   raise Refusal('unsupported pointer arithmetic/comparison')
  a,b=val(a),val(b);deps=(frozenset() if a.inactive else a.deps)|(frozenset() if b.inactive else b.deps)
  if op in ['+','-']:
   if a.zero and a.dim==ZERO:return b if op=='+' else replace(b,c=-b.c if b.c is not None else None)
   if b.zero and b.dim==ZERO:return a
   if a.dim!=b.dim:raise Refusal('addition dimension contradiction '+str((a.dim,b.dim)))
   if a.rate!=b.rate:raise Refusal('addition generalized quantity kind/role contradiction')
   if a.coeff is not None and b.coeff is not None and a.coeff!=b.coeff:raise Refusal('addition row/column role contradiction')
   c=(a.c+b.c if op=='+' else a.c-b.c) if a.c is not None and b.c is not None else None
   return V(a.dim,c,deps,a.coeff or b.coeff,a.rate if a.rate==b.rate else None,a.inactive and b.inactive)
  if op in ['*','/']:
   if op=='/' and b.inactive:raise Refusal('division by zero or annihilated denominator')
   dim=tuple(x+y if op=='*' else x-y for x,y in zip(a.dim,b.dim));c=(a.c*b.c if op=='*' else a.c/b.c) if a.c is not None and b.c is not None else None
   # An annihilated expression supplies no live source roots. It is not a
   # numerical zero fact: unknown floating operands may overflow or be NaN.
   dead=a.inactive or (op=='*' and b.inactive)
   if a.coeff is not None and b.coeff is not None:raise Refusal('unsupported matrix coefficient product/division')
   if op=='/' and (b.coeff is not None or b.rate is not None):raise Refusal('unsupported inverse generalized quantity')
   coeff=a.coeff if b.rate is None else b.coeff if a.rate is None else None
   rate=a.rate if b.rate is None else b.rate if a.rate is None else None
   if op=='*':
    if a.coeff is not None and b.rate is not None:
     if a.coeff[1]!=b.rate:raise Refusal('matrix column/consumer DOF role contradiction')
     cr=a.coeff;rate=cr[0];coeff=None
     if a.dim==(1,cr[0]+cr[1],0) and b.dim==(0,1-cr[1],-1) and not dead:self.momentum_roles.add(cr);self.check(n,'momentum-connection','matrix-vector')
    elif b.coeff is not None and a.rate is not None:
     if b.coeff[0]!=a.rate:raise Refusal('matrix row/consumer DOF role contradiction')
     cr=b.coeff;rate=cr[1];coeff=None
     if b.dim==(1,cr[0]+cr[1],0) and a.dim==(0,1-cr[0],-1) and not dead:self.momentum_roles.add(cr);self.check(n,'momentum-connection','vector-matrix')
    elif a.rate is not None and b.rate is not None:
     if a.rate!=b.rate:raise Refusal('generalized scalar product role contradiction')
     rate=None
   if c is not None and not math.isfinite(c):raise Refusal('nonfinite constant arithmetic')
   return V(dim,c,frozenset() if dead else deps,coeff,rate,dead)
  if op in ['==','!=','<','>','<=','>=']:
   if a.dim!=b.dim:raise Refusal('comparison dimension contradiction')
   if a.c is None or b.c is None:raise Refusal('unknown branch condition')
   return self.arithmetic(op,a.c,b.c,n)
  raise Refusal('unsupported binary '+str(op))
 def expr(self,n,env):
  self.last_site=n;self.tick()
  if n is None:return None
  if n.get('dtype',{}).get('long_double'):raise Refusal('long-double expression outside native proof')
  if n.get('dtype',{}).get('volatile'):raise Refusal('volatile storage is outside stable flow')
  if n.get('concrete_unit'):raise Refusal('concrete unit scale is outside admitted SI flow')
  t=n['tag']
  if t=='id':
   key=n['origin']
   if key not in env:raise Refusal('unbound C identity '+n['name']+' '+key)
   return env[key]
  if t=='number':
   return literal(n['value'])
  if t=='field':
   base=self.expr(n['base'],env)
   if not isinstance(base,Record):raise Refusal('unresolved record access')
   value=base.get(n['field'])
   if isinstance(value,(V,Idx,int,float,bool)) and n.get('dtype',{}).get('kind')==-1:raise Refusal('unresolved scalar field type')
   return coerce(n.get('dtype',{}),value)
  if t=='index':
   base=self.expr(n['base'],env);ix=self.expr(n['index'],env);choices=[]
   # Singleton storage reads have no join; keep all normal bounds checks.
   options=indices(ix)
   if len(options)==1:
    if isinstance(base,Slice):return base.array.read((*base.prefix,options[0]))
    if isinstance(base,Array):return base.read((options[0],))
   for i in options:
    if isinstance(base,MultiSlice):
     for s in base.slices:choices.append(s.array.read((*s.prefix,i)))
     continue
    if isinstance(base,Slice):v=base.array.read((*base.prefix,i))
    elif isinstance(base,Array):v=base.read((i,))
    else:raise Refusal('unresolved storage access')
    choices.append(v)
   if all(isinstance(v,Slice) for v in choices):
    if len(choices)!=1: return MultiSlice(choices)
    return choices[0]
   return self.merge_choices(choices,n)
  if t=='cast':
   if n['cast_type']['pointer'] and n['value']['tag']=='number' and n['value']['value']=='0':return None
   if n['cast_type']['pointer']:raise Refusal('pointer cast loses identity')
   v=self.expr(n['value'],env)
   if n['cast_type']['primitive']!='int' or any(i<0 or i>4096 for i in indices(v)):raise Refusal('cast does not preserve bounded integer index')
   return v
  if t=='binary':
   left=self.expr(n['left'],env)
   if n['op'] in ('&&','||'):
    if isinstance(left,Idx):
     choices={bool(i) for i in left.values}
     if len(choices)>1:
      def pure(x):
       if isinstance(x,dict):return x.get('tag') not in ('assign','call','unsupported') and not(x.get('tag')=='unary' and x.get('op') in ('++','--')) and all(pure(v) for k,v in x.items() if k not in ('dtype','cast_type'))
       if isinstance(x,list):return all(pure(v) for v in x)
       return True
      if not pure(n['right']):raise Refusal('uncertain short-circuit effect')
      return self.arithmetic(n['op'],left,self.expr(n['right'],env),n)
     left=next(iter(choices))
    truth=self.truth(left)
    if n['op']=='&&' and not truth:return False
    if n['op']=='||' and truth:return True
    return self.truth(self.expr(n['right'],env))
   return self.arithmetic(n['op'],left,self.expr(n['right'],env),n)
  if t=='ternary':return self.expr(n['yes'] if self.truth(self.expr(n['condition'],env)) else n['no'],env)
  if t=='unary':
   op=n['op']
   if op=='&':return self.expr(n['left'],env)
   if op in ['++','--']:
    old=self.expr(n['left'],env);self.assign(n['left'],self.arithmetic('+' if op=='++' else '-',old,1,n),env,n);return old if n['postfix'] else self.expr(n['left'],env)
   v=self.expr(n['left'],env)
   if op=='+':return v
   if op=='-':return self.arithmetic('*',-1,v,n)
   if op=='!':return not self.truth(v)
   raise Refusal('unsupported unary '+str(op))
  if t=='assign':
   rhs=self.expr(n['value'],env)
   if n['op']!='=':rhs=self.arithmetic(n['op'][:-1],self.expr(n['target'],env),rhs,n)
   self.assign(n['target'],rhs,env,n);return rhs
  if t=='call':
   if n['callee']['tag']!='id' or not n['callee'].get('direct_function'):raise Refusal('indirect or unresolved call')
   name=n['callee']['name'];args=[self.expr(a,env) for a in n['args']];self.last_site=n
   if name in ['hypot','sqrt','fabs','fmax']:
    if name in self.functions:raise Refusal('shadowed math implementation is not an intrinsic')
    if name=='fabs':
     v=val(args[0]);return replace(v,c=abs(v.c) if v.c is not None else None)
    if name=='hypot':
     if val(args[0]).dim!=val(args[1]).dim or val(args[0]).rate!=val(args[1]).rate or val(args[0]).coeff!=val(args[1]).coeff:raise Refusal('hypot dimension/kind/role contradiction')
     return V(val(args[0]).dim,deps=frozenset().union(*(v.deps for v in map(val,args) if not v.inactive)),rate=val(args[0]).rate,coeff=val(args[0]).coeff,dead=all(val(v).inactive for v in args))
    if name=='sqrt':
     v=val(args[0]);
     if any(x%2 for x in v.dim):raise Refusal('sqrt dimensions')
     return V(tuple(x//2 for x in v.dim),deps=frozenset() if v.inactive else v.deps,dead=v.inactive)
    if val(args[0]).dim!=val(args[1]).dim or val(args[0]).rate!=val(args[1]).rate or val(args[0]).coeff!=val(args[1]).coeff:raise Refusal('max dimension/kind/role contradiction')
    a,b=val(args[0]),val(args[1]);return V(a.dim,max(a.c,b.c) if a.c is not None and b.c is not None else None,(frozenset() if a.inactive else a.deps)|(frozenset() if b.inactive else b.deps),coeff=a.coeff,rate=a.rate,dead=a.inactive and b.inactive)
   if name=='cs_pf_rotation':
    # A separately verified source-derived map summary, never a name-only cast.
    if not hasattr(self,'rotation_summary'):raise Refusal('rotation effect summary absent')
    if len(args)!=3 or not isinstance(args[0],Record) or args[0].name!='model' or any(i<0 or i>=args[0].get('members') for i in indices(args[1])) or any(i not in (0,1) for i in indices(args[2])):raise Refusal('rotation call model/member/end binding')
    model=args[0];ends=indices(args[2]);values=tuple(sorted(set(i for end in ends for i in self.rotation_summaries[(model.get('nodes'),model.get('joints'),end)].values)))
    return Idx(values)
   return self.call(name,args)
  if t=='sizeof':
   v=self.expr(n['value'],env)
   if not isinstance(v,Array):raise Refusal('sizeof unresolved array')
   return math.prod(v.dims)*8
  raise Refusal('unsupported expression '+t)
 def merge_choices(self,choices,n):
  if not choices:raise Refusal('empty index set')
  first=choices[0]
  if all(v is first for v in choices):return first
  if all(isinstance(v,(int,Idx)) for v in choices):
   values=tuple(sorted(set(i for v in choices for i in indices(v))));return values[0] if len(values)==1 else Idx(values)
  if all(isinstance(v,int) for v in choices):return first if len(set(choices))==1 else Idx(tuple(sorted(set(choices))))
  if all(isinstance(v,Record) for v in choices):
   # Inputs must be the same admitted record family, fields conservative unions.
   keys=set(first.fields)
   if any(set(v.fields)!=keys for v in choices):raise Refusal('heterogeneous records')
   return Record(first.name,{k:self.merge_choices([v.fields[k] for v in choices],n) if not isinstance(first.fields[k],Array) else first.fields[k] for k in keys})
  vv=[val(v) for v in choices]
  if any(v.dim!=vv[0].dim or v.coeff!=vv[0].coeff or v.rate!=vv[0].rate for v in vv):raise Refusal('dynamic index merges incompatible physical roles')
  return V(vv[0].dim,vv[0].c if all(v.c==vv[0].c for v in vv) else None,frozenset().union(*(v.deps for v in vv if not v.inactive)),vv[0].coeff,vv[0].rate,all(v.inactive for v in vv))
 def truth(self,v):
  if isinstance(v,(bool,int)):return bool(v)
  if isinstance(v,V) and v.c is not None and v.dim==ZERO:return bool(v.c)
  raise Refusal('unresolved branch')
 def assign(self,n,v,env,site):
  self.last_site=site
  if n['tag']=='id':
   if n['origin'] not in env:raise Refusal('unbound assignment identity')
   env[n['origin']]=coerce(env.types.get(n['origin'],{}),v);self.check(site,'assignment',n['name']);return
  if n['tag']=='field':
   r=self.expr(n['base'],env)
   if not isinstance(r,Record) or n['field'] not in r.fields:raise Refusal('untracked field write')
   v=coerce(n.get('dtype',{}),v)
   old=r.fields[n['field']]
   if isinstance(old,V) and (val(v).dim!=old.dim or val(v).rate!=old.rate):raise Refusal('field dimension/kind/role contradiction')
   r.fields[n['field']]=v;self.check(site,'field-write',r.name+'.'+n['field']);return
  if n['tag']=='index':
   base=self.expr(n['base'],env);ii=indices(self.expr(n['index'],env));targets=[]
   if isinstance(base,Slice):targets=[(base.array,(*base.prefix,i)) for i in ii]
   elif isinstance(base,Array):targets=[(base,(i,)) for i in ii]
   elif isinstance(base,MultiSlice):targets=[(s.array,(*s.prefix,i)) for s in base.slices for i in ii]
   else:raise Refusal('unresolved write target')
   targets=list({(id(a),ix):(a,ix) for a,ix in targets}.values())
   uncertain=len(targets)>1
   for a,ix in targets:
    initialized=ix in a.data or a.uniform is not None
    old=a.read(ix) if uncertain and initialized else None
    a.write(ix,v,self,site)
    if uncertain:
     if initialized:
      new=a.data[ix]
      if isinstance(old,V) and isinstance(new,V) and (old.dim,old.coeff,old.rate)==(new.dim,new.coeff,new.rate):a.data[ix]=V(old.dim,old.c if old.c==new.c else None,old.deps|new.deps,old.coeff,old.rate,old.inactive and new.inactive)
      else:a.data[ix]=self.merge_choices([old,new],site)
     else:
      del a.data[ix];a.write_sites.pop(ix,None)
   return
  raise Refusal('unsupported assignment target')
 def initialize(self,a,n,env,prefix=()):
  if n['tag']!='aggregate':raise Refusal('array initializer requires aggregate')
  entries=n['entries'];size=a.dims[len(prefix)]
  if len(entries)>size:raise Refusal('initializer bounds')
  for i in range(size):
   e=entries[i] if i<len(entries) else None
   if e and (e['field'] or e['index']):raise Refusal('array designated initialization unsupported')
   ix=(*prefix,i)
   if len(ix)<len(a.dims):
    if e:self.initialize(a,e['value'],env,ix)
    else:
     for suffix in product(*(range(x) for x in a.dims[len(ix):])):a.write((*ix,*suffix),0,self,n)
   else:a.write(ix,self.expr(e['value'],env) if e else 0,self,n)
 def stmt(self,n,env):
  self.last_site=n;self.tick()
  if n is None:return
  t=n['tag']
  if t=='block':
   for x in n['body']:self.stmt(x,env)
  elif t=='decl':
   for v in n['variables']:
    env.types[v['key']]=v['dtype']
    if v.get('concrete_unit') or (v.get('dimension') is not None and any(v['dimension'])):raise Refusal('new local dimensional/unit restoration outside bounded source path')
    if v['dtype'].get('static') or v['dtype'].get('extern') or v['dtype'].get('volatile'):raise Refusal('nonautomatic or volatile local storage')
    dims=v['dtype']['dims']
    if dims and all(isinstance(x,int) and x>0 for x in dims) and not v['dtype']['pointer']:
     if math.prod(dims)>729:raise Refusal('storage capacity limit')
     layout_kind=self.layout_bindings.get(v['key'])
     layout=(lambda ix:((0,role(ix[1])-role(ix[0]),0),None)) if layout_kind=='basis' else (lambda ix:(coefficient(ix[0]//6,ix[0]%6),None)) if layout_kind=='local-mass' else None
     a=Array(dims,v['name'],layout=layout,dtype=dict(v['dtype'],dims=[],pointer=0))
     if getattr(self,'active_dofs',None) and dims==[6] and v['dtype']['name']=='size_t':a.index_contract=lambda ix,i:0<=i<self.active_dofs and role(i)==role(ix[0])
     env[v['key']]=a
     if v['init']:self.initialize(a,v['init'],env)
    elif v['init'] and v['init']['tag']=='aggregate':
     if v['dtype']['name']!='CsFrameElement':raise Refusal('unadmitted local record layout/conversions')
     fields={}
     for entry in v['init']['entries']:
      if not entry['field']:raise Refusal('record initializer mapping')
      fields[entry['field']]=self.expr(entry['value'],env)
     env[v['key']]=Record(v['name'],fields)
    else:env[v['key']]=coerce(v['dtype'],self.expr(v['init'],env)) if v['init'] else None
  elif t=='for':
   self.stmt(n['init'],env);count=0
   while self.truth(self.expr(n['condition'],env)):
    if count>=729:raise Refusal('loop bound exceeded')
    count+=1;self.stmt(n['body'],env);self.expr(n['increment'],env)
  elif t=='if':self.stmt(n['yes'] if self.truth(self.expr(n['condition'],env)) else n['no'],env)
  elif t=='return':raise Returned(self.expr(n['value'],env))
  elif t in ['assign','call','unary']:self.expr(n,env)
  else:raise Refusal('unsupported statement '+t)
 def call(self,name,args):
  if self.depth>16:raise Refusal('call recursion limit')
  if name not in self.functions:raise Refusal('unknown producer/call '+name)
  f=self.functions[name];self.last_site=f;params=[v for p in f['parameters'] for v in p.get('variables',[])]
  if len(args)!=len(params):raise Refusal('call signature arity')
  args=[coerce(p['dtype'],a) for p,a in zip(params,args)]
  cacheable=all(isinstance(a,(V,int,float,bool)) and not p['dtype']['pointer'] and not p['dtype']['dims'] and not p['dtype']['name'] for p,a in zip(params,args))
  cachekey=(name,tuple(args)) if cacheable else None
  if cachekey is not None and cachekey in self.scalar_cache:
   self.scalar_reuses+=1;return self.scalar_cache[cachekey]
  for p,a in zip(params,args):
   if p.get('concrete_unit'):raise Refusal('concrete parameter unit scale outside SI admission')
   if p.get('dimension') is not None and val(a).dim!=tuple(p['dimension']):raise Refusal('declared callee parameter dimension contradiction: '+name+'.'+p['name'])
  if name in ('cs_de_kinetic','cs_de_damping','cs_de_work'):
   if not f['file'].endswith('/core_structural_dynamic_energy.h'):raise Refusal('raw DOF interface source identity absent')
   row=role(args[0]);col=role(args[1]) if name!='cs_de_work' else None
   if name=='cs_de_work':
    if any(val(a).rate!=row for a in args[2:]):raise Refusal('raw work argument quantity kind/DOF contradiction')
   else:
    damping=name=='cs_de_damping';slots=[(4,row),(5,row),(7,col),(8,col)] if damping else [(2,row),(4,col)]
    if val(args[6 if damping else 3]).coeff!=(row,col) or any(val(args[k]).rate!=expected for k,expected in slots):raise Refusal('raw kinetic/damping argument quantity kind/DOF contradiction')
    if damping and any(val(args[k]).rate is not None for k in (2,3)):raise Refusal('raw duration/decay argument kind contradiction')
  # Registered existing helper interfaces carry authored DOF kinds; the body is
  # still checked below and an unavailable implementation never passes.
  if name in ENERGY_BLOCK_HELPERS:
   if not f['file'].endswith('/core_structural_dynamic_energy.h') or f.get('return_dim')!=list(ENERGY):raise Refusal('registered energy interface metadata absent')
   block=name.rsplit('_',1)[1];row=int(block[0]=='r');col=int(block[1]=='r');damping='_damping_' in name
   slots=[(2,row),(3,row),(5,col),(6,col)] if damping else [(0,row),(2,col)]
   matrix=args[4 if damping else 1]
   if val(matrix).coeff!=(row,col) or any(val(args[i]).rate!=role for i,role in slots):raise Refusal('existing kinetic/damping quantity kind/DOF contract contradiction')
   if damping and any(val(args[i]).rate is not None for i in (0,1)):raise Refusal('duration/decay cannot be a generalized rate')
  if name in ('cs_de_translation_work','cs_de_rotation_work','cs_physical_translation_work','cs_physical_rotation_work'):
   if f.get('return_dim')!=list(ENERGY) or not f['file'].endswith(('/core_structural_dynamic_energy.h','/core_structural_frame_physical.h')):raise Refusal('registered work interface metadata absent')
   expected=int('rotation' in name)
   if any(val(a).rate!=expected for a in args):raise Refusal('existing work force/displacement quantity kind contract contradiction')
  if name in ('cd_consistent_mass','cd_transform'):
   if f.get('return_type',{}).get('primitive')!='void' or f['return_type'].get('pointer'):raise Refusal('matrix procedure return type changed')
   pp=[p['dtype'] for p in params]
   if len(pp)!=3 or pp[0]['name']!='CsFrameElement' or pp[0]['pointer']!=1 or not pp[0]['const'] or pp[2]['primitive']!='double' or pp[2]['pointer']!=1 or pp[2]['const'] or pp[2]['volatile']:raise Refusal('matrix producer/transform pointer interface changed')
   if name=='cd_consistent_mass' and (pp[1]['primitive']!='double' or pp[1]['pointer']):raise Refusal('matrix mass scalar interface changed')
   if name=='cd_transform' and (pp[1]['primitive']!='double' or pp[1]['pointer']!=1 or not pp[1]['const']):raise Refusal('transform source interface changed')
  if name in ('cd_consistent_mass','cd_transform') and name not in self.layout_bindings:
   def declarations(node):
    if isinstance(node,dict):
     if node.get('tag')=='decl':yield from node['variables']
     for k,v in node.items():
      if k not in ('dtype','cast_type'):yield from declarations(v)
    elif isinstance(node,list):
     for value in node:yield from declarations(value)
   dims=[36] if name=='cd_consistent_mass' else [6,6]
   matches=[v for v in declarations(f['body']) if v['dtype']['dims']==dims and v['dtype']['primitive']=='double' and not v['dtype']['pointer']]
   if len(matches)!=1:raise Refusal('unique admitted local mass/basis layout absent')
   self.layout_bindings[matches[0]['key']]='local-mass' if name=='cd_consistent_mass' else 'basis';self.layout_bindings[name]=True
  if name=='cd_consistent_mass':
   if not isinstance(args[2],Array) or args[2].dims!=(36,):raise Refusal('producer output extent/type')
   if args[2].layout is None:args[2].layout=lambda ix:(coefficient(ix[0]//6,ix[0]%6),(role(ix[0]//6),role(ix[0]%6)))
  matrixkey=None
  if name=='cd_consistent_mass' and len(args)==3 and isinstance(args[0],Record) and set(args[0].fields)=={'length','cosine','sine'} and all(isinstance(x,V) for x in args[0].fields.values()) and isinstance(args[1],V) and isinstance(args[2],Array) and args[2].dims==(36,) and not args[2].data:
   # A checked write-effect summary, keyed by all abstract input facts and the
   # exact output layout. No initialized/aliased output or opaque producer trust.
   matrixkey=(tuple(sorted(args[0].fields.items())),args[1],tuple(args[2].expected((i,)) for i in range(36)))
   if matrixkey in self.matrix_cache:
    values,sites=self.matrix_cache[matrixkey]
    for ix,value in values:args[2].write(ix,value,self,sites[ix])
    self.matrix_reuses+=1;return None
  env=Env({p['key']:a for p,a in zip(params,args)},types={p['key']:p['dtype'] for p in params});self.calls.append(name);self.depth+=1
  try:self.stmt(f['body'],env)
  except Returned as r:
   if 'return_type' not in f:raise Refusal('resolved function return type absent')
   converted=coerce(f['return_type'],r.value)
   r.value=converted
   if (name.startswith(('cs_de_kinetic_','cs_de_damping_')) or name in ('cs_de_kinetic','cs_de_damping','cs_de_work','cs_de_translation_work','cs_de_rotation_work','cs_physical_translation_work','cs_physical_rotation_work')) and (val(r.value).dim!=ENERGY or val(r.value).rate is not None):raise Refusal('registered helper body result not a scalar energy')
   if f.get('return_dim') is not None and val(r.value).dim!=tuple(f['return_dim']):raise Refusal('declared return dimension contradiction: '+name)
   if cachekey is not None:self.scalar_cache[cachekey]=r.value
   return r.value
  finally:self.depth-=1
  if matrixkey is not None:
   if set(args[2].data)!={(i,) for i in range(36)}:raise Refusal('matrix producer summary lacks complete output')
   if tuple(sorted(args[0].fields.items()))!=matrixkey[0]:raise Refusal('matrix producer mutated an input port')
   self.matrix_cache[matrixkey]=(tuple(args[2].data.items()),dict(args[2].write_sites))
  return None
class MultiSlice:
 def __init__(self,slices):self.slices=slices
