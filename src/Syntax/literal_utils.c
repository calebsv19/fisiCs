#include "literal_utils.h"

#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static bool hexValue(char c, uint32_t* out) {
    if (c >= '0' && c <= '9') { *out = (uint32_t)(c - '0'); return true; }
    if (c >= 'a' && c <= 'f') { *out = (uint32_t)(c - 'a' + 10); return true; }
    if (c >= 'A' && c <= 'F') { *out = (uint32_t)(c - 'A' + 10); return true; }
    return false;
}

static bool decodeHexSequence(const char* raw,
                              size_t len,
                              size_t start,
                              size_t minDigits,
                              size_t maxDigits,
                              size_t* consumed,
                              uint32_t* outVal) {
    uint32_t value = 0;
    size_t i = start;
    size_t count = 0;
    while (i < len && count < maxDigits) {
        uint32_t v = 0;
        if (!hexValue(raw[i], &v)) break;
        value = (value << 4) | v;
        ++i;
        ++count;
    }
    if (count < minDigits) {
        return false;
    }
    *consumed = count;
    *outVal = value;
    return true;
}

static bool decodeEscape(const char* raw,
                         size_t len,
                         size_t start,
                         size_t* consumed,
                         uint32_t* outVal) {
    if (start >= len) return false;
    char esc = raw[start];
    switch (esc) {
        case 'n':  *outVal = '\n'; *consumed = 1; return true;
        case 't':  *outVal = '\t'; *consumed = 1; return true;
        case 'r':  *outVal = '\r'; *consumed = 1; return true;
        case 'b':  *outVal = '\b'; *consumed = 1; return true;
        case 'f':  *outVal = '\f'; *consumed = 1; return true;
        case 'a':  *outVal = '\a'; *consumed = 1; return true;
        case 'v':  *outVal = '\v'; *consumed = 1; return true;
        case '\\': *outVal = '\\'; *consumed = 1; return true;
        case '\'': *outVal = '\''; *consumed = 1; return true;
        case '"':  *outVal = '\"'; *consumed = 1; return true;
        case '?':  *outVal = '?'; *consumed = 1; return true;
        case 'x': {
            size_t used = 0;
            uint32_t value = 0;
            if (!decodeHexSequence(raw, len, start + 1, 1, SIZE_MAX, &used, &value)) {
                return false;
            }
            *consumed = 1 + used;
            *outVal = value;
            return true;
        }
        case 'u': {
            size_t used = 0;
            uint32_t value = 0;
            if (!decodeHexSequence(raw, len, start + 1, 4, 4, &used, &value)) {
                return false;
            }
            *consumed = 1 + used;
            *outVal = value;
            return true;
        }
        case 'U': {
            size_t used = 0;
            uint32_t value = 0;
            if (!decodeHexSequence(raw, len, start + 1, 8, 8, &used, &value)) {
                return false;
            }
            *consumed = 1 + used;
            *outVal = value;
            return true;
        }
        default:
            if (esc >= '0' && esc <= '7') {
                size_t i = start;
                uint32_t value = 0;
                size_t count = 0;
                while (i < len && count < 3) {
                    char d = raw[i];
                    if (d < '0' || d > '7') break;
                    value = (value << 3) + (uint32_t)(d - '0');
                    ++i;
                    ++count;
                }
                *consumed = count;
                *outVal = value;
                return true;
            }
            *outVal = (unsigned char)esc;
            *consumed = 1;
            return true;
    }
}

LiteralDecodeResult decode_c_string_literal(const char* raw,
                                            int charBitWidth,
                                            char** outBytes,
                                            size_t* outByteLen) {
    LiteralDecodeResult res = {.ok = true, .overflow = false, .length = 0};
    if (outBytes) *outBytes = NULL;
    if (outByteLen) *outByteLen = 0;
    if (!raw) return res;

    size_t len = strlen(raw);
    uint64_t maxVal;
    if (charBitWidth <= 0) {
        maxVal = UINT64_MAX;
    } else if (charBitWidth >= 63) {
        maxVal = UINT64_MAX;
    } else {
        maxVal = (1ULL << charBitWidth) - 1ULL;
    }

    size_t bufCap = 0;
    size_t bufLen = 0;
    char* buf = NULL;
    if (outBytes && charBitWidth <= 8) {
        bufCap = len + 1;
        buf = (char*)malloc(bufCap);
    }

    for (size_t i = 0; i < len;) {
        uint32_t value = 0;
        size_t consumed = 1;
        if (raw[i] == '\\') {
            if (!decodeEscape(raw, len, i + 1, &consumed, &value)) {
                res.ok = false;
                break;
            }
            consumed += 1; // account for the leading '\'
        } else {
            value = (unsigned char)raw[i];
        }

        if (value > 0x10FFFF || (value >= 0xD800 && value <= 0xDFFF)) {
            res.ok = false;
        }
        if ((uint64_t)value > maxVal) {
            res.overflow = true;
        }
        if (buf && bufLen + 1 < bufCap) {
            buf[bufLen++] = (char)(value & 0xFF);
        }
        res.length += 1;
        i += consumed;
    }

    if (buf) {
        if (bufLen >= bufCap) {
            // Should not happen, but guard against overflow.
            char* grown = (char*)realloc(buf, bufLen + 1);
            if (grown) {
                buf = grown;
            }
        }
        buf[bufLen] = '\0';
        if (outBytes) *outBytes = buf;
        if (outByteLen) *outByteLen = bufLen;
    }

    return res;
}

LiteralDecodeResult decode_c_string_literal_wide(const char* raw,
                                                 int charBitWidth,
                                                 uint32_t** outCodepoints,
                                                 size_t* outLen) {
    LiteralDecodeResult res = {.ok = true, .overflow = false, .length = 0};
    if (outCodepoints) *outCodepoints = NULL;
    if (outLen) *outLen = 0;
    if (!raw) return res;

    size_t len = strlen(raw);
    uint64_t maxVal;
    if (charBitWidth <= 0) {
        maxVal = UINT64_MAX;
    } else if (charBitWidth >= 63) {
        maxVal = UINT64_MAX;
    } else {
        maxVal = (1ULL << charBitWidth) - 1ULL;
    }

    uint32_t* buf = NULL;
    if (outCodepoints) {
        buf = (uint32_t*)malloc((len + 1) * sizeof(uint32_t));
        if (!buf) {
            res.ok = false;
            return res;
        }
    }

    size_t outIdx = 0;
    for (size_t i = 0; i < len;) {
        uint32_t value = 0;
        size_t consumed = 1;
        if (raw[i] == '\\') {
            if (!decodeEscape(raw, len, i + 1, &consumed, &value)) {
                res.ok = false;
                break;
            }
            consumed += 1; // account for the leading '\'
        } else {
            value = (unsigned char)raw[i];
        }

        if (value > 0x10FFFF || (value >= 0xD800 && value <= 0xDFFF)) {
            res.ok = false;
        }
        if ((uint64_t)value > maxVal) {
            res.overflow = true;
        }
        if (buf) {
            buf[outIdx++] = value;
        }
        res.length += 1;
        i += consumed;
    }

    if (buf) {
        if (outCodepoints) *outCodepoints = buf;
        if (outLen) *outLen = outIdx;
    }

    return res;
}
