// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>

typedef unsigned char edu58_u8;
typedef unsigned int edu58_u32;
typedef unsigned long long edu58_u64;

extern edu58_u64 edu22_queue_entry_valid(const edu58_u8 *bytes);

static void edu58_write16(edu58_u8 *bytes, unsigned int offset,
                          unsigned int value) {
    bytes[offset] = (edu58_u8)value;
    bytes[offset + 1U] = (edu58_u8)(value >> 8U);
}

static void edu58_write32(edu58_u8 *bytes, unsigned int offset,
                          edu58_u32 value) {
    unsigned int index = 0U;
    while (index < 4U) {
        bytes[offset + index] = (edu58_u8)(value >> (index * 8U));
        index = index + 1U;
    }
}

static void edu58_write64(edu58_u8 *bytes, unsigned int offset,
                          edu58_u64 value) {
    unsigned int index = 0U;
    while (index < 8U) {
        bytes[offset + index] = (edu58_u8)(value >> (index * 8U));
        index = index + 1U;
    }
}

static edu58_u32 edu58_fnv(const edu58_u8 *bytes, unsigned int count) {
    edu58_u32 value = 0x811C9DC5U;
    unsigned int index = 0U;
    while (index < count) {
        value = (value ^ bytes[index]) * 0x01000193U;
        index = index + 1U;
    }
    return value;
}

static void edu58_seal(edu58_u8 *entry) {
    edu58_u32 checksum = edu58_fnv(entry, 508U);
    edu58_write32(entry, 508U, checksum);
}

static void edu58_event(edu58_u8 *entry, unsigned int index,
                        unsigned int sequence, unsigned int kind,
                        unsigned int state, unsigned int reason,
                        edu58_u64 value) {
    unsigned int offset = 104U + index * 32U;
    edu58_write16(entry, offset, sequence);
    edu58_write16(entry, offset + 2U, 1U);
    edu58_write16(entry, offset + 4U, kind);
    edu58_write64(entry, offset + 8U, (edu58_u64)sequence * 10ULL);
    edu58_write32(entry, offset + 16U, state);
    edu58_write32(entry, offset + 20U, reason);
    edu58_write64(entry, offset + 24U, value);
}

static void edu58_valid_dynamic_complete(edu58_u8 *entry) {
    unsigned int index = 0U;
    const edu58_u64 result = 0x91A2B3C4D5E6F701ULL;
    while (index < 512U) {
        entry[index] = 0U;
        index = index + 1U;
    }
    entry[0] = 'E';
    entry[1] = 'D';
    entry[2] = 'U';
    entry[3] = '1';
    entry[4] = '5';
    entry[5] = 'J';
    edu58_write32(entry, 8U, 11U);
    edu58_write32(entry, 12U, 7U);
    edu58_write64(entry, 16U, 9ULL);
    edu58_write32(entry, 24U, 3U);
    edu58_write32(entry, 28U, 16U);
    edu58_write32(entry, 32U, 1U);
    edu58_write64(entry, 40U, result);
    edu58_write64(entry, 48U, 0x1E3C373BAF48FAF7ULL);
    edu58_write64(entry, 64U, result);
    edu58_write16(entry, 56U, 5U);
    edu58_write16(entry, 58U, 32U);
    entry[60] = 3U;
    entry[61] = 12U;
    edu58_event(entry, 0U, 1U, 1U, 1U, 0U, 0ULL);
    edu58_event(entry, 1U, 2U, 21U, 1U, 0U, result);
    edu58_event(entry, 2U, 3U, 11U, 3U, 0U, result);
    edu58_write16(entry, 488U, 1U);
    edu58_write16(entry, 492U, 1U);
    edu58_write32(entry, 498U, 1U);
    edu58_write16(entry, 502U, 104U);
    edu58_write32(entry, 504U, 1U);
    edu58_seal(entry);
}

int main(void) {
    edu58_u8 entry[512];
    edu58_u64 valid;
    edu58_u64 duplicate_result;
    edu58_u64 stale_state;
    edu58_u64 mismatched_terminal;

    edu58_valid_dynamic_complete(entry);
    valid = edu22_queue_entry_valid(entry);
    edu58_event(entry, 2U, 3U, 21U, 1U, 0U, 0x91A2B3C4D5E6F701ULL);
    edu58_seal(entry);
    duplicate_result = edu22_queue_entry_valid(entry);
    edu58_valid_dynamic_complete(entry);
    edu58_event(entry, 1U, 2U, 21U, 3U, 0U, 0x91A2B3C4D5E6F701ULL);
    edu58_seal(entry);
    stale_state = edu22_queue_entry_valid(entry);
    edu58_valid_dynamic_complete(entry);
    edu58_event(entry, 2U, 3U, 11U, 3U, 0U, 0x11ULL);
    edu58_seal(entry);
    mismatched_terminal = edu22_queue_entry_valid(entry);
    if (valid != 0ULL || duplicate_result != 1ULL || stale_state != 1ULL ||
        mismatched_terminal != 1ULL) {
        puts("OS-DEV-EDU58 canary=result=FAIL");
        return 1;
    }
    puts("OS-DEV-EDU58 source=queue_kernel.c policy=dynamic-result-validation result=PASS");
    return 0;
}
