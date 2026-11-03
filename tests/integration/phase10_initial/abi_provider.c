#include "core_thermal_material_rod.h"
#include "core_structural_frame_motion.h"
CoreThermalRodState p102_roundtrip(CoreThermalRodState value,CoreStructuralFrameMotionSettings settings){value.energy_j[0]+=settings.added_mass_kg[1];value.energy_j[1]+=settings.dt_s;return value;}
