typedef unsigned long osp3_u64;

osp3_u64 osp3_object_switch_transition(
    osp3_u64 state,
    osp3_u64 action,
    osp3_u64 epoch
) {
    switch (state) {
        case 0:
            return action == 1 ? epoch + 1 : epoch;
        case 1:
            return action == 2 ? epoch + 3 : epoch + 2;
        case 2:
            return action == 3 ? epoch + 5 : epoch + 4;
        case 7:
            return action == 0 ? epoch + 7 : epoch + 6;
        case 31:
            return action == 4 ? epoch + 11 : epoch + 9;
        case 255:
            return action == 5 ? epoch + 17 : epoch + 13;
        default:
            return 0;
    }
}
