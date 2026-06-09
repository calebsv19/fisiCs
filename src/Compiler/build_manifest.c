// SPDX-License-Identifier: Apache-2.0

#include "Compiler/build_manifest.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

typedef struct ManifestParser {
    const char* text;
    size_t pos;
    size_t len;
    FisicsBuildManifestDiagnostic* diag;
} ManifestParser;

static void manifest_diag(FisicsBuildManifestDiagnostic* diag, const char* fmt, ...) {
    if (!diag || diag->message[0]) return;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(diag->message, sizeof(diag->message), fmt, ap);
    va_end(ap);
}

static char* manifest_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char* out = (char*)malloc(len + 1u);
    if (!out) return NULL;
    memcpy(out, s, len + 1u);
    return out;
}

static char* manifest_strndup(const char* s, size_t len) {
    char* out = (char*)malloc(len + 1u);
    if (!out) return NULL;
    memcpy(out, s, len);
    out[len] = '\0';
    return out;
}

static void parser_skip_ws(ManifestParser* p) {
    while (p->pos < p->len && isspace((unsigned char)p->text[p->pos])) p->pos++;
}

static bool parser_take(ManifestParser* p, char c) {
    parser_skip_ws(p);
    if (p->pos < p->len && p->text[p->pos] == c) {
        p->pos++;
        return true;
    }
    return false;
}

static bool parser_expect(ManifestParser* p, char c, const char* label) {
    if (parser_take(p, c)) return true;
    manifest_diag(p->diag, "expected '%c' while parsing %s", c, label);
    return false;
}

static bool list_append(FisicsBuildManifestStringList* list, char* value) {
    char** next = (char**)realloc(list->items, sizeof(char*) * (list->count + 1u));
    if (!next) return false;
    list->items = next;
    list->items[list->count++] = value;
    return true;
}

static char* parser_string(ManifestParser* p, const char* label) {
    parser_skip_ws(p);
    if (p->pos >= p->len || p->text[p->pos] != '"') {
        manifest_diag(p->diag, "expected string for %s", label);
        return NULL;
    }
    p->pos++;
    char* out = NULL;
    size_t outLen = 0;
    size_t outCap = 0;
    while (p->pos < p->len) {
        unsigned char ch = (unsigned char)p->text[p->pos++];
        if (ch == '"') {
            char* done = (char*)realloc(out, outLen + 1u);
            if (!done) {
                free(out);
                manifest_diag(p->diag, "out of memory while parsing %s", label);
                return NULL;
            }
            done[outLen] = '\0';
            return done;
        }
        if (ch == '\\') {
            if (p->pos >= p->len) {
                free(out);
                manifest_diag(p->diag, "unterminated escape while parsing %s", label);
                return NULL;
            }
            ch = (unsigned char)p->text[p->pos++];
            switch (ch) {
                case '"': case '\\': case '/': break;
                case 'n': ch = '\n'; break;
                case 'r': ch = '\r'; break;
                case 't': ch = '\t'; break;
                default:
                    free(out);
                    manifest_diag(p->diag, "unsupported escape while parsing %s", label);
                    return NULL;
            }
        }
        if (outLen + 1u > outCap) {
            size_t nextCap = outCap ? outCap * 2u : 32u;
            char* grown = (char*)realloc(out, nextCap);
            if (!grown) {
                free(out);
                manifest_diag(p->diag, "out of memory while parsing %s", label);
                return NULL;
            }
            out = grown;
            outCap = nextCap;
        }
        out[outLen++] = (char)ch;
    }
    free(out);
    manifest_diag(p->diag, "unterminated string for %s", label);
    return NULL;
}

static bool parser_int(ManifestParser* p, int* out, const char* label) {
    parser_skip_ws(p);
    int sign = 1;
    if (p->pos < p->len && p->text[p->pos] == '-') {
        sign = -1;
        p->pos++;
    }
    if (p->pos >= p->len || !isdigit((unsigned char)p->text[p->pos])) {
        manifest_diag(p->diag, "expected integer for %s", label);
        return false;
    }
    int value = 0;
    while (p->pos < p->len && isdigit((unsigned char)p->text[p->pos])) {
        value = value * 10 + (p->text[p->pos++] - '0');
    }
    *out = value * sign;
    return true;
}

static bool parser_string_array(ManifestParser* p,
                                FisicsBuildManifestStringList* out,
                                const char* label) {
    if (!parser_expect(p, '[', label)) return false;
    parser_skip_ws(p);
    if (parser_take(p, ']')) return true;
    while (p->pos < p->len) {
        char* item = parser_string(p, label);
        if (!item) return false;
        if (!list_append(out, item)) {
            free(item);
            manifest_diag(p->diag, "out of memory while parsing %s", label);
            return false;
        }
        parser_skip_ws(p);
        if (parser_take(p, ']')) return true;
        if (!parser_expect(p, ',', label)) return false;
    }
    manifest_diag(p->diag, "unterminated array for %s", label);
    return false;
}

static bool parse_defaults(ManifestParser* p, FisicsBuildManifestDefaults* out) {
    if (!parser_expect(p, '{', "defaults")) return false;
    parser_skip_ws(p);
    if (parser_take(p, '}')) return true;
    while (p->pos < p->len) {
        char* key = parser_string(p, "defaults key");
        if (!key) return false;
        bool ok = parser_expect(p, ':', "defaults");
        if (ok && strcmp(key, "standard") == 0) {
            free(out->standard);
            out->standard = parser_string(p, "defaults.standard");
            ok = out->standard != NULL;
        } else if (ok && strcmp(key, "include_dirs") == 0) {
            ok = parser_string_array(p, &out->includeDirs, "defaults.include_dirs");
        } else if (ok && strcmp(key, "defines") == 0) {
            ok = parser_string_array(p, &out->defines, "defaults.defines");
        } else if (ok && strcmp(key, "overlays") == 0) {
            ok = parser_string_array(p, &out->overlays, "defaults.overlays");
        } else if (ok) {
            manifest_diag(p->diag, "unknown defaults field '%s'", key);
            ok = false;
        }
        free(key);
        if (!ok) return false;
        parser_skip_ws(p);
        if (parser_take(p, '}')) return true;
        if (!parser_expect(p, ',', "defaults")) return false;
    }
    manifest_diag(p->diag, "unterminated defaults object");
    return false;
}

static bool parse_translation_unit(ManifestParser* p,
                                   FisicsBuildManifestTranslationUnit* out) {
    if (!parser_expect(p, '{', "translation unit")) return false;
    parser_skip_ws(p);
    if (parser_take(p, '}')) {
        manifest_diag(p->diag, "translation unit missing source");
        return false;
    }
    while (p->pos < p->len) {
        char* key = parser_string(p, "translation unit key");
        if (!key) return false;
        bool ok = parser_expect(p, ':', "translation unit");
        if (ok && strcmp(key, "source") == 0) {
            free(out->source);
            out->source = parser_string(p, "translation_units.source");
            ok = out->source != NULL;
        } else if (ok && strcmp(key, "object") == 0) {
            free(out->object);
            out->object = parser_string(p, "translation_units.object");
            ok = out->object != NULL;
        } else if (ok) {
            manifest_diag(p->diag, "unknown translation unit field '%s'", key);
            ok = false;
        }
        free(key);
        if (!ok) return false;
        parser_skip_ws(p);
        if (parser_take(p, '}')) {
            if (!out->source || !out->source[0]) {
                manifest_diag(p->diag, "translation unit missing source");
                return false;
            }
            return true;
        }
        if (!parser_expect(p, ',', "translation unit")) return false;
    }
    manifest_diag(p->diag, "unterminated translation unit object");
    return false;
}

static bool append_translation_unit(FisicsBuildManifest* manifest,
                                    FisicsBuildManifestTranslationUnit* unit) {
    FisicsBuildManifestTranslationUnit* next =
        (FisicsBuildManifestTranslationUnit*)realloc(
            manifest->translationUnits,
            sizeof(FisicsBuildManifestTranslationUnit) * (manifest->translationUnitCount + 1u));
    if (!next) return false;
    manifest->translationUnits = next;
    manifest->translationUnits[manifest->translationUnitCount++] = *unit;
    memset(unit, 0, sizeof(*unit));
    return true;
}

static bool parse_translation_units(ManifestParser* p, FisicsBuildManifest* out) {
    if (!parser_expect(p, '[', "translation_units")) return false;
    parser_skip_ws(p);
    if (parser_take(p, ']')) {
        manifest_diag(p->diag, "translation_units must contain at least one item");
        return false;
    }
    while (p->pos < p->len) {
        FisicsBuildManifestTranslationUnit unit = {0};
        if (!parse_translation_unit(p, &unit)) {
            free(unit.source);
            free(unit.object);
            return false;
        }
        if (!append_translation_unit(out, &unit)) {
            free(unit.source);
            free(unit.object);
            manifest_diag(p->diag, "out of memory while parsing translation_units");
            return false;
        }
        parser_skip_ws(p);
        if (parser_take(p, ']')) return true;
        if (!parser_expect(p, ',', "translation_units")) return false;
    }
    manifest_diag(p->diag, "unterminated translation_units array");
    return false;
}

static bool parse_link(ManifestParser* p, FisicsBuildManifestLink* out) {
    if (!parser_expect(p, '{', "link")) return false;
    parser_skip_ws(p);
    if (parser_take(p, '}')) return true;
    while (p->pos < p->len) {
        char* key = parser_string(p, "link key");
        if (!key) return false;
        bool ok = parser_expect(p, ':', "link");
        if (ok && strcmp(key, "output") == 0) {
            free(out->output);
            out->output = parser_string(p, "link.output");
            ok = out->output != NULL;
        } else if (ok && strcmp(key, "libraries") == 0) {
            ok = parser_string_array(p, &out->libraries, "link.libraries");
        } else if (ok && strcmp(key, "library_dirs") == 0) {
            ok = parser_string_array(p, &out->libraryDirs, "link.library_dirs");
        } else if (ok && strcmp(key, "args") == 0) {
            ok = parser_string_array(p, &out->args, "link.args");
        } else if (ok) {
            manifest_diag(p->diag, "unknown link field '%s'", key);
            ok = false;
        }
        free(key);
        if (!ok) return false;
        parser_skip_ws(p);
        if (parser_take(p, '}')) return true;
        if (!parser_expect(p, ',', "link")) return false;
    }
    manifest_diag(p->diag, "unterminated link object");
    return false;
}

static bool parse_manifest_root(ManifestParser* p, FisicsBuildManifest* out) {
    if (!parser_expect(p, '{', "manifest")) return false;
    parser_skip_ws(p);
    if (parser_take(p, '}')) {
        manifest_diag(p->diag, "manifest missing translation_units");
        return false;
    }
    out->version = -1;
    while (p->pos < p->len) {
        char* key = parser_string(p, "manifest key");
        if (!key) return false;
        bool ok = parser_expect(p, ':', "manifest");
        if (ok && strcmp(key, "schema") == 0) {
            free(out->schema);
            out->schema = parser_string(p, "schema");
            ok = out->schema != NULL;
        } else if (ok && strcmp(key, "version") == 0) {
            ok = parser_int(p, &out->version, "version");
        } else if (ok && strcmp(key, "name") == 0) {
            free(out->name);
            out->name = parser_string(p, "name");
            ok = out->name != NULL;
        } else if (ok && strcmp(key, "root") == 0) {
            free(out->root);
            out->root = parser_string(p, "root");
            ok = out->root != NULL;
        } else if (ok && strcmp(key, "build_dir") == 0) {
            free(out->buildDir);
            out->buildDir = parser_string(p, "build_dir");
            ok = out->buildDir != NULL;
        } else if (ok && strcmp(key, "defaults") == 0) {
            ok = parse_defaults(p, &out->defaults);
        } else if (ok && strcmp(key, "translation_units") == 0) {
            ok = parse_translation_units(p, out);
        } else if (ok && strcmp(key, "link") == 0) {
            ok = parse_link(p, &out->link);
        } else if (ok) {
            manifest_diag(p->diag, "unknown manifest field '%s'", key);
            ok = false;
        }
        free(key);
        if (!ok) return false;
        parser_skip_ws(p);
        if (parser_take(p, '}')) break;
        if (!parser_expect(p, ',', "manifest")) return false;
    }
    parser_skip_ws(p);
    if (p->pos != p->len) {
        manifest_diag(p->diag, "unexpected trailing text after manifest");
        return false;
    }
    if (!out->schema || strcmp(out->schema, "fisiCs.project") != 0) {
        manifest_diag(p->diag, "manifest schema must be fisiCs.project");
        return false;
    }
    if (out->version != 0) {
        manifest_diag(p->diag, "manifest version must be 0");
        return false;
    }
    if (!out->name || !out->name[0]) {
        manifest_diag(p->diag, "manifest missing name");
        return false;
    }
    if (out->translationUnitCount == 0) {
        manifest_diag(p->diag, "manifest missing translation_units");
        return false;
    }
    return true;
}

static char* path_dirname(const char* path) {
    const char* slash = path ? strrchr(path, '/') : NULL;
    if (!slash) return manifest_strdup(".");
    if (slash == path) return manifest_strdup("/");
    return manifest_strndup(path, (size_t)(slash - path));
}

static bool path_is_absolute(const char* path) {
    return path && path[0] == '/';
}

static char* path_join2(const char* a, const char* b) {
    if (!b) return NULL;
    if (path_is_absolute(b)) return manifest_strdup(b);
    if (strcmp(b, ".") == 0) return manifest_strdup((a && a[0]) ? a : ".");
    if (strncmp(b, "./", 2u) == 0) b += 2;
    if (!a || !a[0] || strcmp(a, ".") == 0) return manifest_strdup(b);
    size_t alen = strlen(a);
    size_t blen = strlen(b);
    bool slash = a[alen - 1u] != '/';
    char* out = (char*)malloc(alen + (slash ? 1u : 0u) + blen + 1u);
    if (!out) return NULL;
    memcpy(out, a, alen);
    size_t pos = alen;
    if (slash) out[pos++] = '/';
    memcpy(out + pos, b, blen + 1u);
    return out;
}

static char* derive_manifest_object_path(const char* buildDir, const char* source) {
    const char* base = source ? strrchr(source, '/') : NULL;
    base = base ? base + 1 : source;
    if (!base || !base[0]) return NULL;
    const char* dot = strrchr(base, '.');
    size_t stemLen = (dot && strcmp(dot, ".c") == 0) ? (size_t)(dot - base) : strlen(base);
    size_t dirLen = buildDir && buildDir[0] ? strlen(buildDir) : strlen("build/fisics");
    const char* dir = buildDir && buildDir[0] ? buildDir : "build/fisics";
    bool slash = dir[dirLen - 1u] != '/';
    char* out = (char*)malloc(dirLen + (slash ? 1u : 0u) + stemLen + 3u);
    if (!out) return NULL;
    memcpy(out, dir, dirLen);
    size_t pos = dirLen;
    if (slash) out[pos++] = '/';
    memcpy(out + pos, base, stemLen);
    pos += stemLen;
    memcpy(out + pos, ".o", 3u);
    return out;
}

static bool file_exists(const char* path) {
    struct stat st;
    return path && stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

static bool resolve_manifest_paths(const char* manifestPath,
                                   FisicsBuildManifest* out,
                                   FisicsBuildManifestDiagnostic* diag) {
    char* manifestDir = path_dirname(manifestPath);
    if (!manifestDir) {
        manifest_diag(diag, "out of memory while resolving manifest directory");
        return false;
    }
    if (!out->root) out->root = manifest_strdup(".");
    if (!out->buildDir) out->buildDir = manifest_strdup("build/fisics");
    if (!out->root || !out->buildDir) {
        free(manifestDir);
        manifest_diag(diag, "out of memory while applying manifest defaults");
        return false;
    }
    out->resolvedRoot = path_join2(manifestDir, out->root);
    free(manifestDir);
    if (!out->resolvedRoot) {
        manifest_diag(diag, "out of memory while resolving manifest root");
        return false;
    }
    for (size_t i = 0; i < out->translationUnitCount; ++i) {
        FisicsBuildManifestTranslationUnit* tu = &out->translationUnits[i];
        tu->resolvedSource = path_join2(out->resolvedRoot, tu->source);
        if (!tu->resolvedSource) {
            manifest_diag(diag, "out of memory while resolving source path");
            return false;
        }
        if (!file_exists(tu->resolvedSource)) {
            manifest_diag(diag, "translation unit source does not exist: %s", tu->source);
            return false;
        }
        if (!tu->object) {
            tu->object = derive_manifest_object_path(out->buildDir, tu->source);
            if (!tu->object) {
                manifest_diag(diag, "out of memory while deriving object path");
                return false;
            }
        }
        tu->resolvedObject = path_join2(out->resolvedRoot, tu->object);
        if (!tu->resolvedObject) {
            manifest_diag(diag, "out of memory while resolving object path");
            return false;
        }
    }
    if (out->link.output && out->link.output[0]) {
        out->link.resolvedOutput = path_join2(out->resolvedRoot, out->link.output);
        if (!out->link.resolvedOutput) {
            manifest_diag(diag, "out of memory while resolving link output path");
            return false;
        }
    }
    return true;
}

static char* read_file(const char* path, size_t* outLen) {
    FILE* fp = fopen(path, "rb");
    if (!fp) return NULL;
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return NULL;
    }
    long size = ftell(fp);
    if (size < 0) {
        fclose(fp);
        return NULL;
    }
    rewind(fp);
    char* data = (char*)malloc((size_t)size + 1u);
    if (!data) {
        fclose(fp);
        return NULL;
    }
    size_t got = fread(data, 1u, (size_t)size, fp);
    fclose(fp);
    if (got != (size_t)size) {
        free(data);
        return NULL;
    }
    data[got] = '\0';
    if (outLen) *outLen = got;
    return data;
}

bool fisics_build_manifest_load_file(const char* path,
                                     FisicsBuildManifest* out,
                                     FisicsBuildManifestDiagnostic* diag) {
    if (diag) diag->message[0] = '\0';
    if (!path || !out) {
        manifest_diag(diag, "invalid manifest load arguments");
        return false;
    }
    memset(out, 0, sizeof(*out));
    size_t len = 0;
    char* text = read_file(path, &len);
    if (!text) {
        manifest_diag(diag, "failed to read manifest: %s", path);
        return false;
    }
    ManifestParser p = {.text = text, .len = len, .diag = diag};
    bool ok = parse_manifest_root(&p, out) && resolve_manifest_paths(path, out, diag);
    free(text);
    if (!ok) {
        fisics_build_manifest_free(out);
        return false;
    }
    return true;
}

static void string_list_free(FisicsBuildManifestStringList* list) {
    if (!list) return;
    for (size_t i = 0; i < list->count; ++i) free(list->items[i]);
    free(list->items);
    list->items = NULL;
    list->count = 0;
}

void fisics_build_manifest_free(FisicsBuildManifest* manifest) {
    if (!manifest) return;
    free(manifest->schema);
    free(manifest->name);
    free(manifest->root);
    free(manifest->resolvedRoot);
    free(manifest->buildDir);
    free(manifest->defaults.standard);
    string_list_free(&manifest->defaults.includeDirs);
    string_list_free(&manifest->defaults.defines);
    string_list_free(&manifest->defaults.overlays);
    for (size_t i = 0; i < manifest->translationUnitCount; ++i) {
        free(manifest->translationUnits[i].source);
        free(manifest->translationUnits[i].object);
        free(manifest->translationUnits[i].resolvedSource);
        free(manifest->translationUnits[i].resolvedObject);
    }
    free(manifest->translationUnits);
    free(manifest->link.output);
    free(manifest->link.resolvedOutput);
    string_list_free(&manifest->link.libraries);
    string_list_free(&manifest->link.libraryDirs);
    string_list_free(&manifest->link.args);
    memset(manifest, 0, sizeof(*manifest));
}
