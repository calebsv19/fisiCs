// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "Compiler/pipeline.h"
#include "Extensions/Units/units_coverage_policy.h"
bool fisics_units_coverage_write(const char* path,const char* other,const char* source,
    const CompileResult* result,const FisicsCoverageReport* report,const CompileOptions* options,const FisicsCoverageDecision* decision,bool json);

bool fisics_units_coverage_destination(const char* path,const char* other,const char* source,
    const CompileResult* result);
