"""Explicit semantic profile for the hash-bound structural source candidate.

No equation inference: products stay unknown until P6. Only this copied internal
header changes; public source owner, numerical text and layouts remain intact.
"""
import hashlib
import json
from pathlib import Path
import re

def apply_profile(candidate,out):
    header=candidate/'src/core_structural_frame_physical.h'
    before=header.read_text()
    kinds={
        'pressure':('pressure',[ -1,1,-2,0,0,0,0,0]),
        'area':('area',[2,0,0,0,0,0,0,0]),
        'second_moment_of_area':('inertia',[4,0,0,0,0,0,0,0]),
        'length':('displacement',[1,0,0,0,0,0,0,0]),
        'force/length':('translational_stiffness',[0,1,-2,0,0,0,0,0]),
        'force':('force',[1,1,-2,0,0,0,0,0]),
        'force*length':('torque',[2,1,-2,0,0,0,0,0]),
        'scalar':('angle',[0,0,0,0,0,0,0,0])}
    # rr coefficients and local rotational coefficients have stiffness meaning,
    # despite sharing torque dimensions. Coupling remains explicitly distinct.
    def annotate(m):
        kind=kinds[m[1]][0]
        return m[0]+' CS_PHYSICAL_KIND("quantity.'+kind+'")'
    text=re.sub(r'CS_PHYSICAL_DIM\((pressure|area|second_moment_of_area|length|force/length|force\*length|force|scalar)\)',annotate,before)
    for field in ['rr[2][2]','rotation_diagonal','rotation_off_diagonal']:
        pattern=r'(double '+re.escape(field)+r' CS_PHYSICAL_DIM\(force\*length\) CS_PHYSICAL_KIND\(")quantity.torque("\))'
        text,count=re.subn(pattern,r'\1quantity.rotational_stiffness\2',text)
        assert count==1,field
    text=text.replace('#define CS_PHYSICAL_UNIT(v) FISICS_UNIT(v)', '#define CS_PHYSICAL_UNIT(v) FISICS_UNIT(v)\n#define CS_PHYSICAL_KIND(v) FISICS_QUANTITY_KIND(v)')
    text=text.replace('#define CS_PHYSICAL_UNIT(v)\n', '#define CS_PHYSICAL_UNIT(v)\n#define CS_PHYSICAL_KIND(v)\n')
    # Attribute erasure must recover exactly the original candidate text.
    restored=re.sub(r' CS_PHYSICAL_KIND\("quantity\.[a-z_]+"\)','',text)
    restored=restored.replace('\n#define CS_PHYSICAL_KIND(v) FISICS_QUANTITY_KIND(v)','').replace('\n#define CS_PHYSICAL_KIND(v)','')
    assert restored==before,'semantic profiling changed numerical source'
    header.write_text(text)
    entries=[dict(id='quantity.'+kind,dimension_system='fisiCs.si_dim8.v1',dimension_vector=dim,dimension_text=dimtext) for dimtext,(kind,dim) in kinds.items()]
    entries.append(dict(entries[6],id='quantity.rotational_stiffness'))
    catalog=dict(schema='fisiCs.physical_contract_catalog',schema_version=1,catalog_id='solver.p56',quantity_kinds=entries,contracts=[])
    policy=dict(schema='fisiCs.physical_semantics_policy',schema_version=1,policy_id='solver.p56',catalog_paths=['catalog.json'],linear_kinds=[v['id'] for v in entries],affine_pairs=[],origins=[])
    for name,data in [('catalog.json',catalog),('policy.json',policy)]: (out/name).write_text(json.dumps(data,indent=2)+'\n')
    return dict(header_sha256=hashlib.sha256(header.read_bytes()).hexdigest(),numerical_source_identical=True,kinds=len(entries),scope='copied internal candidate header; no physical product inference or shared adoption')
