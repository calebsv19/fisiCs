"""Normalize transient AST identities while preserving binding relationships."""
import hashlib,json

def canonical(value):
 local={};external={}
 def declarations(n):
  if isinstance(n,dict):
   for k in ('id','key'):
    if k in n and n[k] not in local:local[n[k]]='local:'+str(len(local))
   for k,v in n.items():
    if k not in ('dtype','cast_type'):declarations(v)
  elif isinstance(n,list):
   for v in n:declarations(v)
 declarations(value)
 def identity(v):
  if v=='0' or v.endswith('#0'):return 'unbound'
  if v in local:return local[v]
  if v not in external:external[v]='external:'+str(len(external))
  return external[v]
 def normalized(n):
  if isinstance(n,dict):return {k:identity(v) if k in ('id','origin','key') else normalized(v) for k,v in n.items() if k not in ('file','line','column') and not(k=='long_double' and v is False)}
  if isinstance(n,list):return [normalized(v) for v in n]
  return n
 return normalized(value)

def digest(value):
 return hashlib.sha256(json.dumps(canonical(value),sort_keys=True,separators=(',',':')).encode()).hexdigest()
