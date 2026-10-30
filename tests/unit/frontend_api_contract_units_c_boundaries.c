// SPDX-License-Identifier: Apache-2.0
#include "Extensions/Units/units_coverage_policy.h"
#include "AST/ast_node.h"
#include <stdio.h>
int main(void) {
    ASTNode sites[2]={{0},{0}};FisicsCoverageRow rows[2]={{0},{0}};
    FisicsCoverageReport report={0};report.rows=rows;report.rowCount=2;report.enumerationComplete=true;
    for(int i=0;i<2;++i) {
        rows[i].assessment.obligation.site=&sites[i];
        rows[i].assessment.obligation.expected.hasDimension=true;
        rows[i].assessment.obligation.expected.dimension=fisics_dim_zero();
        rows[i].assessment.obligation.expected.dimension.e[0]=1;
        rows[i].assessment.obligation.expected.provenanceComplete=true;
        rows[i].assessment.obligation.actual=rows[i].assessment.obligation.expected;
    }
    rows[0].cInterface=true;
    rows[0].assessment.obligation.actual.dimension.e[0]=0;
    rows[0].assessment.obligation.actual.dimension.e[2]=1;
    FisicsCoveragePolicy p={true,true,false};
    FisicsCoverageDecision d=fisics_units_coverage_decide(&report,p);
    if(d.accepted || d.blocked!=1 || d.acceptedTrust)return 1;
    rows[0].assessment.obligation.expected.hasDimension=false;
    rows[0].assessment.obligation.actual.hasDimension=false;
    d=fisics_units_coverage_decide(&report,p);
    if(!d.accepted || d.acceptedTrust!=1 || d.checked!=1)return 2;
    p.allowDeclaredBoundaries=false;d=fisics_units_coverage_decide(&report,p);
    if(d.accepted || d.blocked!=1)return 3;
    p.allowDeclaredBoundaries=true;rows[0].cInterface=false;d=fisics_units_coverage_decide(&report,p);
    if(d.accepted || d.blocked!=1)return 4;
    puts("C-boundary policy: PASS (4 controls; contradiction precedence and explicit trust)");return 0;
}
