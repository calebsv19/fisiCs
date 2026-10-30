"""P6.1.3 refinement of the copied structural candidate, never shared adoption."""
import hashlib
import json
import re
from solver_physical_profile import apply_profile

def apply_mechanical_profile(candidate,out):
    header=candidate/'src/core_structural_frame_physical.h'
    original=header.read_text()
    apply_profile(candidate,out)
    text=header.read_text()
    for field,kind in [('tr[4][2]','force_per_angle'),('rt[2][4]','moment_per_displacement')]:
        old='double '+field+' CS_PHYSICAL_DIM(force) CS_PHYSICAL_KIND("quantity.force")'
        assert text.count(old)==1
        text=text.replace(old,old.replace('quantity.force','quantity.'+kind))
    # Force density and stiffness share dimensions, but only stiffness is
    # admitted by the four mechanical application primitives.
    for field in ['x','y']:
        old='double '+field+' CS_PHYSICAL_DIM(force/length) CS_PHYSICAL_KIND("quantity.translational_stiffness")'
        assert text.count(old)==1
        text=text.replace(old,old.replace('quantity.translational_stiffness','quantity.line_load'))
    # One value is represented by two different coefficient kinds. Derive both
    # from the same formula, rather than reinterpreting one semantic kind as the other.
    for name in ['coupling','zero_coupling']:
        pattern=r'    double '+name+r' CS_PHYSICAL_DIM\(force\) CS_PHYSICAL_KIND\("quantity.force"\) = ([^;]+);'
        match=re.search(pattern,text);assert match,name
        expr=match[1]
        declarations='\n'.join('    double '+name+'_'+suffix+' CS_PHYSICAL_DIM(force) CS_PHYSICAL_KIND("quantity.'+kind+'") = '+expr+';' for suffix,kind in [('tr','force_per_angle'),('rt','moment_per_displacement')])
        text=text[:match.start()]+declarations+text[match.end():]
    text=re.sub(r'(k->tr\[[^;]+?= -?)(zero_coupling|coupling);',r'\1\2_tr;',text)
    text=re.sub(r'(k->rt\[[^;]+?= -?)(zero_coupling|coupling);',r'\1\2_rt;',text)
    # Verify all changes: erase metadata, collapse the deliberately duplicated
    # coefficient expressions, and recover the original header byte for byte.
    erased=re.sub(r' CS_PHYSICAL_KIND\("quantity\.[a-z_]+"\)','',text)
    erased=erased.replace('\n#define CS_PHYSICAL_KIND(v) FISICS_QUANTITY_KIND(v)','').replace('\n#define CS_PHYSICAL_KIND(v)','')
    erased=re.sub(r'^    double (?:zero_coupling|coupling)_rt CS_PHYSICAL_DIM\(force\) = [^;]+;\n','',erased,flags=re.M)
    for name in ['zero_coupling','coupling']:
        erased=erased.replace(name+'_tr',name).replace(name+'_rt',name)
    assert erased==original,'unexpected structural numeric/ABI edit'
    header.write_text(text)
    catalog=json.loads((out/'catalog.json').read_text())
    for kind in ['force_per_angle','moment_per_displacement']:
        catalog['quantity_kinds'].append(dict(id='quantity.'+kind,dimension_system='fisiCs.si_dim8.v1',dimension_vector=[1,1,-2,0,0,0,0,0],dimension_text='force'))
    catalog['quantity_kinds'].append(dict(id='quantity.line_load',dimension_system='fisiCs.si_dim8.v1',dimension_vector=[0,1,-2,0,0,0,0,0],dimension_text='force/length'))
    catalog['catalog_id']='solver.p613'
    policy=json.loads((out/'policy.json').read_text());policy.update(schema_version=4,policy_id='solver.p613',operation_rules=[])
    policy['linear_kinds'] += ['quantity.force_per_angle','quantity.moment_per_displacement','quantity.line_load']
    for coeff,coordinate,result in [('translational_stiffness','displacement','force'),('force_per_angle','angle','force'),('moment_per_displacement','displacement','torque'),('rotational_stiffness','angle','torque')]:
        for a,b in [(coeff,coordinate),(coordinate,coeff)]:
            policy['operation_rules'].append(dict(id='rule.'+a+'.'+b,operation='multiply',operand_kinds=['quantity.'+a,'quantity.'+b],result_kind='quantity.'+result,reference_role='linear',origin_policy='none',provenance='inputs'))
    for n,v in [('catalog.json',catalog),('policy.json',policy)]:(out/n).write_text(json.dumps(v,indent=2)+'\n')
    return dict(header_sha256=hashlib.sha256(header.read_bytes()).hexdigest(),numerical_source_identical=False,normalized_erasure_matches_original=True,duplicated_coefficient_expressions=['coupling','zero_coupling'],kinds=len(catalog['quantity_kinds']),scope='copied header refinement; four scalar application contracts; construction still unverified; no shared adoption')
