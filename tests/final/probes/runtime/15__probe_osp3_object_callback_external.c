typedef unsigned long osp3_u64;
typedef osp3_u64 (*osp3_policy_callback)(osp3_u64 value, osp3_u64 key);

extern osp3_u64 osp3_external_policy(osp3_u64 value, osp3_u64 key);

osp3_u64 osp3_object_callback_external(
    osp3_u64 value,
    osp3_u64 key,
    osp3_policy_callback callback
) {
    osp3_u64 local = callback(value, key);
    return local ^ osp3_external_policy(key, value);
}
