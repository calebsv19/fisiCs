"""Source-bound thermal kernels, raw boundary inventory and failure controls."""
from pathlib import Path
import ast,copy,hashlib,json,re

def qualify(a,out,run):
    out.mkdir();core=a.core_root;host=a.host_root;root=Path(__file__).resolve().parents[2]
    spec=json.loads((core/'physics/thermal_qualification.json').read_text());policy=core/'physics/thermal_policy.json'
    includes=['-I'+str(core/'include'),'-I'+str(root/'include'),'-I'+str(core.parents[1]/'core/core_sim/include'),'-I'+str(core.parents[1]/'shape/external/cjson'),'-I'+str(host/'include'),'-I'+str(host/'src')]
    if a.physics_root:includes+=['-I'+str(a.physics_root/'include'),'-I'+str(a.physics_root/'src/tools')]
    facts=[];sources=[*sorted((core/'src').glob('*.c')),core/'include/core_thermal_physical.h',host/'src/experiment/participant.c',host/'src/assembly/cycle.c',host/'src/coupling/receiver.c']
    def walk(n):
        yield n
        for child in n.get('inner',[]):yield from walk(child)
    for i,src in enumerate(sources):
        tree=json.loads(run('ast-'+str(i),['clang','-x','c','-std=c11',*includes,'-fsyntax-only','-Xclang','-ast-dump=json',src]))
        text=src.read_text()
        for n in tree.get('inner',[]):
            if n.get('kind')!='FunctionDecl' or not any(x.get('kind')=='CompoundStmt' for x in n.get('inner',[])):continue
            loc=n.get('loc',{});loc=loc.get('expansionLoc',loc)
            if loc.get('includedFrom') or (loc.get('file') and Path(loc['file'])!=src):continue
            lo=n['range']['begin'];lo=lo.get('expansionLoc',lo);hi=n['range']['end'];hi=hi.get('expansionLoc',hi)
            begin,end=lo['offset'],hi['offset']+hi.get('tokLen',1);body=text[begin:end];calls=[]
            for v in walk(n):
                if v.get('kind')=='CallExpr':
                    refs=[x.get('referencedDecl',{}) for x in walk(v.get('inner',[{}])[0]) if x.get('kind')=='DeclRefExpr']
                    calls.append(next((x['name'] for x in refs if x.get('kind')=='FunctionDecl'),'indirect'))
            facts.append(dict(file=str(src),name=n['name'],line=text[:begin].count('\n')+1,signature=n['type']['qualType'],body_sha256=hashlib.sha256(body.encode()).hexdigest(),calls=sorted(set(calls))))
    names=[x['name'] for x in facts];assert set(spec['strict_functions'])<=set(names)
    header_names={x['name'] for x in facts if x['file'].endswith('core_thermal_physical.h')}
    assert header_names==set(spec['strict_functions']+spec['declared_reference_boundaries']),header_names
    used={call for x in facts if '/src/' in x['file'] for call in x['calls']}
    assert set(spec['strict_functions'])<=used,('unadopted helper',set(spec['strict_functions'])-used)
    source=out/'probe.c';source.write_text('#include "core_thermal_physical.h"\n');coverage=[]
    def report(name,mode,src=source,expected=0,extra=(),function=None):
        fn=function or name;dest=out/(name+'-'+mode+'.json')
        prefix='physical-semantics' if mode=='physical' else 'units-coverage'
        flags=['--overlay=physics-semantics','-DCORE_THERMAL_PHYSICAL=1','--physical-semantics-policy',policy] if mode=='physical' else ['--overlay=physics-units','-DCORE_THERMAL_UNITS=1']
        run(name+'-'+mode,[a.compiler,*flags,*extra,*includes,'--'+prefix+'-function',fn,'--'+prefix+'-strict','--'+prefix+'-allow-literal-assumptions','--emit-'+prefix+'-json',dest,src],expected=expected)
        if dest.exists():
            d=json.loads(dest.read_text());s=d['selection'] if mode=='physical' else d['strict_policy']
            assert s['accepted']==(expected==0),(name,mode,s)
            assert s['required']==s['checked']+s['permitted_trust']+s['blocked']
            coverage.append(dict(function=fn,label=name,mode=mode,selection=s))
            return s
        assert expected
    for fn in spec['strict_functions']:
        for mode in ['units','physical']:
            # Select the real adopted TU where available, preserving include provenance.
            actual=next(Path(x['file']) for x in facts if '/src/' in x['file'] and fn in x['calls'])
            s=report(fn,mode,src=actual);assert s['checked']>0 and s['blocked']==0
    for fn in spec['declared_reference_boundaries']:
        report('boundary-'+fn,'physical',expected=1,function=fn,extra=['--physical-semantics-allow-declared-boundaries'])
    # Remaining actual function bodies are measured, never promoted to whole-owner proof.
    for f in facts:
        if 'core_thermal_physical.h' in f['file']:continue
        for mode in ['units','physical']:
            dest=out/('raw-'+Path(f['file']).stem+'-'+f['name']+'-'+mode+'.json');prefix='units-coverage' if mode=='units' else 'physical-semantics'
            flags=['--overlay=physics-semantics','-DCORE_THERMAL_PHYSICAL=1','--physical-semantics-policy',policy] if mode=='physical' else ['--overlay=physics-units','-DCORE_THERMAL_UNITS=1']
            run('raw-'+Path(f['file']).stem+'-'+f['name']+'-'+mode,[a.compiler,*flags,*includes,'--'+prefix+'-function',f['name'],'--emit-'+prefix+'-json',dest,f['file']])
            data=json.loads(dest.read_text());coverage.append(dict(function=f['name'],file=f['file'],mode=mode,strict_claim=False,report=data.get('selection',data.get('denominator'))))
    negatives=[
      ('absolute-product','double CT_E check(double c CT_C,double t CT_T){return c*t;}'),
      ('absolute-reconstruction','double CT_T check(double e CT_E,double c CT_C){return e/c;}'),
      ('kind-launder','double CT_E check(double p CT_P,double t CT_T){return p*t;}'),
      ('ratio-angle','double CT_R check(double t CT_DIM(1) CT_KIND("quantity.angle")){return expm1(t);}'),
      ('raw-power','double CT_E check(double p,double t CT_DT){return ct_external(p,t);}'),
      ('form-relabel','double CT_E CT_FORM("energy.thermal") check(double e CT_E CT_FORM("energy.chemical")){return e;}'),
      ('torque-substitution','double CT_E check(double q CT_DIM(energy) CT_KIND("quantity.torque"),double c CT_C){return q/c;}')]
    for name,body in negatives:
        p=out/(name+'.c');p.write_text('#include "core_thermal_physical.h"\n'+body+'\n')
        report(name,'physical',src=p,expected=1,function='check',extra=['--physical-semantics-allow-declared-boundaries'])
    for name,body in [
      ('wrong-material-origin','double check(CtMaterialEnergy e){return ct_zero_restore(e,1);}'),
      ('stored-not-transfer','double check(CtZeroKelvinEnergy e){return ct_exchange_power(1,e,1);}')]:
        p=out/(name+'.c');p.write_text('#include "core_thermal_physical.h"\n'+body+'\n')
        for tool,label in [('clang','clang'),(a.compiler,'fisics')]:run(name+'-'+label,[tool,*includes,'-c',p,'-o',out/(name+'-'+label+'.o')],expected=1)
    # Dimension/kind removal cannot shrink the selection into acceptance.
    text=(core/'include/core_thermal_physical.h').read_text()
    for label,pattern in [('no-capacity-kind',r'CT_KIND\("quantity.heat_capacity"\)'),('no-capacity-dimension',r'CT_DIM\(energy / temperature\)')]:
        work=out/label;work.mkdir();changed,count=re.subn(pattern,'',text);assert count
        (work/'core_thermal_physical.h').write_text(changed)
        report(label,'physical',expected=1,function='ct_capacity',extra=['-I'+str(work)])
    wiring=[]
    for stem in ['core_thermal_rod','core_thermal_material_rod']:
        text=(core/'src'/(stem+'.c')).read_text()
        for label,old,new in [('rhs-as-capacity','ct_increment(value,diagonal[i])','ct_increment(value,rhs[i])'),('capacity-as-energy','ct_eliminate_e(rhs[i],factor,rhs[i-1])','ct_eliminate_e(diagonal[i],factor,rhs[i-1])')]:
            assert old in text
            src=out/(stem+'-'+label+'.c');src.write_text(text.replace(old,new))
            for mode in ['units','physical']:
                flags=['--overlay=physics-semantics','-DCORE_THERMAL_PHYSICAL=1','--physical-semantics-policy',policy] if mode=='physical' else ['--overlay=physics-units','-DCORE_THERMAL_UNITS=1']
                run(stem+'-'+label+'-'+mode,[a.compiler,*flags,*includes,'-c',src,'-o',out/(stem+'-'+label+'-'+mode+'.o')],expected=1)
            wiring.append(stem+'/'+label)
    py=[]
    for name in ['electrical_heat.py','electrical_identity.py']:
        p=host/'tools'/name
        for n in ast.parse(p.read_text()).body:
            if isinstance(n,ast.FunctionDef):py.append(dict(file=str(p),function=n.name,line=n.lineno))
    data=dict(passed=True,functions=facts,python_interfaces=py,coverage=coverage,manifest=spec,wiring_failures=wiring,strict_kernel_count=len(spec['strict_functions']),whole_owner_strict_claim=False)
    (out/'inventory.json').write_text(json.dumps(data,indent=2)+'\n');return data
