from build import PROFILES,dump,sha

def qualify(b):
 root=b.work/'abi';root.mkdir();native={}
 for name in ['provider','consumer']:native[name]=b.compile_native(b.source/('abi_'+name+'.c'),root/(name+'-clang.o'))
 results=[]
 for profile in PROFILES:
  objects={}
  for name in ['provider','consumer']:
   if profile=='matched-clang':objects[name]=native[name];continue
   p=b.prepared(b.source/('abi_'+name+'.c'),'D1',profile,root/(name+'-'+profile+'.c'));o=p.with_suffix('.o');flags=[]
   if profile=='fisics-units':flags=['--overlay=physics-units']
   if profile=='fisics-physical':flags=['--overlay=physics-semantics','--physical-semantics-policy',b.contract['policies']['D1']]
   b.run('abi-compile-'+name+'-'+profile,[b.fisics,*flags,'-c',p,'-o',o]);objects[name]=o
  for direction,parts in [('candidate-provider',[objects['provider'],native['consumer']]),('candidate-consumer',[native['provider'],objects['consumer']])]:
   exe=b.link('abi/'+profile+'-'+direction,parts);raw=b.run('abi-run-'+profile+'-'+direction,[exe]);results.append(dict(profile=profile,direction=direction,layout=raw.decode().strip(),exe_sha256=sha(exe)))
 assert len({r['layout'] for r in results})==1
 dump(b.out/'abi.json',dict(passed=True,runs=results,reciprocal=True,binary64=True));return results
