// SPDX-License-Identifier: Apache-2.0

#include "Preprocessor/include_resolver_internal.h"

#include <stdlib.h>
#include <string.h>

static char* ir_join_virtual_lines(const char* const* lines, size_t count) {
    size_t total = 1;
    size_t offset = 0;
    char* out = NULL;

    for (size_t i = 0; i < count; ++i) {
        total += strlen(lines[i]) + 1;
    }

    out = (char*)malloc(total);
    if (!out) {
        return NULL;
    }

    for (size_t i = 0; i < count; ++i) {
        size_t len = strlen(lines[i]);
        memcpy(out + offset, lines[i], len);
        offset += len;
        out[offset++] = 10;
    }
    out[offset] = '\0';
    return out;
}

const IncludeFile* ir_try_virtual_audio_toolbox(IncludeResolver* resolver, const char* name) {
    static const char* kAudioToolboxName = "AudioToolbox/AudioToolbox.h";
    static const char* kVirtualPath = "<fisics>/AudioToolbox/AudioToolbox.h";
    static const char* kShim =
        "#ifndef FISICS_AUDIO_TOOLBOX_SHIM_H\n"
        "#define FISICS_AUDIO_TOOLBOX_SHIM_H\n"
        "typedef int OSStatus;\n"
        "typedef unsigned int UInt32;\n"
        "typedef unsigned char UInt8;\n"
        "typedef long CFIndex;\n"
        "typedef long long SInt64;\n"
        "typedef double Float64;\n"
        "typedef void* CFURLRef;\n"
        "typedef void* CFStringRef;\n"
        "typedef struct OpaqueExtAudioFile* ExtAudioFileRef;\n"
        "enum { noErr = 0 };\n"
        "enum { kExtAudioFileProperty_FileDataFormat = 1 };\n"
        "enum { kExtAudioFileProperty_FileLengthFrames = 2 };\n"
        "enum { kExtAudioFileProperty_ClientDataFormat = 3 };\n"
        "enum { kAudioFormatLinearPCM = 1819304813 };\n"
        "enum { kAudioFormatFlagIsFloat = 1u << 0 };\n"
        "enum { kAudioFormatFlagIsPacked = 1u << 3 };\n"
        "enum { kAudioFormatFlagsNativeEndian = 0u };\n"
        "typedef struct AudioStreamBasicDescription {\n"
        "  Float64 mSampleRate;\n"
        "  UInt32 mFormatID;\n"
        "  UInt32 mFormatFlags;\n"
        "  UInt32 mBytesPerPacket;\n"
        "  UInt32 mFramesPerPacket;\n"
        "  UInt32 mBytesPerFrame;\n"
        "  UInt32 mChannelsPerFrame;\n"
        "  UInt32 mBitsPerChannel;\n"
        "} AudioStreamBasicDescription;\n"
        "typedef struct AudioBuffer {\n"
        "  UInt32 mNumberChannels;\n"
        "  UInt32 mDataByteSize;\n"
        "  void* mData;\n"
        "} AudioBuffer;\n"
        "typedef struct AudioBufferList {\n"
        "  UInt32 mNumberBuffers;\n"
        "  AudioBuffer mBuffers[1];\n"
        "} AudioBufferList;\n"
        "CFURLRef CFURLCreateFromFileSystemRepresentation(void* alloc,\n"
        "                                                  const UInt8* buffer,\n"
        "                                                  CFIndex len,\n"
        "                                                  int isDir);\n"
        "void CFRelease(void* cf);\n"
        "const char* CFStringGetCStringPtr(CFStringRef str, unsigned int enc);\n"
        "OSStatus ExtAudioFileOpenURL(CFURLRef url, ExtAudioFileRef* outExtAudioFile);\n"
        "OSStatus ExtAudioFileGetProperty(ExtAudioFileRef inExtAudioFile,\n"
        "                                 UInt32 inPropertyID,\n"
        "                                 UInt32* ioPropertyDataSize,\n"
        "                                 void* outPropertyData);\n"
        "OSStatus ExtAudioFileSetProperty(ExtAudioFileRef inExtAudioFile,\n"
        "                                 UInt32 inPropertyID,\n"
        "                                 UInt32 inPropertyDataSize,\n"
        "                                 const void* inPropertyData);\n"
        "OSStatus ExtAudioFileRead(ExtAudioFileRef inExtAudioFile,\n"
        "                          UInt32* ioNumberFrames,\n"
        "                          AudioBufferList* ioData);\n"
        "OSStatus ExtAudioFileDispose(ExtAudioFileRef inExtAudioFile);\n"
        "#endif\n";

    if (!resolver || !name || strcmp(name, kAudioToolboxName) != 0) {
        return NULL;
    }
    const IncludeFile* cached = ir_lookup_exact_path(resolver, kVirtualPath);
    if (cached) return cached;

    IncludeFile file = {0};
    file.path = ir_strdup(kVirtualPath);
    file.contents = ir_strdup(kShim);
    file.lexedTokens = NULL;
    file.lexedTokenCount = 0;
    file.lexedTokenCapacity = 0;
    file.canonicalPath = ir_strdup(kVirtualPath);
    file.mtime = 0;
    file.pragmaOnce = true;
    file.includedOnce = false;
    file.origin = INCLUDE_SEARCH_RAW;
    file.originIndex = (size_t)-1;
    if (!file.path || !file.contents || !file.canonicalPath) {
        free(file.path);
        free(file.contents);
        free(file.canonicalPath);
        return NULL;
    }
    if (!ir_append_file(resolver, file)) {
        free(file.path);
        free(file.contents);
        free(file.canonicalPath);
        return NULL;
    }
    return &resolver->files[resolver->count - 1];
}

const IncludeFile* ir_try_virtual_tgmath(IncludeResolver* resolver, const char* name) {
    static const char* kTgmathName = "tgmath.h";
    static const char* kVirtualPath = "<fisics>/tgmath.h";
    static const char* kTgmathLines[] = {
        "#ifndef __TGMATH_H",
        "#define __TGMATH_H",
        "",
        "/*",
        " * fisiCs virtual tgmath shim.",
        " *",
        " * Apple tgmath.h depends on Clang-specific overloadable helpers and",
        " * GNU __typeof__ dispatch that fisiCs does not model yet. This shim",
        " * closes the supported real-number dispatch surface directly so system",
        " * header ingress remains usable in both baseline and shadow paths.",
        " */",
        "",
        "#include <math.h>",
        "",
        "#define __FISICS_TGMATH_UNARY_REAL(__name) static inline double __fisics_tg_##__name(double __x) { return __name(__x); }",
        "#define __FISICS_TGMATH_BINARY_REAL(__name) static inline double __fisics_tg_##__name(double __x, double __y) { return __name(__x, __y); }",
        "",
        "#ifdef acos",
        "#undef acos",
        "#endif",
        "__FISICS_TGMATH_UNARY_REAL(acos)",
        "#define acos(__x) __fisics_tg_acos((double)(__x))",
        "",
        "#ifdef asin",
        "#undef asin",
        "#endif",
        "__FISICS_TGMATH_UNARY_REAL(asin)",
        "#define asin(__x) __fisics_tg_asin((double)(__x))",
        "",
        "#ifdef atan",
        "#undef atan",
        "#endif",
        "__FISICS_TGMATH_UNARY_REAL(atan)",
        "#define atan(__x) __fisics_tg_atan((double)(__x))",
        "",
        "#ifdef ceil",
        "#undef ceil",
        "#endif",
        "__FISICS_TGMATH_UNARY_REAL(ceil)",
        "#define ceil(__x) __fisics_tg_ceil((double)(__x))",
        "",
        "#ifdef cos",
        "#undef cos",
        "#endif",
        "__FISICS_TGMATH_UNARY_REAL(cos)",
        "#define cos(__x) __fisics_tg_cos((double)(__x))",
        "",
        "#ifdef cosh",
        "#undef cosh",
        "#endif",
        "__FISICS_TGMATH_UNARY_REAL(cosh)",
        "#define cosh(__x) __fisics_tg_cosh((double)(__x))",
        "",
        "#ifdef exp",
        "#undef exp",
        "#endif",
        "__FISICS_TGMATH_UNARY_REAL(exp)",
        "#define exp(__x) __fisics_tg_exp((double)(__x))",
        "",
        "#ifdef fabs",
        "#undef fabs",
        "#endif",
        "__FISICS_TGMATH_UNARY_REAL(fabs)",
        "#define fabs(__x) __fisics_tg_fabs((double)(__x))",
        "",
        "#ifdef floor",
        "#undef floor",
        "#endif",
        "__FISICS_TGMATH_UNARY_REAL(floor)",
        "#define floor(__x) __fisics_tg_floor((double)(__x))",
        "",
        "#ifdef log",
        "#undef log",
        "#endif",
        "__FISICS_TGMATH_UNARY_REAL(log)",
        "#define log(__x) __fisics_tg_log((double)(__x))",
        "",
        "#ifdef log10",
        "#undef log10",
        "#endif",
        "__FISICS_TGMATH_UNARY_REAL(log10)",
        "#define log10(__x) __fisics_tg_log10((double)(__x))",
        "",
        "#ifdef sin",
        "#undef sin",
        "#endif",
        "__FISICS_TGMATH_UNARY_REAL(sin)",
        "#define sin(__x) __fisics_tg_sin((double)(__x))",
        "",
        "#ifdef sinh",
        "#undef sinh",
        "#endif",
        "__FISICS_TGMATH_UNARY_REAL(sinh)",
        "#define sinh(__x) __fisics_tg_sinh((double)(__x))",
        "",
        "#ifdef sqrt",
        "#undef sqrt",
        "#endif",
        "__FISICS_TGMATH_UNARY_REAL(sqrt)",
        "#define sqrt(__x) __fisics_tg_sqrt((double)(__x))",
        "",
        "#ifdef tan",
        "#undef tan",
        "#endif",
        "__FISICS_TGMATH_UNARY_REAL(tan)",
        "#define tan(__x) __fisics_tg_tan((double)(__x))",
        "",
        "#ifdef tanh",
        "#undef tanh",
        "#endif",
        "__FISICS_TGMATH_UNARY_REAL(tanh)",
        "#define tanh(__x) __fisics_tg_tanh((double)(__x))",
        "",
        "#ifdef atan2",
        "#undef atan2",
        "#endif",
        "__FISICS_TGMATH_BINARY_REAL(atan2)",
        "#define atan2(__x, __y) __fisics_tg_atan2((double)(__x), (double)(__y))",
        "",
        "#ifdef fmax",
        "#undef fmax",
        "#endif",
        "__FISICS_TGMATH_BINARY_REAL(fmax)",
        "#define fmax(__x, __y) __fisics_tg_fmax((double)(__x), (double)(__y))",
        "",
        "#ifdef fmin",
        "#undef fmin",
        "#endif",
        "__FISICS_TGMATH_BINARY_REAL(fmin)",
        "#define fmin(__x, __y) __fisics_tg_fmin((double)(__x), (double)(__y))",
        "",
        "#ifdef fmod",
        "#undef fmod",
        "#endif",
        "__FISICS_TGMATH_BINARY_REAL(fmod)",
        "#define fmod(__x, __y) __fisics_tg_fmod((double)(__x), (double)(__y))",
        "",
        "#ifdef hypot",
        "#undef hypot",
        "#endif",
        "__FISICS_TGMATH_BINARY_REAL(hypot)",
        "#define hypot(__x, __y) __fisics_tg_hypot((double)(__x), (double)(__y))",
        "",
        "#ifdef pow",
        "#undef pow",
        "#endif",
        "__FISICS_TGMATH_BINARY_REAL(pow)",
        "#define pow(__x, __y) __fisics_tg_pow((double)(__x), (double)(__y))",
        "",
        "#undef __FISICS_TGMATH_UNARY_REAL",
        "#undef __FISICS_TGMATH_BINARY_REAL",
        "",
        "#endif"
    };

    if (!resolver || !name || strcmp(name, kTgmathName) != 0) {
        return NULL;
    }
    const IncludeFile* cached = ir_lookup_exact_path(resolver, kVirtualPath);
    if (cached) return cached;

    IncludeFile file = {0};
    file.path = ir_strdup(kVirtualPath);
    file.contents = ir_join_virtual_lines(
        kTgmathLines, sizeof(kTgmathLines) / sizeof(kTgmathLines[0]));
    file.lexedTokens = NULL;
    file.lexedTokenCount = 0;
    file.lexedTokenCapacity = 0;
    file.canonicalPath = ir_strdup(kVirtualPath);
    file.mtime = 0;
    file.pragmaOnce = true;
    file.includedOnce = false;
    file.origin = INCLUDE_SEARCH_RAW;
    file.originIndex = (size_t)-1;
    if (!file.path || !file.contents || !file.canonicalPath) {
        free(file.path);
        free(file.contents);
        free(file.canonicalPath);
        return NULL;
    }
    if (!ir_append_file(resolver, file)) {
        free(file.path);
        free(file.contents);
        free(file.canonicalPath);
        return NULL;
    }
    return &resolver->files[resolver->count - 1];
}
