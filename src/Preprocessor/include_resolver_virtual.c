// SPDX-License-Identifier: Apache-2.0

#include "Preprocessor/include_resolver_internal.h"

#include <stdlib.h>
#include <string.h>

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
