#!/usr/bin/env python3
"""Independent C caller/callee ABI regression for the bounded native HFA repair."""
import argparse,hashlib,json,os,platform,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
def check(compiler,out):
    if platform.machine().lower() not in ('arm64','aarch64'):
        (out/'result.json').write_text(json.dumps(dict(passed=None,status='skipped',reason='AArch64 HFA gate requires native arm64',host=platform.machine())))
        print('aggregate HFA: SKIP (native AArch64 only)');return
    commands=[];env={**os.environ,'FISICS_MAX_PROCS':'0'}
    def run(n,args):
        x=subprocess.run(list(map(str,args)),cwd=ROOT,env=env,capture_output=True,text=True,timeout=120);(out/(n+'.log')).write_text(x.stdout+x.stderr);commands.append(dict(id=n,command=list(map(str,args)),exit=x.returncode));assert x.returncode==0,(n,x.returncode,x.stdout+x.stderr);return x.stdout
    cases=[('d3','double x,y,z;','{2,-3,5}','a.z','5'),('d4','double x,y,z,w;','{2,-3,5,7}','a.w','7'),('f3','float x,y,z;','{2,-3,5}','a.z','5'),('array4','double v[4];','{{2,-3,5,7}}','a.v[3]','7'),('large5','double x,y,z,w,q;','{2,-3,5,7,9}','a.q','9'),('mixed','double x;long y;double z;','{2,3,5}','a.z','5')]
    try:
        for name,fields,initializer,member,value in cases:
            header=f'typedef struct V {{{fields}}} V;\n'
            callee=out/(name+'_callee.c');caller=out/(name+'_caller.c')
            callee.write_text(header+'V copy(V a){return a;}\n')
            caller.write_text(header+f'V copy(V a);int main(void){{V a={initializer};V b=copy(a);return !({member.replace("a.","b.")}=={value});}}\n')
            for tool,exe in [('clang','clang'),('fisics',compiler)]:
                for role,source in [('caller',caller),('callee',callee)]:run(name+'_'+role+'_'+tool,[exe,'-c',source,'-o',out/(name+'_'+role+'_'+tool+'.o')])
            for a,b in [('clang','clang'),('fisics','fisics'),('clang','fisics'),('fisics','clang')]:
                label=name+'_'+a+'_'+b;binary=out/label
                run('link_'+label,['clang',out/(name+'_callee_'+a+'.o'),out/(name+'_caller_'+b+'.o'),'-o',binary]);run('run_'+label,[binary])
        result=dict(passed=True,compiler_sha256=hashlib.sha256(compiler.read_bytes()).hexdigest(),host=platform.machine(),cases=len(cases),commands=commands)
    except BaseException:
        (out/'result.json').write_text(json.dumps(dict(passed=False,commands=commands),indent=2));raise
    (out/'result.json').write_text(json.dumps(result,indent=2)+'\n');print('aggregate ABI PASS',len(cases),'cases',len(commands),'commands')
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',type=Path,required=True);p.add_argument('--out',type=Path);a=p.parse_args()
    if a.out:a.out.mkdir(parents=True,exist_ok=False);check(a.compiler.resolve(),a.out.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix='fisics-hfa-') as t:check(a.compiler.resolve(),Path(t))
if __name__=='__main__':main()
