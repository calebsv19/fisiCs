#include <stdio.h>

#include "15__probe_units_runtime_multitu_gpu_feedback_ladder_shared.h"

int main(void) {
    ProbeGpuFeedbackLadderState a = probe_gpu_feedback_ladder_seed(30.0, 430.0);
    ProbeGpuFeedbackLadderState b = probe_gpu_feedback_ladder_seed(35.0, 590.0);
    ProbeGpuFeedbackLadderState c = probe_gpu_feedback_ladder_seed(40.5, 760.0);
    ProbeGpuFeedbackLadderState d = probe_gpu_feedback_ladder_seed(46.0, 920.0);

    a = probe_gpu_feedback_ladder_push(a, 0, 6.3);
    b = probe_gpu_feedback_ladder_push(b, 1, 7.2);
    c = probe_gpu_feedback_ladder_push(c, 2, 8.5);
    d = probe_gpu_feedback_ladder_push(d, 3, 9.4);

    ProbeGpuFeedbackLadderState left =
        probe_gpu_feedback_ladder_merge(a, b, 3.6, 560.0);
    ProbeGpuFeedbackLadderState right =
        probe_gpu_feedback_ladder_merge(c, d, 5.8, 910.0);
    ProbeGpuFeedbackLadderState total =
        probe_gpu_feedback_ladder_merge(left, right, 7.1, 1040.0);

    printf("%.6f %.6f %.6f %.6f\n",
           total.distance_m,
           total.energy_j,
           total.charge_ah,
           probe_gpu_feedback_ladder_score(total, 1890.0, 205.0));
    return 0;
}
