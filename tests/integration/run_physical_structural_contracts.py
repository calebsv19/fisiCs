#!/usr/bin/env python3
"""P7.3 maintained structural scalar contracts and fail-closed controls."""
import argparse, copy, hashlib, json, os, shutil, subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]

def check(compiler, source, base, out, probe=None):
    commands=[]; cases=[]; result={'passed':False,'commands':commands,'cases':cases,
        'compiler_sha256':hashlib.sha256(compiler.read_bytes()).hexdigest()}
    env={**os.environ,'FISICS_MAX_PROCS':'0','PYTHONDONTWRITEBYTECODE':'1'}
    env.pop('FISICS_OVERLAY',None)
    def run(label,cmd,code=0):
        p=subprocess.run(list(map(str,cmd)),env=env,capture_output=True,text=True,timeout=180)
        (out/(label+'.log')).write_text(p.stdout+p.stderr)
        commands.append({'label':label,'command':list(map(str,cmd)),'exit':p.returncode,'expected':code})
        assert p.returncode==code,(label,p.returncode,(p.stdout+p.stderr)[-1500:])
        assert code or 'Error:' not in p.stdout+p.stderr,label
        return p.stdout
    def write(name,data):
        p=out/name;p.write_text(json.dumps(data,indent=2)+'\n');return p
    header=source/'src/core_structural_frame_physical.h';text=header.read_text()
    inputs=[header,source/'physics/frame_policy.json',source/'physics/frame_catalog.json',*[source/f'src/core_structural_frame_{part}.c' for part in ['element','loads','recovery']]]
    pins={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs}
    result['runner_sha256']=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    policy=json.loads((source/'physics/frame_policy.json').read_text())
    catalog=json.loads((source/'physics/frame_catalog.json').read_text())
    write('frame_catalog.json',catalog);model=write('frame_policy.json',policy)
    dims={x['id']:x['dimension_text'] for x in catalog['quantity_kinds']}
    include=['-I',ROOT/'include','-I',source/'include','-I',source/'src','-I',base/'include']
    sem=['--overlay=physics-semantics','--physical-semantics-policy',model,'-DCORE_STRUCTURAL_PHYSICAL_SEMANTICS=1']
    wrapper=out/'probe.c';local=out/'core_structural_frame_physical.h'
    def declared(k,name):return 'double '+name+' CS_PHYSICAL_DIM('+dims['quantity.'+k]+') CS_PHYSICAL_KIND("quantity.'+k+'")'
    def report(label,fn='check',body='',code=0,permissions=False,modified=None,pol=None,actual=None):
        local.write_text(text if modified is None else modified)
        wrapper.write_text('#include "core_structural_frame_physical.h"\n'+body+'\n')
        src=actual or wrapper;dest=out/(label+'.json')
        args=sem if pol is None else ['--overlay=physics-semantics','--physical-semantics-policy',pol,'-DCORE_STRUCTURAL_PHYSICAL_SEMANTICS=1']
        run(label,[compiler,*args,*include,'--physical-semantics-strict','--physical-semantics-allow-literal-assumptions',*(['--physical-semantics-allow-declared-boundaries'] if permissions else []),'--physical-semantics-function',fn,'--emit-physical-semantics-json',dest,src],code)
        if dest.exists():
            d=json.loads(dest.read_text());s=d['selection'];assert s['accepted']==(code==0)
            assert s['required']==s['checked']+s['permitted_trust']+s['blocked']
            if not code:assert s['enumeration_complete'] and s['checked']>0 and not s['blocked'] and not s['allow_declared_boundaries']
            cases.append({'label':label,'selection':s})
            return d
        assert code
        cases.append({'label':label,'refused_before_publication':True})
    try:
        rows=[('cs_physical_build','element',(61,7,15)),('cs_physical_apply','recovery',(26,2,6)),('cs_physical_equivalent_load','loads',(16,2,0)),('cs_physical_recover_end','recovery',(28,0,6)),*[(f'cs_physical_{n}','element',(1 if n.startswith('as_') else 2,0,0)) for n in ['as_tr','as_rt','normal_stress','bending_stress','translation_work','rotation_work','line_moment']]]
        first=None
        for fn,part,counts in rows:
            d=report(fn,fn,actual=source/f'src/core_structural_frame_{part}.c')
            s=d['selection'];assert (s['checked'],s['permitted_trust'],s['ordinary_controls'])==counts
            if first is None:first=d
        for label,callee,a,b,k in [('work-translation','cs_physical_translation_work','force','displacement','energy'),('work-rotation','cs_physical_rotation_work','torque','angle','energy'),('line-moment','cs_physical_line_moment','force','geometric_length','torque')]:
            report(label,body=declared(k,'')+' check('+declared(a,'a')+','+declared(b,'b')+'){return '+callee+'(a,b);}')
        runtime=out/'runtime.c'
        runtime.write_text('#include "core_structural_frame_physical.h"\n#include <stdio.h>\nint main(void){printf("%.17g %.17g %.17g %.17g %.17g %.17g %.17g\\n",cs_physical_as_tr(7),cs_physical_as_rt(7),cs_physical_normal_stress(1000,2),cs_physical_bending_stress(300,3),cs_physical_translation_work(10,.25),cs_physical_rotation_work(8,.125),cs_physical_line_moment(4,2));return 0;}\n')
        local.write_text(text)
        for label,tool,args in [('clang','clang',['-std=c11','-Wall','-Wextra','-Wpedantic','-Werror']),('units',compiler,['--overlay=physics-units']),('semantic',compiler,sem)]:
            obj=out/('runtime-'+label+'.o');binary=out/('runtime-'+label)
            run('runtime-'+label+'-compile',[tool,*args,*include,'-c',runtime,'-o',obj])
            run('runtime-'+label+'-link',['clang',obj,'-o',binary])
            assert run('runtime-'+label+'-run',[binary])=='7 7 500 100 2.5 1 8\n'
        # Every ordinary scalar contract has independent positive and missing-rule cases.
        for i,r in enumerate(policy['operation_rules']):
            if r['operation'] not in ['multiply','divide']:continue
            a,b=[x.removeprefix('quantity.') for x in r['operand_kinds']];k=r['result_kind'].removeprefix('quantity.')
            body=declared(k,'')+' check('+declared(a,'a')+','+declared(b,'b')+'){return a'+('*' if r['operation']=='multiply' else '/')+'b;}'
            report('scalar-'+str(i),body=body)
            unknown=declared(k,'')+' check(double a,'+declared(b,'b')+'){return a'+('*' if r['operation']=='multiply' else '/')+'b;}'
            report('unknown-operand-'+str(i),body=unknown,code=1,permissions=True)
            stripped=copy.deepcopy(policy);stripped['operation_rules'].remove(r)
            # Header definitions need their rules too; refusal is required even outside selection.
            report('missing-rule-'+str(i),body=body,code=1,permissions=True,pol=write('missing-rule-'+str(i)+'-policy.json',stripped))
        # Same-Dim8 substitutions remain legal units/C, but fail semantic qualification.
        for label,old,new,fn in [
            ('wrong-coupling','= cs_physical_as_tr(six','= cs_physical_as_rt(six','cs_physical_build'),
            ('changed-constructor','{ return value; }','{ return -value; }','cs_physical_build'),
            ('changed-work-body','{ return force*displacement; }','{ return displacement*force; }','cs_physical_translation_work'),
            ('stripped-constructor','CS_PHYSICAL_OPERATION("rule.construct.tr")','', 'cs_physical_build'),
            ('stripped-coordinate',' CS_PHYSICAL_KIND("quantity.displacement")','', 'cs_physical_apply')]:
            assert old in text;modified=text.replace(old,new,1)
            report(label,fn=fn,code=1,permissions=True,modified=modified)
            run(label+'-clang',['clang','-std=c11','-pedantic-errors',*include,'-fsyntax-only',wrapper])
            run(label+'-units',[compiler,'--overlay=physics-units',*include,'-c',wrapper,'-o',out/(label+'.o')])
        # Unknown input cannot be supplied by a destination, either trust flag or a cast.
        report('unknown-constructor',body=declared('force_per_angle','')+' check(double x){return cs_physical_as_tr(x);}',code=1,permissions=True)
        report('unknown-work',body=declared('energy','')+' check(double f,'+declared('displacement','x')+'){return cs_physical_translation_work(f,x);}',code=1,permissions=True)
        report('wrong-constructor-input',body=declared('force_per_angle','')+' check('+declared('force','x')+'){return cs_physical_as_tr(x);}',code=1,permissions=True)
        report('wrong-work-pair',body=declared('energy','')+' check('+declared('rotational_stiffness','m')+','+declared('angle','x')+'){return cs_physical_rotation_work(m,x);}',code=1,permissions=True)
        report('unqualified-force-displacement',body=declared('energy','')+' check('+declared('force','f')+','+declared('displacement','x')+'){return f*x;}',code=1,permissions=True)
        report('unqualified-force-span',body=declared('torque','')+' check('+declared('force','f')+','+declared('geometric_length','x')+'){return f*x;}',code=1,permissions=True)
        report('raw-refusal',fn='cs_physical_dense_restore_assumed_si',code=1,permissions=True)
        report('indirect-constructor',body=declared('force_per_angle','')+' check('+declared('coupling_coefficient','v')+'){double (*f)(double)=cs_physical_as_tr;return f(v);}',code=1,permissions=True)
        report('stripped-constructor-input',fn='cs_physical_build',code=1,permissions=True,modified=text.replace(' CS_PHYSICAL_KIND("quantity.coupling_coefficient")','',1))
        collision=copy.deepcopy(policy)
        collision['representations']=[dict(id='rep.test',storage='record',scalar='double',rank=1,extent=3,fields=['x','y','z'],parity='polar')]
        collision['shape_helpers']=[dict(id='rule.construct.tr',operation='add',operand_kinds=['quantity.force','quantity.force'],operand_representations=['rep.test','rep.test'],result_kind='quantity.force',result_representation='rep.test',preconditions='common_cartesian_basis_right_handed',provenance='declared_implementation_and_basis')]
        report('identity-collision',fn='cs_physical_apply',code=1,pol=write('collision-policy.json',collision))
        # New constructor models participate in the existing exact cross-TU check.
        prefix='#include <fisics/extensions.h>\n'
        ctor='static inline double FISICS_DIM(force) FISICS_QUANTITY_KIND("quantity.force_per_angle") FISICS_PHYSICAL_OPERATION("rule.construct.tr") construct(double v FISICS_DIM(force) FISICS_QUANTITY_KIND("quantity.coupling_coefficient")){return v;}\n'
        signature='double FISICS_DIM(force) FISICS_QUANTITY_KIND("quantity.force_per_angle") produce(double v FISICS_DIM(force) FISICS_QUANTITY_KIND("quantity.coupling_coefficient"))'
        receipts=[]
        for label,body in [('a',signature+'{return construct(v);}'),('b',signature+';\n'+signature.replace('produce','consume')+'{return produce(v);}')]:
            src=out/('tu-'+label+'.c');src.write_text(prefix+ctor+body+'\n');dest=out/('tu-'+label+'.json')
            run('tu-'+label,[compiler,*sem,*include,'--physical-semantics-function','produce' if label=='a' else 'consume','--emit-physical-semantics-json',dest,src]);receipts.append(dest)
        run('cross-tu',[compiler,'--physical-semantics-check',*receipts])
        tampered=json.loads(receipts[0].read_text())
        row=next(x for x in tampered['operation_model']['rules'] if x['id']=='rule.construct.tr');row['result_kind']='quantity.moment_per_displacement'
        run('cross-tu-tamper',[compiler,'--physical-semantics-check',write('tampered.json',tampered),receipts[1]],1)

        # Model admission cannot use equal dimensions to reinterpret unrelated quantities.
        invalid=copy.deepcopy(policy);invalid['operation_rules'].append(dict(id='rule.ambiguous.force',operation='multiply',operand_kinds=['quantity.force','quantity.geometric_length'],result_kind='quantity.torque',reference_role='linear',origin_policy='none',provenance='inputs'))
        bad=write('invalid-policy.json',invalid)
        report('unsupported-rule',fn='cs_physical_apply',code=1,pol=bad)
        for version in [1,2,3,4,5,6]:
            legacy=copy.deepcopy(policy);legacy['schema_version']=version
            # Preserve the exact legacy envelope; new contracts alone must refuse.
            for v,keys in [(7,['energy_forms','energy_transfers']),(6,['frame_contexts','reference_points','frames','frame_transforms']),(5,['representations','shape_helpers']),(2,['operation_rules'])]:
                if version<v:
                    for key in keys:legacy.pop(key,None)
            if version>=3:report('legacy-constructor-'+str(version),fn='cs_physical_apply',code=1,pol=write('legacy-'+str(version)+'.json',legacy))
        # Independently encode the full ordered operation fingerprint.
        def num(n):return int(n).to_bytes(8,'little')
        def txt(s):b=s.encode();return num(len(b))+b
        ops={'fabs':2,'fmax':4,'multiply':100,'divide':101,'frame_coupling_tr':102,'frame_coupling_rt':103,'translation_work':104,'rotation_work':105,'transverse_line_moment':106}
        rs=sorted(policy['operation_rules'],key=lambda r:(ops[r['operation']],*r['operand_kinds']))
        payload=txt('fisiCs.physical_mechanics.v1')+num(len(rs))
        for r in rs:
            a=r['operand_kinds'];payload+=txt(r['id'])+txt(r['operation'])+txt(a[0])+txt(a[1] if len(a)>1 else '')+txt(r['result_kind'])+num(1)+txt('none')+txt('inputs')
        h=1469598103934665603
        for b in payload:h=((h^b)*1099511628211)&((1<<64)-1)
        assert first['operation_model']['fingerprint']==f'{h:016x}'
        assert first['operation_model']['rules']==rs
        result['operation_fingerprint']=f'{h:016x}'
        # Native driver proves allocation rollback, independent models and legacy reload.
        if probe:
            v1={k:copy.deepcopy(v) for k,v in policy.items() if k in ['schema','schema_version','policy_id','catalog_paths','linear_kinds','affine_pairs','origins']};v1['schema_version']=1
            result['native']=json.loads(run('native-allocation',[probe,write('v1.json',v1),model,bad]))
        assert all(hashlib.sha256(Path(p).read_bytes()).hexdigest()==h for p,h in pins.items()),'selected owner/model drift'
        result['source_sha256']=pins
        assert header.read_text()==text
        result['passed']=True
        print(f'Physical structural contracts: PASS; {len(cases)} cases, {len(commands)} commands')
    finally:(out/'results.json').write_text(json.dumps(result,indent=2)+'\n')

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for n in ['compiler','source-root','base-root','out']:p.add_argument('--'+n,type=Path,required=True)
    p.add_argument('--probe',type=Path)
    a=p.parse_args();a.out.mkdir(parents=True,exist_ok=False)
    check(a.compiler.resolve(),a.source_root.resolve(),a.base_root.resolve(),a.out.resolve(),a.probe.resolve() if a.probe else None)
if __name__=='__main__':main()
