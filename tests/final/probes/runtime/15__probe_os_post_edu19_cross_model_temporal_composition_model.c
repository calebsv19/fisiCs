/*
 * Hardware-blind compiler probe joining independently-owned temporal,
 * Queue-v2, Wire-v7, and scheduler models.  This is not an OS-P4 metadata
 * schema and does not claim OS execution or persistence authority.
 */
typedef unsigned char cm_u8;
typedef unsigned long cm_word;
typedef unsigned long long cm_u64;

extern cm_u64 edu22_queue_entry_valid(const cm_u8 *bytes);
extern cm_u64 edu31_wire_v7_valid(const cm_u8 *bytes);
extern void osp2_scheduler_init(void);
extern cm_word osp2_scheduler_choose(
    cm_word current, cm_word reason, cm_word now, cm_word sleep_ticks);
extern cm_word osp2_scheduler_state(cm_word task);
extern cm_word osp2_scheduler_switch_count(void);
extern cm_word osp2_scheduler_yield_count(void);
extern cm_word osp2_scheduler_preemption_count(void);
extern int edu44_stale_evidence_cross_generation_rejected(
    const cm_u8 *old_lanes, const cm_u8 *new_lanes,
    const cm_u8 *entries, const cm_u8 *before_contexts,
    const cm_u8 *restarted_contexts,
    const cm_u8 *old_mailbox, const cm_u8 *new_mailbox,
    unsigned int old_ack_generation, cm_u64 old_ack_request);

enum {
    CM_QUEUE_OK = 0,
    CM_WIRE_OK = 0,
    CM_SCHED_RUNNABLE = 1,
    CM_SCHED_YIELD = 1,
    CM_SCHED_TIMER = 3
};

int edu45_scheduler_handoff_valid(cm_word first, cm_word second) {
    cm_word selected;
    osp2_scheduler_init();
    selected = osp2_scheduler_choose(0UL, CM_SCHED_YIELD, 7UL, 0UL);
    if (selected != first) return 0;
    selected = osp2_scheduler_choose(1UL, CM_SCHED_TIMER, 8UL, 0UL);
    return selected == second &&
           osp2_scheduler_state(0UL) == CM_SCHED_RUNNABLE &&
           osp2_scheduler_state(1UL) == CM_SCHED_RUNNABLE &&
           osp2_scheduler_switch_count() == 2UL &&
           osp2_scheduler_yield_count() == 1UL &&
           osp2_scheduler_preemption_count() == 1UL;
}

int edu45_cross_model_temporal_admission(
    const cm_u8 *queue_entry, const cm_u8 *wire_frame,
    const cm_u8 *old_lanes, const cm_u8 *new_lanes,
    const cm_u8 *entries, const cm_u8 *before_contexts,
    const cm_u8 *restarted_contexts,
    const cm_u8 *old_mailbox, const cm_u8 *new_mailbox,
    unsigned int old_ack_generation, cm_u64 old_ack_request) {
    return edu22_queue_entry_valid(queue_entry) == CM_QUEUE_OK &&
           edu31_wire_v7_valid(wire_frame) == CM_WIRE_OK &&
           edu45_scheduler_handoff_valid(1UL, 0UL) &&
           edu44_stale_evidence_cross_generation_rejected(
               old_lanes, new_lanes, entries, before_contexts,
               restarted_contexts, old_mailbox, new_mailbox,
               old_ack_generation, old_ack_request);
}
