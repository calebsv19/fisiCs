#include "core_thermal_material_rod.h"
#include "core_structural_frame_motion.h"
#include <float.h>
#include <stddef.h>
#include <stdio.h>
extern CoreThermalRodState p102_roundtrip(CoreThermalRodState,CoreStructuralFrameMotionSettings);
int main(void){CoreThermalRodState s={0};s.count=2;s.energy_j[0]=3;s.energy_j[1]=4;CoreStructuralFrameMotionSettings p={0};p.dt_s=.25;p.added_mass_kg[1]=1000;CoreThermalRodState r=p102_roundtrip(s,p);
 if(sizeof(double)!=8||DBL_EPSILON!=0x1p-52||DBL_MIN!=0x1p-1022||r.count!=2||r.energy_j[0]!=1003||r.energy_j[1]!=4.25||s.energy_j[0]!=3)return 2;
 printf("%zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu\n",sizeof(CoreThermalRodState),sizeof(CoreThermalRodStep),sizeof(CoreThermalMaterialRod),sizeof(CoreThermalMaterialRodStep),sizeof(CoreStructuralFrameMotionSettings),sizeof(CoreStructuralFrameMotionState),offsetof(CoreThermalRodState,energy_j),offsetof(CoreThermalRodStep,face_transfer_j),offsetof(CoreStructuralFrameMotionState,velocity),offsetof(CoreStructuralFrameMotionState,initial_internal_work_j),offsetof(CoreStructuralFrameMotionSettings,added_mass_kg),offsetof(CoreThermalMaterialRod,material));return 0;}
