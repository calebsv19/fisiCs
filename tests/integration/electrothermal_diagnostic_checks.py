"""Qualification of real accepted-prefix reports, partial support and failure meaning."""
import copy
from pathlib import Path
import sys
from electrothermal_binding import encoded, load, require, sha
from electrothermal_diagnostics import inspect_bundle, publish, summary_markdown, verify_report
from electrothermal_runtime import refusal


def qualify(a, out, cases, heat, run, failures):
    out.mkdir(); reports = []; full_count = partial_count = 0
    for case in cases:
        bundle = a.out/case['bundle']; name = bundle.parent.name
        report = inspect_bundle(bundle,a.native,heat)
        require(report['reported_through_step'] == 100 and len(report['samples']) == 101, 'full diagnostic denominator')
        require(report['samples'][0]['accepted_allocated_j'] == 0 and report['samples'][-1]['pending_allocated_at_sample_j'] == 0, 'initial/final prefix meaning')
        require(report['material']['initial_total_stored_j'] > report['horizon']['allocated_j'], 'nonzero initial storage control')
        path = bundle.parent/'diagnostics.json'; publish(report,path,bundle)
        case.update(diagnostics=str(path.relative_to(a.out)),diagnostics_sha256=sha(path))
        part = inspect_bundle(bundle.parent/'partial',a.native,heat)
        require(part['available_completed_steps'] == 37 and len(part['samples']) == 38, 'partial diagnostic denominator')
        require(part['samples'] == report['samples'][:38], 'partial/native accepted history differs')
        require(part['samples'][-1]['pending_allocated_at_sample_j'] > 0, 'future allocated heat already consumed')
        part_path = bundle.parent/'partial-diagnostics.json'
        publish(part,part_path,bundle.parent/'partial')
        case.update(partial_diagnostics=str(part_path.relative_to(a.out)),partial_diagnostics_sha256=sha(part_path))
        historical = inspect_bundle(bundle,a.native,heat,37)
        require(historical['available_completed_steps'] == 100 and historical['reported_through_step'] == 37 and
                historical['samples'] == part['samples'], 'sample versus global completed steps')
        zero = inspect_bundle(bundle,a.native,heat,0)
        require(len(zero['samples']) == 1 and zero['reported_through_step'] == 0 and zero['samples'][0]['accepted_allocated_j'] == 0, 'zero-prefix selection')
        publish(zero,bundle.parent/'zero-diagnostics.json',bundle)
        refusal(name+'-future-prefix',lambda: inspect_bundle(bundle.parent/'partial',a.native,heat,38),failures)
        reports += [(name,report),(name+' at37',part)]; full_count += len(report['samples']); partial_count += len(part['samples'])
    selected = load(a.out/cases[0]['diagnostics']); bundle = a.out/cases[0]['bundle']
    early = inspect_bundle(bundle,a.native,heat,37)
    for label, edit in [
        ('diagnostic-full-horizon-as-consumed', lambda v: v['samples'][-1].update(accepted_allocated_j=v['horizon']['allocated_j'])),
        ('diagnostic-future-pending-zero', lambda v: v['samples'][-1].update(pending_allocated_at_sample_j=0)),
        ('diagnostic-initial-storage-as-heat', lambda v: v['samples'][0].update(accepted_allocated_j=v['material']['initial_total_stored_j'])),
        ('diagnostic-reference-zero', lambda v: v['material']['cells'][0].update(material_min_k=0)),
        ('diagnostic-future-time', lambda v: v['samples'][1].update(time_num=100)),
        ('diagnostic-missing-prefix', lambda v: v['samples'].pop(10)),
        ('diagnostic-unallocated-deposited', lambda v: v['horizon'].update(allocated_j=v['horizon']['allocated_j']+v['horizon']['unallocated_not_deposited_j'])),
        ('diagnostic-packet-laundering', lambda v: v['samples'][0].update(cell_exchange_j=v['horizon']['allocated_j']))]:
        bad = copy.deepcopy(early);edit(bad)
        refusal(label,lambda: verify_report(bad,bundle,a.native,heat),failures)
    loss_case = next(c for c in cases if c['bundle'] == 'runtime/boundary-loss/full')
    loss = load(a.out/loss_case['diagnostics']); end = loss['samples'][-1]
    require(end['boundary_net_into_j'] < 0 and end['native_sink_j'] == 0 and
            end['native_stored_change_j'] < end['accepted_allocated_j'], 'real boundary-loss convention')
    require(end['boundary_left_into_j'] < 0 and end['boundary_right_into_j'] < 0, 'two inward face signs')
    bad = copy.deepcopy(loss);bad['samples'][-1]['boundary_net_into_j'] *= -1
    refusal('diagnostic-boundary-sign',lambda: verify_report(bad,a.out/loss_case['bundle'],a.native,heat),failures)
    bad = copy.deepcopy(loss);bad['samples'][-1]['native_sink_j'] = -end['boundary_net_into_j'];bad['samples'][-1]['boundary_net_into_j'] = 0
    refusal('diagnostic-boundary-as-sink',lambda: verify_report(bad,a.out/loss_case['bundle'],a.native,heat),failures)
    summary = summary_markdown(reports);(out/'summary.md').write_text(summary)
    # CLI proof uses actual frozen bundles: create, readback, no overwrite and no future publication.
    cli = Path(__file__).with_name('run_electrothermal_diagnostics.py')
    args = [sys.executable,'-B',cli,'--host-root',a.host_root,'--native',a.native,'--bundle',bundle]
    report_path = out/'cli-prefix.json'
    run('diagnostic-cli-create',[*args,'--through','37','--output',report_path])
    before = report_path.read_bytes()
    run('diagnostic-cli-read',[*args,'--verify-report',report_path])
    run('diagnostic-cli-no-overwrite',[*args,'--through','0','--output',report_path],expected=1)
    require(report_path.read_bytes() == before, 'CLI replaced existing diagnostic')
    future = out/'future-must-not-exist.json'
    run('diagnostic-cli-future',[*args,'--through','101','--output',future],expected=1)
    require(not future.exists(), 'future diagnostic was published')
    original = {str(p):sha(p) for p in bundle.rglob('*') if p.is_file()}
    refusal('diagnostic-input-publication',lambda: publish(selected,bundle/'must-not-exist.json',bundle),failures)
    require(all(sha(p) == h for p,h in original.items()) and not (bundle/'must-not-exist.json').exists(), 'diagnostic changed input')
    return dict(passed=True,full_history_samples=full_count,partial_history_samples=partial_count,
                cases=len(cases),summary=str((out/'summary.md').relative_to(a.out)))
