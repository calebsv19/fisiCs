#!/usr/bin/env python3
"""Bounded P6.2 storage, helper identity, strict and receipt acceptance gate."""
import argparse,copy,hashlib,json,os,re,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
FIX=ROOT/'tests/fixtures/physical_semantics/representations'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def check(compiler,out,probe=None):
    commands=[];reports={};result=dict(passed=False,compiler_sha256=sha(compiler),commands=commands)
    env={**os.environ,'FISICS_MAX_PROCS':'0','PYTHONDONTWRITEBYTECODE':'1'};env.pop('DISABLE_CODEGEN',None);env.pop('FISICS_OVERLAY',None)
    def write(n,v):p=out/n;p.write_text(json.dumps(v,indent=2)+'\n' if not isinstance(v,str) else v);return p
    def run(n,args,code=0):
        p=subprocess.run(list(map(str,args)),cwd=ROOT,env=env,capture_output=True,text=True,timeout=90);log=p.stdout+p.stderr;write(n+'.log',log);commands.append(dict(id=n,command=list(map(str,args)),exit=p.returncode,expected=code));assert p.returncode==code,(n,p.returncode,log[-4500:]);assert code or 'Error:' not in log,(n,log[-4500:]);return log
    model=json.loads((FIX/'policy.json').read_text());catalog=json.loads((FIX/'catalog.json').read_text());write('catalog.json',catalog);policy=write('policy.json',model)
    def rep(prec='float',ext=3,rank=1,storage='record',parity='polar'):return f'rep.{storage}.{prec}.r{rank}.n{ext}.{parity if rank==1 else "tensor"}'
    def attr(k='force',prec='float',ext=3,rank=1,storage='record',parity='polar'):
        dim={'force':'force','displacement':'length','torque':'energy','energy':'energy','ratio':'1'}[k]
        return f'FISICS_DIM({dim}) FISICS_QUANTITY_KIND("quantity.{k}") FISICS_REPRESENTATION("{rep(prec,ext,rank,storage,parity)}")'
    def scalar(k):return 'FISICS_DIM('+dict(force='force',displacement='length',torque='energy',energy='energy',ratio='1')[k]+') FISICS_QUANTITY_KIND("quantity.'+k+'")'
    def source(body):return '#include <fisics/extensions.h>\n'+body+'\n'
    def report(n,body,code=0,permissions=(),pol=policy,fn='check'):
        src=write(n+'.c',source(body));dest=out/(n+'.json')
        run(n,[compiler,'-I'+str(ROOT/'include'),'--overlay=physics-semantics','--physical-semantics-policy',pol,'--physical-semantics-function',fn,'--physical-semantics-strict',*['--physical-semantics-allow-'+p for p in permissions],'--emit-physical-semantics-json',dest,src],code)
        if dest.exists():
            data=json.loads(dest.read_text());reports[n]=data;s=data['selection'];assert s['accepted']==(code==0);assert s['total']==s['required']+s['ordinary_controls']==len(data['obligations']);assert s['required']==s['checked']+s['permitted_trust']+s['blocked'];assert data['schema_version']==model['schema_version']
        return dest
    try:
        for prec in ['float','double']:
            for ext in [2,3]:
                a=attr(prec=prec,ext=ext);record='typedef struct V {'+prec+' '+','.join('xyz'[:ext])+';} V;\n'
                report(f'copy_{prec}_{ext}',record+f'{a} V check(const V a {a}){{V b {a}=a;b=a;return b;}}')
                report(f'component_{prec}_{ext}',record+f'{scalar("force")} {prec} check(V a {a}){{{prec} b {scalar("force")}=a.x;return b;}}')
                report(f'write_{prec}_{ext}',record+f'{scalar("force")} {prec} check(V a {a},{prec} f {scalar("force")}){{a.x=f;return a.x;}}')
                report(f'array_{prec}_{ext}',f'{scalar("force")} {prec} check({prec} f {scalar("force")}){{{prec} a[{ext}] {attr(prec=prec,ext=ext,storage="array")}={{'+','.join(['f']*ext)+'};a[0]=f;return a[0];}')
                for rankext in [2,3]:
                    ra=attr('ratio',prec,rankext,2);rr=f'typedef struct M {{{prec} axes[{rankext}][{rankext}];}} M;\n'
                    report(f'tensor_{prec}_{rankext}_{ext}',rr+f'{ra} M check(M a {ra}){{M b {ra}=a;return b;}}')
        a=attr();f=scalar('force');v='typedef struct V {float x,y,z;} V;\n';both=['literal-assumptions','declared-boundaries']
        report('initializer',v+f'{f} float check(float f {f}){{V a {a}={{f,f,f}};return a.x;}}')
        report('literal_initializer',v+f'{f} float check(V b {a}){{V a {a}={{1,2,3}};return b.x;}}',permissions=['literal-assumptions'])
        for n,record in [('order','float y,x,z;'),('precision','double x,y,z;'),('extra','float x,y,z,w;'),('integer','int x,y,z;'),('nested','float x[1],y,z;')]:report(n,'typedef struct V {'+record+'} V;'+f'{a} V check(V a {a}){{return a;}}',1,permissions=both)
        report('aligned_field','typedef struct V { _Alignas(16) float x;float y,z;} V;'+f'{a} V check(V a {a}){{return a;}}',1,permissions=both)
        report('packed_record','typedef struct __attribute__((packed)) V {float x,y,z;} V;'+f'{a} V check(V a {a}){{return a;}}',1,permissions=both)
        report('union','typedef union V {float x,y,z;} V;'+f'{a} V check(V a {a}){{return a;}}',1,permissions=both)
        report('dof',f'{f} float check(float f {f}){{float a[4] {attr(storage="array")}={{f,f,f,f}};return a[0];}}',1,permissions=both)
        report('wrong_tensor',f'typedef struct M {{float axes[2][3];}} M;{attr("ratio",rank=2)} M check(M a {attr("ratio",rank=2)}){{return a;}}',1,permissions=both)
        report('array_param',f'{f} float check(float a[3] {attr(storage="array")}){{return a[0];}}',1,permissions=both)
        report('pointer',v+f'{a} V check(V *a){{return *a;}}',1,permissions=both)
        report('raw',v+f'{a} V check(void *p){{return *(V*)p;}}',1,permissions=both)
        report('no_representation',v+f'{a} V check(V x){{V a {a}=x;return a;}}',1,permissions=both)
        report('wrong_kind',v+f'{a} V check(V x {attr("displacement")}){{return x;}}',1,permissions=both)
        report('wrong_parity',v+f'{attr("torque",parity="axial")} V check(V x {attr("torque")}){{return x;}}',1,permissions=both)
        report('scalar_vector',v+f'{f} float check(V x {a}){{return x.x+x.y;}}')
        base=reports['copy_float_3']['selection']['total']
        report('removed',v+f'V check(V a){{V b=a;b=a;return b;}}',1,permissions=both)
        assert reports['removed']['selection']['total']==base
        helper_sources={}
        for op in ['add','subtract','scale','dot','cross']:
            ks={'add':['force']*3,'subtract':['force']*3,'scale':['force','ratio','force'],'dot':['force','displacement','energy'],'cross':['displacement','force','torque']}[op]
            ats=[scalar(k) if (op=='scale' and i==1) or (op=='dot' and i==2) else attr(k,parity='axial' if op=='cross' and i==2 else 'polar') for i,k in enumerate(ks)]
            ts=['float' if (op=='scale' and i==1) or (op=='dot' and i==2) else 'V' for i in range(3)]
            decl=f'{ats[2]} FISICS_PHYSICAL_OPERATION("helper.{op}.float.n3") {ts[2]} h({ts[0]} a {ats[0]},{ts[1]} b {ats[1]});\n'
            # Checked scalar component work prevents trust-only scope admission.
            body=v+decl+f'{ats[2]} {ts[2]} check({ts[0]} a {ats[0]},{ts[1]} b {ats[1]}){{float clean {scalar(ks[0])}=a.x;return h(a,b);}}'
            helper_sources[op]=body
            for precision,extent in [('float',2),('double',2),('double',3)]:
                if op=='cross' and extent==2:continue
                variant=body.replace('float',precision).replace('n3','n'+str(extent))
                if extent==2:variant=variant.replace('x,y,z','x,y')
                report(f'helper_{op}_{precision}_{extent}',variant,permissions=['declared-boundaries'])
            report(op+'_trust_required',body,1)
            report(op+'_permitted',body,permissions=['declared-boundaries'])
            assert reports[op+'_permitted']['selection']['permitted_trust']>0
            assert any('frames unassessed' in (e['reason'] or '') for e in reports[op+'_permitted']['expressions'])
        report('reversed_cross',helper_sources['cross'].replace('return h(a,b)','return h(b,a)'),1,permissions=both)
        report('unbound_helper',helper_sources['dot'].replace('FISICS_PHYSICAL_OPERATION("helper.dot.float.n3")',''),1,permissions=both) # declared result permission absent
        report('wrong_signature',helper_sources['dot'].replace('helper.dot.float.n3','helper.cross.float.n3'),1,permissions=both)
        report('shadowed_helper',helper_sources['dot'].replace('return h(a,b);','{float h=0;return h;}'),1,permissions=both)
        # Binding validation is independent of the selected scope and callee spelling.
        report('renamed_helper',helper_sources['dot'].replace(' h(',' other(').replace('return h(','return other('),permissions=['declared-boundaries'])
        report('unknown_operation',helper_sources['dot'].replace('helper.dot.float.n3','helper.unknown'),1,permissions=both)
        report('duplicate_operation',helper_sources['dot'].replace('FISICS_PHYSICAL_OPERATION("helper.dot.float.n3")','FISICS_PHYSICAL_OPERATION("helper.dot.float.n3") FISICS_PHYSICAL_OPERATION("helper.dot.float.n3")'),1,permissions=both)
        report('operation_variable',v+f'{f} float check(float x {f}){{float bad FISICS_PHYSICAL_OPERATION("helper.dot.float.n3")=0;return x;}}',1,permissions=both)
        report('component_kind','typedef struct V {float x '+scalar('displacement')+';float y,z;} V;'+f'{a} V check(V a {a}){{return a;}}',1,permissions=both)
        report('component_dimension','typedef struct V {float x FISICS_DIM(length);float y,z;} V;'+f'{a} V check(V a {a}){{return a;}}',1,permissions=both)
        report('tensor_component','typedef struct M {double axes[3][3];} M;'+scalar('ratio')+' double check(M a '+attr('ratio','double',3,2)+'){return a.axes[1][2];}')
        report('vla',f'{f} float check(int n,float f {f}){{float a[n] {attr(storage="array")};a[0]=f;return a[0];}}',1,permissions=both)
        report('duplicate_representation',v+f'{a} FISICS_REPRESENTATION("rep.record.float.r1.n3.polar") V check(V a {a}){{return a;}}',1,permissions=both)
        report('conflicting_helper_redeclaration',helper_sources['add'].replace('\n'+a+' V check', '\n'+a+' FISICS_PHYSICAL_OPERATION("helper.subtract.float.n3") V h(V a '+a+',V b '+a+');\n'+a+' V check'),1,permissions=both)
        # Cross-TU representation signatures, reverse order and tamper/freshness.
        x=report('tu_a',v+f'{a} V shared(V a {a});{a} V check(V a {a}){{V b {a}=a;return b;}}')
        y=report('tu_b',v+f'{a} V shared(V a {a});{a} V check_b(V a {a}){{return a;}}',fn='check_b')
        run('cross_tu',[compiler,'--physical-semantics-check',x,y]);run('cross_tu_reverse',[compiler,'--physical-semantics-check',y,x])
        for n,mut in [('tamper_rep',lambda d:d['declarations'][0]['slots'][0]['fact'].update(representation='rep.unknown')),('tamper_model',lambda d:d['representation_model'].update(fingerprint='0'*16))]:
            d=copy.deepcopy(reports['tu_a']);mut(d);bad=write(n+'.json',d);run(n,[compiler,'--physical-semantics-check',bad,y],1)
        # Same dimensions/shape cannot hide a different physical helper or C ABI.
        shared_add=helper_sources['add'].replace('check(', 'check_a(')
        ha=report('helper_tu_a',shared_add,permissions=['declared-boundaries'],fn='check_a')
        hb=report('helper_tu_b',helper_sources['add'].replace('check(', 'check_b(').replace('helper.add.float.n3','helper.subtract.float.n3'),permissions=['declared-boundaries'],fn='check_b')
        run('cross_helper_conflict',[compiler,'--physical-semantics-check',ha,hb],1)
        (out/'tu_a.c').write_text((out/'tu_a.c').read_text()+'\n/* stale */\n')
        run('stale_source',[compiler,'--physical-semantics-check',x,y],1)
        # Canonical input order does not affect the model, while descriptor changes do.
        reordered=copy.deepcopy(model);reordered['representations'].reverse();reordered['shape_helpers'].reverse();rp=write('reordered_policy.json',reordered)
        report('reordered_model',v+f'{a} V check(V a {a}){{return a;}}',pol=rp)
        assert reports['reordered_model']['representation_model']==reports['tu_b']['representation_model']
        changed=copy.deepcopy(model);changed['representations'][1]['scalar']='double' if changed['representations'][1]['scalar']=='float' else 'float';cp=write('changed_policy.json',changed)
        report('changed_model',v+f'{a} V check(V a {a}){{return a;}}',pol=cp)
        assert reports['changed_model']['representation_model']['fingerprint']!=reports['tu_b']['representation_model']['fingerprint']
        badpolicies=[]
        for n,mut in [('duplicate',lambda p:p['representations'].append(copy.deepcopy(p['representations'][0]))),('rank',lambda p:p['representations'][0].update(rank=3)),('extent',lambda p:p['representations'][0].update(extent=4)),('float_type',lambda p:p['representations'][0].update(scalar='long_double')),('fields',lambda p:p['representations'][0].update(fields=['y','x','x'])),('helper_kind',lambda p:p['shape_helpers'][0].update(result_kind='quantity.energy')),('helper_precondition',lambda p:p['shape_helpers'][0].update(preconditions='assumed')),('unknown_field',lambda p:p['representations'][0].update(extra=1)),('missing',lambda p:p.pop('representations')),('limit',lambda p:p.update(representations=p['representations']*13))]:
            p=copy.deepcopy(model);mut(p);bad=write('bad_'+n+'.json',p);badpolicies.append(bad);src=write('invalid_model.c',source('int main(void){return 0;}'));run('model_'+n,[compiler,'--overlay=physics-semantics','--physical-semantics-policy',bad,'-c',src,'-o',out/'invalid.o'],1)
        if probe:run('native_model',[probe,policy,badpolicies[0]])
        result.update(passed=True,reports={n:d['selection'] for n,d in reports.items()},representation_model=reports['tu_a']['representation_model'])
    finally:write('result.json',result)
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',required=True,type=Path);p.add_argument('--out',type=Path);p.add_argument('--probe',type=Path);a=p.parse_args()
    if a.out:a.out.mkdir(parents=True,exist_ok=False);check(a.compiler.resolve(),a.out.resolve(),a.probe.resolve() if a.probe else None)
    else:
        with tempfile.TemporaryDirectory(prefix='fisics-representations-') as tmp:check(a.compiler.resolve(),Path(tmp),a.probe.resolve() if a.probe else None)
if __name__=='__main__':main()
