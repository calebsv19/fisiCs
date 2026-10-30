#!/usr/bin/env python3
"""Stable P5.2 model qualification; no source-level semantic enforcement implied."""
import argparse
import copy
import json
import pathlib
import subprocess
import tempfile


def quantity(name, vector, text):
    return dict(id="quantity."+name, dimension_system="fisiCs.si_dim8.v1",
                dimension_vector=vector, dimension_text=text)


def contract(name, form="scalar"):
    return dict(contract_id="contract."+name, subject=dict(kind="semantic_id", id="subject."+name),
                quantity_kind="quantity.energy", unit_policy=dict(mode="dimension_only", unit=None),
                value_form=dict(kind=form, component_count=3 if form=="vector" else None,
                                tensor_rank=2 if form=="tensor" else None,
                                tensor_shape=[3, 3] if form=="tensor" else []),
                reference_semantics=dict(kind="linear", reference_subject=None, period_binary64=None),
                frame_id="frame.world")


def fnv(data, h=1469598103934665603):
    for b in data:
        h=((h ^ b)*1099511628211) & ((1<<64)-1)
    return h


def number(x):
    return x.to_bytes(8, "little")


def text(x):
    b=x.encode(); return number(len(b))+b


def run(root, probe):
    count=0
    def write(name, value):
        path=root/name
        path.write_text(json.dumps(value, indent=2)+"\n")
        return path
    qs=[quantity("energy", [2,1,-2,0,0,0,0,0], "mass*length^2/time^2"),
        quantity("torque", [2,1,-2,0,0,0,0,0], "mass*length^2/time^2"),
        quantity("position", [1,0,0,0,0,0,0,0], "length"),
        quantity("displacement", [1,0,0,0,0,0,0,0], "length"),
        quantity("temperature", [0,0,0,0,1,0,0,0], "temperature"),
        quantity("temperature_delta", [0,0,0,0,1,0,0,0], "temperature"),
        quantity("angle", [0]*8, "1"), quantity("ratio", [0]*8, "1"),
        quantity("unprofiled", [0]*8, "1")]
    catalog=dict(schema="fisiCs.physical_contract_catalog", schema_version=1,
                 catalog_id="test.catalog", quantity_kinds=qs,
                 contracts=[contract("scalar"), contract("vector", "vector"), contract("tensor", "tensor")])
    policy=dict(schema="fisiCs.physical_semantics_policy", schema_version=1,
                policy_id="test.physics", catalog_paths=["catalog with spaces.json"],
                linear_kinds=["quantity.energy", "quantity.torque", "quantity.angle", "quantity.ratio"],
                affine_pairs=[dict(absolute_kind="quantity.position", difference_kind="quantity.displacement"),
                              dict(absolute_kind="quantity.temperature", difference_kind="quantity.temperature_delta")],
                origins=[dict(id="origin.world", absolute_kind="quantity.position"),
                         dict(id="origin.local", absolute_kind="quantity.position"),
                         dict(id="origin.kelvin", absolute_kind="quantity.temperature")])
    cat=write("catalog with spaces.json", catalog); good=write("good.json", policy)
    def call(target, mode="show", *args):
        nonlocal count
        p=subprocess.run([str(probe), str(good), str(target), mode, *map(str,args)], text=True, capture_output=True, timeout=120)
        if p.returncode:
            raise AssertionError(f"{target.name} {mode} {args}: {p.returncode}\n{p.stdout}\n{p.stderr}")
        count+=1; return json.loads(p.stdout)
    def bad_policy(name, change, code=4303):
        value=copy.deepcopy(policy); change(value)
        call(write(name+".json", value), "fail", code)
    def bad_catalog(name, change, code=4302):
        value=copy.deepcopy(catalog); change(value)
        bad=write(name+".catalog.json", value)
        p=copy.deepcopy(policy); p["catalog_paths"]=[bad.name]
        call(write(name+".json", p), "fail", code)
    result=call(good)
    assert result["policy_hash"]==f"{fnv(good.read_bytes()):016x}" and result["policy_bytes"]==good.stat().st_size
    assert result["catalog_receipts"]==[dict(hash=f"{fnv(cat.read_bytes()):016x}", bytes=cat.stat().st_size)]
    definitions=[]
    for q in sorted(qs, key=lambda x:x["id"]):
        definitions.append(f'{fnv(text("fisiCs.physical_quantity.v1")+text(q["id"])+text(q["dimension_system"])+bytes(x%256 for x in q["dimension_vector"])+text(q["dimension_text"])):016x}')
    assert result["definitions"]==definitions
    # An independent reference encoding checks the complete semantic fingerprint.
    ordered=sorted(qs, key=lambda x:x["id"]); roles={x:(1, "") for x in policy["linear_kinds"]}
    for pair in policy["affine_pairs"]:
        a,b=pair["absolute_kind"],pair["difference_kind"];roles[a]=(2,b);roles[b]=(3,a)
    payload=text("fisiCs.physical_semantics_policy.v1")+text(policy["policy_id"])+number(int(result["registry"],16))+number(len(qs))
    for q,fp in zip(ordered, definitions):
        role,pair=roles.get(q["id"],(0,""));payload+=number(int(fp,16))+number(role)+text(pair)
    payload+=number(len(policy["origins"]))
    for o in sorted(policy["origins"],key=lambda x:x["id"]):payload+=text(o["id"])+text(o["absolute_kind"])
    assert result["fingerprint"]==f"{fnv(payload):016x}"
    for key in ["linear_kinds", "affine_pairs", "origins"]:
        p=copy.deepcopy(policy);p[key].reverse()
        assert call(write("permuted-"+key+".json",p))["fingerprint"]==result["fingerprint"]
    p=copy.deepcopy(policy);p["policy_id"]="changed.physics"
    assert call(write("changed.json",p))["fingerprint"]!=result["fingerprint"]
    # Catalog ordering and identical definitions merge without changing identities.
    extra=copy.deepcopy(catalog);extra["contracts"]=[];extra["quantity_kinds"].reverse()
    write("extra.json",extra)
    p=copy.deepcopy(policy);p["catalog_paths"].append("extra.json")
    multi=write("multi.json",p); assert call(multi)["fingerprint"]==result["fingerprint"]
    p["catalog_paths"].reverse(); assert call(write("multi-reverse.json",p))["fingerprint"]==result["fingerprint"]
    compact=root/"compact.json";compact.write_text(json.dumps(policy,separators=(",",":")))
    compact_result=call(compact);assert compact_result["fingerprint"]==result["fingerprint"] and compact_result["policy_hash"]!=result["policy_hash"]
    changed=copy.deepcopy(catalog);changed["quantity_kinds"][0]["dimension_text"]="length^2*mass/time^2"
    write("equivalent.json",changed);p=copy.deepcopy(policy);p["catalog_paths"]=["equivalent.json"]
    assert call(write("equivalent-policy.json",p))["fingerprint"]!=result["fingerprint"]
    for key in policy:
        bad_policy("missing-"+key,lambda p,k=key:p.pop(k),4301)
    bad_policy("extra-field",lambda p:p.update(extra=True),4301)
    bad_policy("version",lambda p:p.update(schema_version=2),4301)
    bad_policy("bad-id",lambda p:p.update(policy_id="bad id"),4301)
    bad_policy("paths-empty",lambda p:p.update(catalog_paths=[]),4301)
    bad_policy("paths-type",lambda p:p.update(catalog_paths=[42]),4301)
    bad_policy("missing-file",lambda p:p.update(catalog_paths=["does-not-exist.json"]),4306)
    bad_policy("directory",lambda p:p.update(catalog_paths=["."]),4306)
    bad_policy("duplicate-path",lambda p:p["catalog_paths"].append("./catalog with spaces.json"),4301)
    (root/"alias.json").symlink_to(cat.name)
    bad_policy("alias-path",lambda p:p["catalog_paths"].append("alias.json"),4301)
    for key in ["linear_kinds","affine_pairs","origins"]:
        bad_policy("wrong-array-"+key,lambda p,k=key:p.update({k:None}))
    bad_policy("unknown-linear",lambda p:p["linear_kinds"].append("quantity.missing"))
    bad_policy("duplicate-linear",lambda p:p["linear_kinds"].append("quantity.energy"))
    bad_policy("role-overlap",lambda p:p["linear_kinds"].append("quantity.position"))
    bad_policy("unknown-pair",lambda p:p["affine_pairs"][0].update(difference_kind="quantity.missing"))
    bad_policy("same-pair",lambda p:p["affine_pairs"][0].update(difference_kind="quantity.position"))
    bad_policy("bad-dim-pair",lambda p:p["affine_pairs"][0].update(difference_kind="quantity.unprofiled"))
    bad_policy("overlap-pair",lambda p:p["affine_pairs"].append(p["affine_pairs"][0]))
    bad_policy("pair-extra",lambda p:p["affine_pairs"][0].update(extra=True))
    bad_policy("duplicate-origin",lambda p:p["origins"].append(p["origins"][0]))
    bad_policy("origin-difference",lambda p:p["origins"][0].update(absolute_kind="quantity.displacement"))
    bad_policy("origin-unknown",lambda p:p["origins"][0].update(absolute_kind="quantity.missing"))
    bad_policy("origin-bad-id",lambda p:p["origins"][0].update(id="not valid"))
    bad_policy("origin-extra",lambda p:p["origins"][0].update(extra=True))
    raw=root/"duplicate-key.json";raw.write_text(good.read_text().replace('"schema_version": 1','"schema_version": 1, "schema_version": 1'))
    call(raw,"fail",4301)
    raw=root/"malformed.json";raw.write_text('{"schema":');call(raw,"fail",4301)
    for key in catalog:
        bad_catalog("missing-catalog-"+key,lambda c,k=key:c.pop(k))
    bad_catalog("catalog-version",lambda c:c.update(schema_version=2))
    bad_catalog("catalog-extra",lambda c:c.update(extra=True))
    bad_catalog("quantity-extra",lambda c:c["quantity_kinds"][0].update(reference_kind="absolute"))
    bad_catalog("vector-short",lambda c:c["quantity_kinds"][0].update(dimension_vector=[1]))
    bad_catalog("vector-range",lambda c:c["quantity_kinds"][0]["dimension_vector"].__setitem__(0,128))
    bad_catalog("vector-fraction",lambda c:c["quantity_kinds"][0]["dimension_vector"].__setitem__(0,0.5))
    bad_catalog("vector-text",lambda c:c["quantity_kinds"][0].update(dimension_text="length"))
    bad_catalog("dimension-system",lambda c:c["quantity_kinds"][0].update(dimension_system="new"))
    def conflict(c):
        q=copy.deepcopy(c["quantity_kinds"][0]);q["dimension_text"]="length";q["dimension_vector"]=[1,0,0,0,0,0,0,0];c["quantity_kinds"].append(q)
    bad_catalog("conflicting-kind",conflict)
    bad_catalog("contract-unknown",lambda c:c["contracts"][0].update(quantity_kind="quantity.missing"))
    bad_catalog("contract-duplicate",lambda c:c["contracts"].append(c["contracts"][0]))
    bad_catalog("subject-duplicate",lambda c:c["contracts"][1].update(subject=c["contracts"][0]["subject"]))
    bad_catalog("wrong-unit",lambda c:c["contracts"][0].update(unit_policy=dict(mode="exact_unit",unit="m")))
    bad_catalog("vector-components",lambda c:c["contracts"][1]["value_form"].update(component_count=None))
    bad_catalog("tensor-rank",lambda c:c["contracts"][2]["value_form"].update(tensor_rank=1))
    bad_catalog("bad-reference",lambda c:c["contracts"][0]["reference_semantics"].update(kind="relative"))
    bad_catalog("dim8-reserved",lambda c:c["quantity_kinds"][0]["dimension_vector"].__setitem__(7,1))
    def compact_conflict(c):
        c["quantity_kinds"] += copy.deepcopy(c["quantity_kinds"])
        conflict(c)
    bad_catalog("merge-then-conflict",compact_conflict)
    bad_catalog("semantic-text-conflict",lambda c:c["quantity_kinds"].append(dict(c["quantity_kinds"][0],dimension_text="length^2*mass/time^2")))
    changed=copy.deepcopy(catalog)
    changed["contracts"][0]["unit_policy"]=dict(mode="exact_unit",unit="joule")
    changed["contracts"][0]["reference_semantics"]=dict(kind="relative",reference_subject="subject.external",period_binary64=None)
    changed["contracts"][1]["reference_semantics"]=dict(kind="cyclic",reference_subject=None,period_binary64="6.283185307179586")
    write("declared.catalog.json",changed);p=copy.deepcopy(policy);p["catalog_paths"]=["declared.catalog.json"]
    declared=write("declared.json",p)
    assert call(declared)["fingerprint"]==result["fingerprint"]
    declared_sweep=call(declared,"sweep")
    empty=copy.deepcopy(catalog);empty.update(quantity_kinds=[],contracts=[]);write("empty.catalog.json",empty)
    p=copy.deepcopy(policy);p.update(catalog_paths=["empty.catalog.json"],linear_kinds=[],affine_pairs=[],origins=[])
    assert call(write("empty.json",p))["kinds"]==0
    for option,n in [("bytes",max(good.stat().st_size,cat.stat().st_size)),
                     ("total",good.stat().st_size+cat.stat().st_size),("kinds",9),("contracts",3),("origins",3)]:
        call(good,"allow",option,n)
    call(multi,"allow","catalogs",2)
    # Every owned allocation site is failed while replacing an existing model.
    sweep=call(good,"sweep");multi_sweep=call(multi,"sweep")
    for option,n in [("bytes",100),("total",good.stat().st_size+cat.stat().st_size-1),
                     ("kinds",8),("contracts",2),("catalogs",1),("origins",2)]:
        call(multi if option=="catalogs" else good,"limit",option,n)
    # Actual upper boundary: handle 65536 must remain representable; +1 fails.
    large=copy.deepcopy(catalog);large["contracts"]=[]
    large["quantity_kinds"]=[quantity(f"k{i:05d}",[0]*8,"1") for i in range(65536)]
    write("large.catalog.json",large)
    lp=copy.deepcopy(policy);lp.update(catalog_paths=["large.catalog.json"],linear_kinds=[],affine_pairs=[],origins=[])
    large_policy=write("large.json",lp); assert call(large_policy)["kinds"]==65536
    large["quantity_kinds"].append(quantity("overflow",[0]*8,"1"));write("large.catalog.json",large)
    call(large_policy,"fail",4304)
    summary=dict(cases=count, allocation_sites=sweep["allocations"], merged_allocation_sites=multi_sweep["allocations"], declared_allocation_sites=declared_sweep["allocations"],
                 fingerprint=result["fingerprint"], maximum_kind_handle=65536)
    (root/"results.json").write_text(json.dumps(summary,indent=2)+"\n")
    print(json.dumps(summary))


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument("--probe",type=pathlib.Path,required=True);p.add_argument("--out",type=pathlib.Path)
    a=p.parse_args();probe=a.probe.resolve()
    if a.out:
        a.out.mkdir(parents=True,exist_ok=False);run(a.out.resolve(),probe)
    else:
        with tempfile.TemporaryDirectory(prefix="fisics-physical-policy-") as tmp:run(pathlib.Path(tmp),probe)


if __name__=="__main__":main()
