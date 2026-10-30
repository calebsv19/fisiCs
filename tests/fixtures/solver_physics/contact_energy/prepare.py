"""Generate contained candidates; no writes to Particle or immutable donor."""
from pathlib import Path
import hashlib,json
ROOT=Path(__file__).resolve().parent
SELECTED=['src/contact/coulomb_step.c','src/contact/three_response.c','src/contact/three_step.c']
DONOR='src/physics/rigid3d/rigid_convex_manifold_solver_3d.c'
def prepare(particle,donor,out):
 out.mkdir(parents=True,exist_ok=False);h=ROOT/'contact_energy.h';(out/h.name).write_bytes(h.read_bytes());pins={}
 replacements={
 SELECTED[0]:[('.5*p->mass*p->v[k]*p->v[k]','ce_kinetic(p->mass,p->v[k])'),('.2*p->mass*p->radius*p->radius*p->omega[k]*p->omega[k]','ce_spin(p->mass,p->radius,p->omega[k])')],
 SELECTED[1]:[('p->mass*(s->bodies[i].velocity_mps.x-p->v[0])','ce_momentum_delta(p->mass,s->bodies[i].velocity_mps.x,p->v[0])'),('double loss=before-after','double loss=ce_loss(before,after)')],
 SELECTED[2]:[('after-w->step.energy_before+w->step.dissipation','ce_balance(after,w->step.energy_before,w->step.dissipation)')],
 DONOR:[('(current_closing - point->target_closing_speed) * point->effective_mass','ce_impulse(current_closing,point->target_closing_speed,point->effective_mass)'),('(closing - x->target_closing_speed) * x->effective_mass','ce_impulse(closing,x->target_closing_speed,x->effective_mass)')]}
 for name,pairs in replacements.items():
  original=(donor if name==DONOR else particle)/name;text=original.read_text();before=text
  for old,new in pairs:
   assert text.count(old)==1,(name,old);text=text.replace(old,new)
  dest=out/Path(name).name;dest.write_text('#include "contact_energy.h"\n'+text)
  pins[str(original)]={'sha256':hashlib.sha256(before.encode()).hexdigest(),'candidate':str(dest),'candidate_sha256':hashlib.sha256(dest.read_bytes()).hexdigest(),'replacements':pairs}
 (out/'transforms.json').write_text(json.dumps(pins,indent=2)+'\n');return pins
