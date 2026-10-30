"""Current static-owner inventory; AST discovery is distinct from strict proof."""
import hashlib, json, subprocess
from collections import Counter
from pathlib import Path
PHYSICAL={f'cs_physical_{n}' for n in ['build','apply','equivalent_load','recover_end','as_tr','as_rt','normal_stress','bending_stress','translation_work','rotation_work','line_moment']}|{'cs_frame_elastic_sum'}
DIMENSIONAL={'cs_frame_member_energy_quadrature','cs_frame_distributed_work_quadrature'}
CONTROLS={'cs_frame_world_dofs_assumed_order','geometric_rank','index_of','root','join','root_of','compare_id','sorted_order'}
PARTS=['element','compile','connections','assemble','loads','linear','result','recovery','inertia']
def inventory(source,base,out,compiler,include):
    out.mkdir();rows=[];commands=[];checks=[]
    files=[source/'src'/('core_structural_frame_'+p+'.c') for p in PARTS]+[source/'src'/('core_structural_'+p+'.c') for p in ['model','validate','analysis']]+[source/'src'/('core_structural_frame_'+p+'.h') for p in ['physical','boundaries','energy']]
    def walk(n):
        yield n
        for c in n.get('inner',[]):yield from walk(c)
    for p in files:
        args=['clang','-std=c11','-x','c',*include,'-DCORE_STRUCTURAL_VERSION="'+(source/'VERSION').read_text().strip()+'"','-fsyntax-only','-Xclang','-ast-dump=json',str(p)]
        job=subprocess.run(list(map(str,args)),capture_output=True,text=True,timeout=60)
        commands.append(dict(command=list(map(str,args)),exit=job.returncode));assert job.returncode==0,(p,job.stderr)
        ast=json.loads(job.stdout);text=p.read_text()
        for n in ast['inner']:
            if n.get('kind')!='FunctionDecl' or not any(c.get('kind')=='CompoundStmt' for c in n.get('inner',[])):continue
            loc=n.get('loc',{});loc=loc.get('expansionLoc',loc)
            if loc.get('includedFrom') or (loc.get('file') and Path(loc['file'])!=p):continue
            begin=n['range']['begin'];begin=begin.get('expansionLoc',begin);end=n['range']['end'];end=end.get('expansionLoc',end)
            lo=begin.get('offset');hi=end.get('offset',0)+end.get('tokLen',1)
            assert lo is not None and hi<=len(text),(p,n['name'])
            body=text[lo:hi];calls=[]
            for k in walk(n):
                if k.get('kind')=='CallExpr':
                    refs=[q.get('referencedDecl',{}) for q in walk(k.get('inner',[{}])[0]) if q.get('kind')=='DeclRefExpr']
                    calls.append(next((q['name'] for q in refs if q.get('kind')=='FunctionDecl'),'indirect'))
            name=n['name']
            category=('physical_kernel' if name in PHYSICAL else 'dimension_only_kernel' if name in DIMENSIONAL else 'deferred_dynamic' if p.name.endswith('_inertia.c') else 'shared_dispatch_or_control' if '_frame_' not in p.name else 'ordinary_control' if name in CONTROLS else 'numerical_or_raw_boundary')
            rows.append(dict(file=str(p.relative_to(source)),name=name,line=text[:lo].count('\n')+1,signature=n['type']['qualType'],body_sha256=hashlib.sha256(body.encode()).hexdigest(),calls=sorted(set(calls)),category=category,static_frame_identity='not_inferred',physical_strict_claim=False))
    assert len({(x['file'],x['name']) for x in rows})==len(rows)
    assert PHYSICAL|DIMENSIONAL <= {x['name'] for x in rows}
    # Independently read every original static outer function's selected report.
    # Raw scopes are expected refusals, not successful physics qualification.
    for row in rows:
        if row['category'] in ['deferred_dynamic','shared_dispatch_or_control']:continue
        p=source/row['file'];src=p
        if p.suffix=='.h':
            src=out/'scope.c';src.write_text('#include "'+p.name+'"\n')
        dest=out/(row['name']+'.json')
        sem=['--overlay=physics-semantics','--physical-semantics-policy',source/'physics/frame_policy.json','-DCORE_STRUCTURAL_PHYSICAL_SEMANTICS=1']
        args=[compiler,*sem,*include,'--physical-semantics-function',row['name'],'--emit-physical-semantics-json',dest,src]
        job=subprocess.run(list(map(str,args)),capture_output=True,text=True,timeout=60)
        (out/(row['name']+'.log')).write_text(job.stdout+job.stderr);commands.append(dict(command=list(map(str,args)),exit=job.returncode));assert job.returncode==0,(row['name'],job.stderr)
        data=json.loads(dest.read_text());sel=data['selection'];assert sel['function']==row['name'] and data['analysis_valid']
        assert sel['required']==sel['checked']+sel['permitted_trust']+sel['blocked']
        row['report_selection']=sel
        row['obligations']=len(data['obligations'])
        # A report without strict enforcement is never promoted to acceptance.
        # The joined physical/energy gates supply the separate strict proof.
        if row['category']=='physical_kernel':row['strict_evidence_gate']='energy' if row['name']=='cs_frame_elastic_sum' else 'physical'
        elif row['category']=='dimension_only_kernel':row['strict_evidence_gate']='energy: units-only'
    result=dict(schema='fisics.structural_owner_inventory.v1',passed=True,owner_version=(source/'VERSION').read_text().strip(),files=[str(p.relative_to(source)) for p in files],functions=rows,counts=dict(Counter(x['category'] for x in rows)),commands=commands,whole_function_strict_claim=False,deferred_source_files=sorted(str(p.relative_to(source)) for p in (source/'src').glob('*.c') if p not in files))
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n');return result
