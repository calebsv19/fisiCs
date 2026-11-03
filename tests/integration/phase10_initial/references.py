"""Independent scalar physical expectations for the fixed initial batch."""
from decimal import Decimal as D,localcontext
import math,sys

def dec(x):return D.from_float(float(x))
class Checks:
 def __init__(self):self.n=0;self.errors={}
 def check(self,ok,why):
  self.n+=1
  if not ok:raise AssertionError(why)
 def close(self,x,y,bound,field):
  self.check(math.isfinite(float(x)) and math.isfinite(float(y)),('nonfinite',field,x,y));error=abs(D(str(x))-D(str(y)));b=D(str(bound));self.errors[field]=max(self.errors.get(field,0),float(error));self.check(error<=b,(field,str(x),str(y),'bound',str(bound)))
 def relative(self,x,y,bound,field):
  self.check(y!=0,('zero relative reference',field));self.close(dec(x)/y,D(1),bound,field)

def thermal(header,rows,case,spec=None):
 c=Checks();kind=header['kind'];variant=header['variant'];params=case['parameters'];n=spec['ticks'] if spec else case['grid']['ticks'][0];dt=spec['h'] if spec else float(case['grid']['h_s'][0]['numerator']/case['grid']['h_s'][0]['denominator']);c.check(header['dt']==dt and header['ticks']==n and len(rows)==n,'thermal grid');c.check(header['count']==(1 if kind==0 else 2),'cell count')
 with localcontext() as ctx:
  ctx.prec=80;h=dec(dt);count=header['count'];power=D(0);injected=D(0);max_continuum=0
  if kind==0:C=[D(24)];e=[D(240)];P=dec(params['power_w_profiles'][variant]);g=D(0)
  elif kind==1:
   C=[D(100),D(200)];ts=params['temperature_profiles_k'][variant];e=[dec(float(C[i]*dec(ts[i]))) for i in range(2)];g=D(10)
  else:
   C=[D(50),D(100 if variant==7 else 50)];g=D(8)/3 if variant==7 else D(2);amp=dec(params['amplitudes'][variant]);e=[amp if variant==6 else D(0),D(0)];injected=sum(e)
  c.check(all(dec(header['initial_e'][i])==e[i] for i in range(count)),'independent initial energy');initial=sum(e);previous=e[:];zero_source_acc=D(0);round_residual=D(0)
  for tick,row in enumerate(rows,1):
   c.check(row['tick']==tick and row['time']==tick*dt,'accepted thermal clock');p=[D(0)]*count
   if kind==0:p=[P]
   if kind==2:
    p=[D(0) if variant in (4,6) else amp,amp if variant==4 else D(0)]
    if variant==5:p[0]=amp if tick-1<round(1.28/dt) else D(0) if tick-1<round(2.56/dt) else -amp/4
   c.check(all(row['p'][i]==float(p[i]) for i in range(count)),'authored power schedule')
   if count==1:e=[e[0]+h*p[0]];face=D(0)
   else:
    a=h*g/C[0];b=h*g/C[1];x=e[0]+h*p[0];y=e[1]+h*p[1];e=[((1+b)*x+b*y)/(1+a+b),(a*x+(1+a)*y)/(1+a+b)];face=h*g*(e[0]/C[0]-e[1]/C[1])
   injected+=h*sum(p);zero_source_acc+=dec(row['source'])-dec(row['sink']);round_residual+=dec(row['residual'])
   c.check(row['boundary']==[0,0] and row['face'][0]==0 and (count==1 or row['face'][2]==0),'insulated boundary')
   if kind==2:
    for i in range(2):c.check(row['e'][i]>0 and 300<=row['t'][i]<=1200,'positive faint state/range');c.relative(row['e'][i],e[i],D('1e-9'),'cell_relative')
    c.relative(sum(map(dec,row['e'][:count])),injected,D('1e-10'),'total_relative');c.check(2<=row['iterations']<=96,'material iteration admission')
    c.close(dec(row['face'][1]),face,abs(face)*D('1e-9'),'face_j')
   else:
    temperature_bound=D('1e-8')
    if spec and spec['role']=='long':temperature_bound+=n*D(64)*dec(sys.float_info.epsilon)*max(D(1),*(abs(x/C[i]) for i,x in enumerate(e)))
    for i in range(count):c.close(dec(row['t'][i]),e[i]/C[i],temperature_bound,'temperature_k')
    c.close(row['residual'],0,D('1e-10'),'step_residual_j');total_bound=D('1e-9')
    if spec and spec['role']=='long':total_bound+=n*D(2048)*dec(sys.float_info.epsilon)*(1+abs(initial)+abs(zero_source_acc))
    c.close(sum(map(dec,row['e'][:count]))-initial-zero_source_acc,0,total_bound,'total_residual_j')
    if kind==1:c.close(dec(row['face'][1]),face,D('1e-10'),'face_j')
   c.close(dec(row['source'])-dec(row['sink']),h*sum(p),D('1e-10') if kind!=2 else abs(h*sum(p))*D('1e-10'),'external_j')
   if kind!=2:c.close(dec(row['stored']),sum(e)-sum(previous),D('1e-10'),'stored_change_j')
   previous=e[:]
  return dict(passed=True,checks=c.n,maximum_errors=c.errors,reference_digits=80,steps=n,final_reference_energy_j=list(map(str,e)))

def dynamic(header,rows,case,spec=None):
 c=Checks();p=case['parameters'];h=header['dt'];alpha=p['alpha_per_s'];kind=header['kind'];variant=header['variant'];expected_h=spec['h'] if spec else .0008;expected_n=spec['ticks'] if spec else 200;c.check(h==expected_h and header['ticks']==expected_n and len(rows)==expected_n+1,'bound dynamic grid')
 # Reconstruct stiffness/inertia exclusively from authored geometry/material/inertia.
 k=p['youngs_modulus_pa']*sum(p['fiber_area_m2'])/p['length_m'];mass=p['density_kg_m3']*sum(p['fiber_area_m2'])*p['length_m']/2+p['added_mass_kg_node1'];held=p['held_force_n'];release=p['released_force_n_profiles'][variant] if kind!=2 else 0;amplitude=p['additional_force_n_profiles'][variant] if kind==2 else 0;omega=.8*math.sqrt(k/mass)
 c.check(header['held']==held and header['released']==release and header['amplitude']==amplitude and header['alpha']==alpha and header['omega']==omega,'dynamic supplied values')
 with localcontext() as ctx:
  ctx.prec=80;H=dec(h);K=dec(k);M=dec(mass);A=dec(alpha);u=(dec(held)+dec(release))/K;v=D(0);u0=u;I0=K*u*u/2;force0=dec(held);damp=work=D(0);observed_damp=observed_work=0.;old=None;initial_dynamic=dec(release)**2/(2*K);previous_energy=None;continuum_max=0.
  # Independent 2x2 trapezoidal system, not the native Newmark predictor/Newton update.
  l00=D(1);l01=-H/2;l10=H*K/(2*M);l11=1+H*A/2;det=l00*l11-l01*l10
  for n,row in enumerate(rows):
   f=dec(held+amplitude*math.sin(omega*(n*h))) if kind==2 else dec(held)
   if n:
    r0=u+H*v/2;r1=-H*K*u/(2*M)+(1-H*A/2)*v+H*(force0+f)/(2*M);un=(l11*r0-l01*r1)/det;vn=(-l10*r0+l00*r1)/det
    damp+=H*A*M*((v+vn)/2)**2;work+=(force0+f)*(un-u)/2;u,v=un,vn
   c.check(row['tick']==n and row['time']==n*h,'native tick/time');c.check(row['plastic']==0,'elastic-only material');c.check(row['force'][3]==float(f),'total endpoint force');c.check(all(row['u'][i]==0 and row['v'][i]==0 for i in range(6) if i!=3),'fixed coordinates')
   c.close(dec(row['u'][3]),u,D('1e-12'),'u_m');c.close(dec(row['v'][3]),v,D('1e-8'),'v_m_s')
   c.close(row['I0'],I0,D('1e-12')+abs(I0)*D('1e-8'),'initial_work_j');c.close(row['I'],K*u*u/2,D('1e-12')+abs(K*u*u/2)*D('1e-8'),'material_work_j')
   for key,reference in [('K',M*v*v/2),('D',damp),('W',work)]:c.close(row[key],reference,D('1e-12')+abs(reference)*D('1e-8'),key+'_independent_j')
   # A separate actual-state observer checks K/D/W even if native phase is wrong.
   if old:
    observed_damp+=h*alpha*mass*((old['v'][3]+row['v'][3])/2)**2;observed_work+=(old['force'][3]+row['force'][3])*(row['u'][3]-old['u'][3])/2
   for key,expected in [('K',mass*row['v'][3]**2/2),('D',observed_damp),('W',observed_work)]:c.close(row[key],expected,1e-12+1e-8*abs(expected),key+'_observer_j')
   terms=[row['I'],row['I0'],row['K'],row['D'],row['W']];r=terms[0]-terms[1]+terms[2]+terms[3]-terms[4];c.close(row['R'],r,64*sys.float_info.epsilon*max(sum(map(abs,terms)),sys.float_info.min),'ledger_reconstruction_j')
   c.check(abs(row['residual_n'])<=1e-4 and abs(row['residual_nm'])<=1e-4,'force/moment equilibrium')
   energy=row['I']+row['K'];
   if kind==0:c.close(energy,float(I0),float(initial_dynamic)*1e-7,'free_energy_drift_j')
   if kind==1 and previous_energy is not None:c.check(energy<=previous_energy+1e-12+1e-7*float(initial_dynamic),'passive monotonicity')
   if kind==2:c.close(row['R'],0,1e-12+.005*max(abs(row['I']-row['I0']),abs(row['W']),abs(row['K']),abs(row['D'])),'forced_balance_j')
   previous_energy=energy;old=row;force0=f
  return dict(passed=True,checks=c.n,maximum_errors=c.errors,reference_digits=80,steps=expected_n,stiffness_n_m=k,mass_kg=mass,continuum_refinement_assessed=False)
