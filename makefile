# === Config ===
.DEFAULT_GOAL := all
CC := cc
LLVM_CONFIG := llvm-config

LLVM_CFLAGS := $(shell $(LLVM_CONFIG) --cflags)
LLVM_LDFLAGS := $(shell $(LLVM_CONFIG) --ldflags)
LLVM_LIBS := $(shell $(LLVM_CONFIG) --libs core)

CODEGEN_DEBUG ?= 0
CODEGEN_TRACE ?= 0
PARSER_DEBUG ?= 0
DISABLE_CODEGEN ?= 0
BUILD_PROFILE ?= unsanitized
SHIM_MODE ?= off
SHARED_ROOT ?= third_party/codework_shared
SYS_SHIMS_DIR ?= $(SHARED_ROOT)/sys_shims
CORE_BASE_DIR ?= $(SHARED_ROOT)/core/core_base
CORE_IO_DIR ?= $(SHARED_ROOT)/core/core_io
CORE_DATA_DIR ?= $(SHARED_ROOT)/core/core_data
CORE_PACK_DIR ?= $(SHARED_ROOT)/core/core_pack
SYS_SHIMS_OVERLAY := $(abspath $(SYS_SHIMS_DIR)/overlay/include)
SYS_SHIMS_INCLUDE := $(abspath $(SYS_SHIMS_DIR)/include)
SHIM_PROFILE_ID := shim_profile_lang_frontend_shadow_v1
SHIM_PROFILE_VERSION := 1

CODEGEN_DEFS :=
ifeq ($(CODEGEN_DEBUG),1)
CODEGEN_DEFS += -DCODEGEN_DEBUG
endif
ifeq ($(CODEGEN_TRACE),1)
CODEGEN_DEFS += -DCODEGEN_TRACE
endif

PARSER_DEFS :=
ifeq ($(PARSER_DEBUG),1)
PARSER_DEFS += -DPARSER_DEBUG
endif

BASE_CFLAGS = -Wall -Wextra -Wpedantic $(LLVM_CFLAGS) $(CODEGEN_DEFS) $(PARSER_DEFS) -DDEFAULT_INCLUDE_PATHS=\"$(DEFAULT_INCLUDE_PATHS)\"
BASE_LDFLAGS := $(LLVM_LDFLAGS) $(LLVM_LIBS)

ifeq ($(BUILD_PROFILE),sanitized)
PROFILE_CFLAGS := -g3 -O0 -fno-omit-frame-pointer -fsanitize=address,undefined
PROFILE_LDFLAGS := -fsanitize=address,undefined
else ifeq ($(BUILD_PROFILE),unsanitized)
PROFILE_CFLAGS := -O3 -DNDEBUG
PROFILE_LDFLAGS :=
else
$(error Unsupported BUILD_PROFILE '$(BUILD_PROFILE)' (expected unsanitized or sanitized))
endif

CFLAGS = $(BASE_CFLAGS) $(PROFILE_CFLAGS)
LDFLAGS := $(BASE_LDFLAGS) $(PROFILE_LDFLAGS)
DEPFLAGS := -MMD -MP

INCLUDES := -Isrc -Isrc/Lexer -Isrc/Parser -Isrc/Syntax -Isrc/Syntax/Expr -Isrc/Syntax/Decls -Isrc/CodeGen -Isrc/CodeGen/Expr -I$(CORE_BASE_DIR)/include -I$(CORE_IO_DIR)/include -I$(CORE_DATA_DIR)/include -I$(CORE_PACK_DIR)/include
SRC_DIR := src
BUILD_DIR := build/$(BUILD_PROFILE)
BIN := fisics
INCLUDE_PATHS_BASE := include
INCLUDE_PATHS ?= $(INCLUDE_PATHS_BASE)
ifeq ($(SHIM_MODE),shadow)
INCLUDE_PATHS := $(SYS_SHIMS_OVERLAY):$(INCLUDE_PATHS_BASE)
BIN := fisics_shim_shadow
endif
DEFAULT_INCLUDE_PATHS := $(if $(INCLUDE_PATHS),$(INCLUDE_PATHS),include)
EXAMPLES_DIR := examples
EXAMPLES_BUILD_DIR := build/examples
RELEASE_VERSION ?= 0.1.0
RELEASE_CHANNEL ?= stable
RELEASE_PLATFORM ?= macOS
RELEASE_ARCH ?= arm64
RELEASE_BASENAME := fisiCs-$(RELEASE_VERSION)-$(RELEASE_PLATFORM)-$(RELEASE_ARCH)-$(RELEASE_CHANNEL)
RELEASE_ROOT := build/release
RELEASE_STAGE_ROOT := $(RELEASE_ROOT)/stage
RELEASE_STAGE_DIR := $(RELEASE_STAGE_ROOT)/$(RELEASE_BASENAME)
RELEASE_BIN := $(RELEASE_STAGE_DIR)/bin/fisics
RELEASE_ARTIFACT_ZIP := $(RELEASE_ROOT)/$(RELEASE_BASENAME).zip
RELEASE_ARTIFACT_TGZ := $(RELEASE_ROOT)/$(RELEASE_BASENAME).tar.gz
RELEASE_MANIFEST := $(RELEASE_ROOT)/$(RELEASE_BASENAME).manifest.txt
RELEASE_SHA256 := $(RELEASE_ROOT)/$(RELEASE_BASENAME).sha256
RELEASE_NOTARY_LOG := $(RELEASE_ROOT)/$(RELEASE_BASENAME).notary.json
RELEASE_PKG_ROOT := $(RELEASE_ROOT)/pkgroot
RELEASE_PKG := $(RELEASE_ROOT)/$(RELEASE_BASENAME).pkg
RELEASE_BRIDGE_DIR := $(RELEASE_ROOT)/bridge/$(RELEASE_BASENAME)
RELEASE_PUBLIC_BASE_URL ?= https://downloads.example.com/fisiCs
RELEASE_TAP_REPO ?= owner/homebrew-fisics
RELEASE_HOMEPAGE ?= https://github.com/owner/fisiCs
RELEASE_VPS_TARGET ?= user@example.com
RELEASE_VPS_ROOT ?= /var/www/downloads/fisiCs
INSTALL_PREFIX ?= /usr/local
PKG_IDENTIFIER ?= com.cosm.fisics
APPLE_SIGN_IDENTITY ?=
APPLE_NOTARY_PROFILE ?=
APPLE_INSTALLER_IDENTITY ?=

# === Step 1: Generate keyword_lookup.c from keywords.gperf ===
src/Lexer/keyword_lookup.c: src/Lexer/keywords.gperf
	@echo "Generating keyword_lookup.c from keywords.gperf..."
	gperf src/Lexer/keywords.gperf > src/Lexer/keyword_lookup.c

# === Step 2: Ensure all objects depend on keyword_lookup.c ===
# This makes sure it's generated before any compilation happens
$(BUILD_DIR)/%.o: src/Lexer/keyword_lookup.c

# === Step 3: Find all .c files and create .o list ===
SRCS := $(shell find $(SRC_DIR) -name '*.c')
SRCS += src/Lexer/keyword_lookup.c

SRCS += src/Preprocessor/include_resolver.c
CORE_BASE_SRCS := $(CORE_BASE_DIR)/src/core_base.c
CORE_IO_SRCS := $(CORE_IO_DIR)/src/core_io.c
CORE_DATA_SRCS := $(CORE_DATA_DIR)/src/core_data.c
CORE_PACK_SRCS := $(CORE_PACK_DIR)/src/core_pack.c

OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))
CORE_BASE_OBJS := $(patsubst $(CORE_BASE_DIR)/src/%.c,$(BUILD_DIR)/core_base/%.o,$(CORE_BASE_SRCS))
CORE_IO_OBJS := $(patsubst $(CORE_IO_DIR)/src/%.c,$(BUILD_DIR)/core_io/%.o,$(CORE_IO_SRCS))
CORE_DATA_OBJS := $(patsubst $(CORE_DATA_DIR)/src/%.c,$(BUILD_DIR)/core_data/%.o,$(CORE_DATA_SRCS))
CORE_PACK_OBJS := $(patsubst $(CORE_PACK_DIR)/src/%.c,$(BUILD_DIR)/core_pack/%.o,$(CORE_PACK_SRCS))
OBJS += $(CORE_BASE_OBJS) $(CORE_IO_OBJS) $(CORE_DATA_OBJS) $(CORE_PACK_OBJS)
DEPS := $(OBJS:.o=.d)
LIB_FRONTEND := libfisics_frontend.a
# Include codegen in the archive so downstream IDEs don’t see undefined symbols.
FRONTEND_OBJS := $(filter-out $(BUILD_DIR)/main.o $(BUILD_DIR)/Compiler/object_emit.o,$(OBJS))
# Only keep the object emitter separate for the CLI binary.
BACKEND_OBJS := $(BUILD_DIR)/Compiler/object_emit.o

# === Default Target ===
all: $(BIN)

# Frontend-only targets
.PHONY: frontend frontend-clean frontend-rebuild frontend-rebuild-local frontend-rebuild-ide \
        frontend-sanitized frontend-unsanitized frontend-both
frontend: $(LIB_FRONTEND)
frontend-sanitized:
	@$(MAKE) BUILD_PROFILE=sanitized LIB_FRONTEND=libfisics_frontend_sanitized.a frontend
frontend-unsanitized:
	@$(MAKE) BUILD_PROFILE=unsanitized LIB_FRONTEND=libfisics_frontend_unsanitized.a frontend
frontend-both: frontend-unsanitized frontend-sanitized
frontend-clean:
	@echo "Cleaning frontend library..."
	rm -f $(LIB_FRONTEND)
	rm -f $(FRONTEND_OBJS)
frontend-rebuild-local: frontend-clean frontend
frontend-rebuild-ide: frontend-both
	@cp -f libfisics_frontend_unsanitized.a libfisics_frontend.a
	@echo "Refreshed frontend IDE archives: libfisics_frontend_unsanitized.a, libfisics_frontend_sanitized.a"
frontend-rebuild: frontend-rebuild-ide

# Frontend static library (reusable by IDE)
$(LIB_FRONTEND): $(FRONTEND_OBJS)
	@echo "Archiving $@..."
	ar rcs $@ $^

# === Link object files into final binary ===
$(BIN): $(LIB_FRONTEND) $(BUILD_DIR)/main.o $(BACKEND_OBJS)
	@echo "Linking $@..."
	$(CC) $(BUILD_DIR)/main.o $(BACKEND_OBJS) $(LIB_FRONTEND) -o $@ $(LDFLAGS)

# === Compile each .c file into .o only if changed ===
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(DEPFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/core_base/%.o: $(CORE_BASE_DIR)/src/%.c
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(DEPFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/core_io/%.o: $(CORE_IO_DIR)/src/%.c
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(DEPFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/core_data/%.o: $(CORE_DATA_DIR)/src/%.c
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(DEPFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/core_pack/%.o: $(CORE_PACK_DIR)/src/%.c
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(DEPFLAGS) $(INCLUDES) -c $< -o $@

-include $(DEPS)

# === Clean: remove binary + build artifacts ===
clean:
	@echo "Cleaning..."
	rm -rf build $(BIN) $(LIB_FRONTEND) libfisics_frontend_unsanitized.a libfisics_frontend_sanitized.a
	rm -f src/Lexer/keyword_lookup.c

release-clean:
	@echo "Cleaning release artifacts..."
	rm -rf $(RELEASE_ROOT)

release-contract:
	@echo "Release contract"
	@echo "  version:      $(RELEASE_VERSION)"
	@echo "  channel:      $(RELEASE_CHANNEL)"
	@echo "  platform:     $(RELEASE_PLATFORM)"
	@echo "  arch:         $(RELEASE_ARCH)"
	@echo "  basename:     $(RELEASE_BASENAME)"
	@echo "  stage dir:    $(RELEASE_STAGE_DIR)"
	@echo "  zip artifact: $(RELEASE_ARTIFACT_ZIP)"
	@echo "  tgz artifact: $(RELEASE_ARTIFACT_TGZ)"
	@echo "  notary log:   $(RELEASE_NOTARY_LOG)"

release-build:
	@$(MAKE) BUILD_PROFILE=unsanitized SHIM_MODE=off all

release-stage: release-build
	@echo "Staging release tree..."
	@rm -rf "$(RELEASE_STAGE_DIR)"
	@mkdir -p "$(RELEASE_STAGE_DIR)/bin" "$(RELEASE_STAGE_DIR)/docs"
	@cp -f "./$(BIN)" "$(RELEASE_BIN)"
	@cp -f README.md LICENSE "$(RELEASE_STAGE_DIR)/"
	@cp -f docs/00_docs_index.md docs/README.md "$(RELEASE_STAGE_DIR)/docs/"
	@printf "name=fisiCs\nversion=%s\nchannel=%s\nplatform=%s\narch=%s\n" \
		"$(RELEASE_VERSION)" "$(RELEASE_CHANNEL)" "$(RELEASE_PLATFORM)" "$(RELEASE_ARCH)" \
		> "$(RELEASE_STAGE_DIR)/release_metadata.env"
	@chmod 755 "$(RELEASE_BIN)"

release-manifest-from-stage:
	@echo "Generating release manifest..."
	@mkdir -p "$(RELEASE_ROOT)"
	@{ \
		echo "# manifest $(RELEASE_BASENAME)"; \
		echo "# generated=$$(date -u +%Y-%m-%dT%H:%M:%SZ)"; \
	} > "$(RELEASE_MANIFEST)"
	@(cd "$(RELEASE_STAGE_ROOT)" && \
		find "$(RELEASE_BASENAME)" -type f | sort | while read -r rel; do \
			shasum -a 256 "$$rel"; \
		done) >> "$(RELEASE_MANIFEST)"

release-manifest: release-stage release-manifest-from-stage

release-archive-zip-from-stage: release-manifest-from-stage
	@echo "Creating zip artifact..."
	@mkdir -p "$(RELEASE_ROOT)"
	@rm -f "$(RELEASE_ARTIFACT_ZIP)"
	@(cd "$(RELEASE_STAGE_ROOT)" && \
		ditto -c -k --sequesterRsrc --keepParent "$(RELEASE_BASENAME)" "$(abspath $(RELEASE_ARTIFACT_ZIP))")
	@shasum -a 256 "$(RELEASE_ARTIFACT_ZIP)" > "$(RELEASE_SHA256)"
	@echo "Created $(RELEASE_ARTIFACT_ZIP)"

release-archive-zip: release-stage release-archive-zip-from-stage

release-archive-tgz-from-stage: release-manifest-from-stage
	@echo "Creating tar.gz artifact..."
	@mkdir -p "$(RELEASE_ROOT)"
	@rm -f "$(RELEASE_ARTIFACT_TGZ)"
	@COPYFILE_DISABLE=1 tar -C "$(RELEASE_STAGE_ROOT)" -czf "$(RELEASE_ARTIFACT_TGZ)" "$(RELEASE_BASENAME)"
	@echo "Created $(RELEASE_ARTIFACT_TGZ)"

release-archive-tgz: release-stage release-archive-tgz-from-stage

release-archive: release-stage release-archive-zip-from-stage release-archive-tgz-from-stage

release-sign: release-stage
	@if [ -z "$(APPLE_SIGN_IDENTITY)" ]; then \
		echo "ERROR: APPLE_SIGN_IDENTITY is required for release-sign"; \
		exit 2; \
	fi
	@echo "Signing release binary with identity: $(APPLE_SIGN_IDENTITY)"
	@codesign --force --options runtime --timestamp --sign "$(APPLE_SIGN_IDENTITY)" "$(RELEASE_BIN)"
	@codesign --verify --verbose=2 "$(RELEASE_BIN)"
	@$(MAKE) release-archive-zip-from-stage release-archive-tgz-from-stage
	@echo "release-sign complete."

release-notarize: release-sign
	@if [ -z "$(APPLE_NOTARY_PROFILE)" ]; then \
		echo "ERROR: APPLE_NOTARY_PROFILE is required for release-notarize"; \
		exit 2; \
	fi
	@echo "Submitting archive for notarization..."
	@xcrun notarytool submit "$(RELEASE_ARTIFACT_ZIP)" --keychain-profile "$(APPLE_NOTARY_PROFILE)" --wait --output-format json > "$(RELEASE_NOTARY_LOG)"
	@echo "Notarization response saved to $(RELEASE_NOTARY_LOG)"

release-verify: release-archive
	@echo "Verifying release bundle..."
	@test -x "$(RELEASE_BIN)"
	@codesign --verify --verbose=2 "$(RELEASE_BIN)" || true
	@spctl --assess --type execute --verbose=4 "$(RELEASE_BIN)" || \
		echo "note: spctl may reject raw CLI binaries as non-app; use notarization status + codesign verification for release gate."
	@shasum -a 256 -c "$(RELEASE_SHA256)"
	@echo "release-verify complete."

release-pkg: release-sign
	@if [ -z "$(APPLE_INSTALLER_IDENTITY)" ]; then \
		echo "ERROR: APPLE_INSTALLER_IDENTITY is required for release-pkg"; \
		echo "Expected: Developer ID Installer: <Name> (<TEAMID>)"; \
		exit 2; \
	fi
	@echo "Building installer pkg..."
	@rm -rf "$(RELEASE_PKG_ROOT)"
	@mkdir -p "$(RELEASE_PKG_ROOT)$(INSTALL_PREFIX)/bin" "$(RELEASE_ROOT)"
	@cp -f "$(RELEASE_BIN)" "$(RELEASE_PKG_ROOT)$(INSTALL_PREFIX)/bin/fisics"
	@pkgbuild --root "$(RELEASE_PKG_ROOT)" \
		--identifier "$(PKG_IDENTIFIER)" \
		--version "$(RELEASE_VERSION)" \
		--install-location "/" \
		"$(RELEASE_PKG).unsigned"
	@productsign --sign "$(APPLE_INSTALLER_IDENTITY)" "$(RELEASE_PKG).unsigned" "$(RELEASE_PKG)"
	@rm -f "$(RELEASE_PKG).unsigned"
	@echo "Built $(RELEASE_PKG)"

release-all: release-contract release-archive release-verify

release-bridge-prepare: release-verify
	@RELEASE_ROOT="$(RELEASE_ROOT)" \
	RELEASE_BASENAME="$(RELEASE_BASENAME)" \
	RELEASE_VERSION="$(RELEASE_VERSION)" \
	RELEASE_CHANNEL="$(RELEASE_CHANNEL)" \
	RELEASE_PLATFORM="$(RELEASE_PLATFORM)" \
	RELEASE_ARCH="$(RELEASE_ARCH)" \
	RELEASE_ARTIFACT_ZIP="$(RELEASE_ARTIFACT_ZIP)" \
	RELEASE_ARTIFACT_TGZ="$(RELEASE_ARTIFACT_TGZ)" \
	RELEASE_MANIFEST="$(RELEASE_MANIFEST)" \
	RELEASE_SHA256="$(RELEASE_SHA256)" \
	RELEASE_NOTARY_LOG="$(RELEASE_NOTARY_LOG)" \
	RELEASE_PUBLIC_BASE_URL="$(RELEASE_PUBLIC_BASE_URL)" \
	RELEASE_TAP_REPO="$(RELEASE_TAP_REPO)" \
	RELEASE_HOMEPAGE="$(RELEASE_HOMEPAGE)" \
	RELEASE_VPS_TARGET="$(RELEASE_VPS_TARGET)" \
	RELEASE_VPS_ROOT="$(RELEASE_VPS_ROOT)" \
	./scripts/release_bridge_prepare.sh

release-bridge-verify:
	@test -f "$(RELEASE_BRIDGE_DIR)/release_index.json"
	@test -f "$(RELEASE_BRIDGE_DIR)/homebrew/fisics.rb"
	@test -f "$(RELEASE_BRIDGE_DIR)/ide/fisics_compiler_discovery_contract.md"
	@test -f "$(RELEASE_BRIDGE_DIR)/publish/upload_commands.sh"
	@test -f "$(RELEASE_BRIDGE_DIR)/publish/release_publish_checklist.md"
	@test -f "$(RELEASE_BRIDGE_DIR)/artifacts/$(RELEASE_BASENAME).zip"
	@test -f "$(RELEASE_BRIDGE_DIR)/artifacts/$(RELEASE_BASENAME).tar.gz"
	@test -f "$(RELEASE_BRIDGE_DIR)/artifacts/$(RELEASE_BASENAME).manifest.txt"
	@test -f "$(RELEASE_BRIDGE_DIR)/artifacts/$(RELEASE_BASENAME).sha256"
	@echo "release-bridge-verify complete."

release-bridge: release-bridge-prepare release-bridge-verify
	@echo "release-bridge complete."

integration-compile-only: $(BIN)
	@./tests/integration/compile_only.sh ./$(BIN)

integration-compile-link: $(BIN)
	@./tests/integration/compile_and_link.sh ./$(BIN)

integration-diags-pack: $(BIN)
	@./tests/integration/run_emit_diags_pack.sh ./$(BIN)

integration-std-atomic: $(BIN)
	@./tests/integration/run_std_atomic.sh ./$(BIN)

integration-std-atomic-link: $(BIN)
	@./tests/integration/run_std_atomic_link.sh ./$(BIN)

integration: integration-compile-only integration-compile-link integration-std-atomic integration-std-atomic-link

ci-guardrails:
	@./tests/integration/run_ci_guardrails.sh

# Final C99 behavior suite
.PHONY: final final-update final-id final-prefix final-glob final-bucket final-manifest final-wave final-runtime final-timing final-timing-sync-db final-timing-rollup final-timing-sync
final: $(BIN)
	@python3 -u tests/final/run_final.py ./$(BIN)

final-update: $(BIN)
	@UPDATE_FINAL=1 python3 -u tests/final/run_final.py ./$(BIN)

# Exact-id slice (example: make final-id ID=14__runtime_multitu_mixed_abi_call_chain)
final-id: $(BIN)
	@if [ -z "$(ID)" ]; then echo "ERROR: provide ID=<test_id>"; exit 2; fi
	@FINAL_FILTER="$(ID)" python3 -u tests/final/run_final.py ./$(BIN)

# Prefix slice (example: make final-prefix PREFIX=14__)
final-prefix: $(BIN)
	@if [ -z "$(PREFIX)" ]; then echo "ERROR: provide PREFIX=<id_prefix>"; exit 2; fi
	@FINAL_PREFIX="$(PREFIX)" python3 -u tests/final/run_final.py ./$(BIN)

# Glob slice (example: make final-glob GLOB='14__runtime_multitu_*')
final-glob: $(BIN)
	@if [ -z "$(GLOB)" ]; then echo "ERROR: provide GLOB=<fnmatch_pattern>"; exit 2; fi
	@FINAL_GLOB="$(GLOB)" python3 -u tests/final/run_final.py ./$(BIN)

# Bucket slice by metadata bucket field (example: make final-bucket BUCKET=runtime-surface)
final-bucket: $(BIN)
	@if [ -z "$(BUCKET)" ]; then echo "ERROR: provide BUCKET=<bucket_name>"; exit 2; fi
	@FINAL_BUCKET="$(BUCKET)" python3 -u tests/final/run_final.py ./$(BIN)

# Manifest shard slice (example: make final-manifest MANIFEST=14-runtime-surface-wave43-multitu-abi-stress-promotions.json)
final-manifest: $(BIN)
	@if [ -z "$(MANIFEST)" ]; then echo "ERROR: provide MANIFEST=<manifest_name_or_token>"; exit 2; fi
	@FINAL_MANIFEST="$(MANIFEST)" python3 -u tests/final/run_final.py ./$(BIN)

# Wave slice (runtime-focused by default).
# Examples:
#   make final-wave WAVE=43
#   make final-wave WAVE=43 WAVE_BUCKET=14-runtime-surface
WAVE_BUCKET ?= 14-runtime-surface
final-wave: $(BIN)
	@if [ -z "$(WAVE)" ]; then echo "ERROR: provide WAVE=<number>"; exit 2; fi
	@FINAL_MANIFEST_GLOB="$(WAVE_BUCKET)-wave$(WAVE)-*.json" python3 -u tests/final/run_final.py ./$(BIN)

# Runtime convenience slice (all bucket-14 tests)
final-runtime: $(BIN)
	@FINAL_PREFIX="14__" python3 -u tests/final/run_final.py ./$(BIN)

FINAL_TIMING_RUNS ?= 1
FINAL_TIMING_TAG ?= manual
FINAL_TIMING_LOG ?= $(abspath ../docs/private_program_docs/fisiCs/audits/make_final_timing_log.csv)
FINAL_TIMING_NOTES ?= $(abspath ../docs/private_program_docs/fisiCs/audits/make_final_timing_notes.md)
FINAL_TIMING_DB ?= $(abspath ../data/fisics_timing/make_final_timing.sqlite)
FINAL_TIMING_ROLLUP ?= $(abspath ../docs/private_program_docs/fisiCs/audits/make_final_timing_rollup.md)
FINAL_TIMING_NOTE ?=

# Full-suite timing capture with append-only logging.
# Examples:
#   make final-timing
#   make final-timing FINAL_TIMING_RUNS=3 FINAL_TIMING_TAG=checkpoint
#   make final-timing FINAL_TIMING_NOTE="after bucket 15 wave update"
final-timing: $(BIN)
	@FINAL_TIMING_RUNS="$(FINAL_TIMING_RUNS)" \
	  FINAL_TIMING_TAG="$(FINAL_TIMING_TAG)" \
	  FINAL_TIMING_LOG="$(FINAL_TIMING_LOG)" \
	  FINAL_TIMING_NOTES="$(FINAL_TIMING_NOTES)" \
	  FINAL_TIMING_NOTE="$(FINAL_TIMING_NOTE)" \
	  ./scripts/capture_make_final_timing.sh

# Mirror canonical CSV rows into SQLite for query/rollup use.
final-timing-sync-db:
	@./scripts/sync_make_final_timing_sqlite.py \
	  --csv "$(FINAL_TIMING_LOG)" \
	  --db "$(FINAL_TIMING_DB)"

# Render markdown rollup from SQLite.
final-timing-rollup:
	@./scripts/render_make_final_timing_rollup.py \
	  --csv "$(FINAL_TIMING_LOG)" \
	  --db "$(FINAL_TIMING_DB)" \
	  --output "$(FINAL_TIMING_ROLLUP)"

# End-to-end timing capture + sqlite mirror + markdown rollup.
final-timing-sync: final-timing final-timing-sync-db final-timing-rollup

# === Run the compiled binary ===
run: src/Lexer/keyword_lookup.c $(BIN)
	@MallocNanoZone=0 ./$(BIN)

examples: $(BIN) examples-hello examples-sdl

examples-hello: $(BIN)
	@mkdir -p $(EXAMPLES_BUILD_DIR)
	@./$(BIN) $(EXAMPLES_DIR)/hello_world.c -o $(EXAMPLES_BUILD_DIR)/hello_world
	@echo "Built $(EXAMPLES_BUILD_DIR)/hello_world"

examples-sdl: $(BIN)
	@mkdir -p $(EXAMPLES_BUILD_DIR)
	@if command -v sdl2-config >/dev/null 2>&1; then \
		./$(BIN) -c $(EXAMPLES_DIR)/sdl_window_loop.c -o $(EXAMPLES_BUILD_DIR)/sdl_window_loop.o; \
		$(CC) $(EXAMPLES_BUILD_DIR)/sdl_window_loop.o -o $(EXAMPLES_BUILD_DIR)/sdl_window_loop $$(sdl2-config --cflags --libs); \
		echo "Built $(EXAMPLES_BUILD_DIR)/sdl_window_loop"; \
	else \
		echo "Skipping SDL example build: sdl2-config not found on PATH."; \
	fi

union-decl: $(BIN)
	@./tests/parser/run_union_decl.sh ./$(BIN)

initializer-expr: $(BIN)
	@./tests/parser/run_initializer_expr.sh ./$(BIN)

typedef-chain: $(BIN)
	@./tests/parser/run_typedef_chain.sh ./$(BIN)

designated-init: $(BIN)
	@./tests/parser/run_designated_init.sh ./$(BIN)

control-flow: $(BIN)
	@./tests/parser/run_control_flow.sh ./$(BIN)

cast-grouped: $(BIN)
	@./tests/parser/run_cast_grouped.sh ./$(BIN)

for_typedef: $(BIN)
	@./tests/parser/run_for_typedef.sh ./$(BIN)

comma-decl-init: $(BIN)
	@./tests/parser/run_comma_decl_init.sh ./$(BIN)

function-pointer: $(BIN)
	@./tests/parser/run_function_pointer.sh ./$(BIN)

compound-literal-nested-braces: $(BIN)
	@./tests/parser/run_compound_literal_nested_braces.sh ./$(BIN)

switch-flow: $(BIN)
ifeq ($(DISABLE_CODEGEN),1)
	@echo "Skipping switch_flow (codegen disabled)"
else
	@./tests/parser/run_switch_flow.sh ./$(BIN)
endif

goto-flow: $(BIN)
	@./tests/parser/run_goto_flow.sh ./$(BIN)

recovery: $(BIN)
	@./tests/parser/run_recovery.sh ./$(BIN)

pointer-arith: $(BIN)
ifeq ($(DISABLE_CODEGEN),1)
	@echo "Skipping pointer-arith (codegen disabled)"
else
	@./tests/codegen/run_pointer_arith.sh ./$(BIN)
endif

codegen-pointer-deref: $(BIN)
ifeq ($(DISABLE_CODEGEN),1)
	@echo "Skipping codegen-pointer-deref (codegen disabled)"
else
	@./tests/codegen/run_codegen_pointer_deref.sh ./$(BIN)
endif

codegen-pointer-diff: $(BIN)
ifeq ($(DISABLE_CODEGEN),1)
	@echo "Skipping codegen-pointer-diff (codegen disabled)"
else
	@./tests/codegen/run_codegen_pointer_diff_long.sh ./$(BIN)
endif

codegen-pointer-diff-decay: $(BIN)
ifeq ($(DISABLE_CODEGEN),1)
	@echo "Skipping codegen-pointer-diff-decay (codegen disabled)"
else
	@./tests/codegen/run_codegen_pointer_diff_decay.sh ./$(BIN)
endif

codegen-pointer-to-array: $(BIN)
ifeq ($(DISABLE_CODEGEN),1)
	@echo "Skipping codegen-pointer-to-array (codegen disabled)"
else
	@./tests/codegen/run_codegen_pointer_to_array.sh ./$(BIN)
endif

codegen-logical-short-circuit: $(BIN)
ifeq ($(DISABLE_CODEGEN),1)
	@echo "Skipping codegen-logical-short-circuit (codegen disabled)"
else
	@./tests/codegen/run_codegen_logical_short_circuit.sh ./$(BIN)
endif

codegen-function-pointer-call: $(BIN)
ifeq ($(DISABLE_CODEGEN),1)
	@echo "Skipping codegen-function-pointer-call (codegen disabled)"
else
	@./tests/codegen/run_function_pointer_call.sh ./$(BIN)
endif

codegen-compound-literal-pointer-decay: $(BIN)
ifeq ($(DISABLE_CODEGEN),1)
	@echo "Skipping codegen-compound-literal-pointer-decay (codegen disabled)"
else
	@./tests/codegen/run_compound_literal_pointer_decay.sh ./$(BIN)
endif

codegen-bitfield: $(BIN)
ifeq ($(DISABLE_CODEGEN),1)
	@echo "Skipping codegen-bitfield (codegen disabled)"
else
	@./tests/codegen/run_codegen_bitfield.sh ./$(BIN)
endif

codegen-alignas: $(BIN)
ifeq ($(DISABLE_CODEGEN),1)
	@echo "Skipping codegen-alignas (codegen disabled)"
else
	@./tests/codegen/run_codegen_alignas.sh ./$(BIN)
endif

codegen-flex-memset: $(BIN)
ifeq ($(DISABLE_CODEGEN),1)
	@echo "Skipping codegen-flex-memset (codegen disabled)"
else
	@./tests/codegen/run_codegen_flex_memset.sh ./$(BIN)
endif

codegen-flex-lvalue: $(BIN)
ifeq ($(DISABLE_CODEGEN),1)
	@echo "Skipping codegen-flex-lvalue (codegen disabled)"
else
	@./tests/codegen/run_codegen_flex_lvalue.sh ./$(BIN)
endif

codegen-flex-struct-array: $(BIN)
ifeq ($(DISABLE_CODEGEN),1)
	@echo "Skipping codegen-flex-struct-array (codegen disabled)"
else
	@./tests/codegen/run_codegen_flex_struct_array.sh ./$(BIN)
endif

codegen-ternary-merge: $(BIN)
ifeq ($(DISABLE_CODEGEN),1)
	@echo "Skipping codegen-ternary-merge (codegen disabled)"
else
	@./tests/codegen/run_codegen_ternary_merge.sh ./$(BIN)
endif

codegen-shift: $(BIN)
ifeq ($(DISABLE_CODEGEN),1)
	@echo "Skipping codegen-shift (codegen disabled)"
else
	@./tests/codegen/run_codegen_shift.sh ./$(BIN)
endif

codegen-varargs-call: $(BIN)
ifeq ($(DISABLE_CODEGEN),1)
	@echo "Skipping codegen-varargs-call (codegen disabled)"
else
	@./tests/codegen/run_codegen_varargs_call.sh ./$(BIN)
endif

codegen-opaque-pointer: $(BIN)
ifeq ($(DISABLE_CODEGEN),1)
	@echo "Skipping codegen-opaque-pointer (codegen disabled)"
else
	@./tests/codegen/run_codegen_opaque_pointer_diag.sh ./$(BIN)
endif

codegen-const-globals: $(BIN)
ifeq ($(DISABLE_CODEGEN),1)
	@echo "Skipping codegen-const-globals (codegen disabled)"
else
	@./tests/codegen/run_codegen_const_globals.sh ./$(BIN)
endif

codegen-callconv-declspec: $(BIN)
ifeq ($(DISABLE_CODEGEN),1)
	@echo "Skipping codegen-callconv-declspec (codegen disabled)"
else
	@./tests/codegen/run_codegen_callconv_declspec.sh ./$(BIN)
endif

codegen-switch-dense: $(BIN)
ifeq ($(DISABLE_CODEGEN),1)
	@echo "Skipping codegen-switch-dense (codegen disabled)"
else
	@./tests/codegen/run_codegen_switch_dense.sh ./$(BIN)
endif

codegen-switch-sparse: $(BIN)
ifeq ($(DISABLE_CODEGEN),1)
	@echo "Skipping codegen-switch-sparse (codegen disabled)"
else
	@./tests/codegen/run_codegen_switch_sparse.sh ./$(BIN)
endif

codegen-compound-literal-storage: $(BIN)
ifeq ($(DISABLE_CODEGEN),1)
	@echo "Skipping codegen-compound-literal-storage (codegen disabled)"
else
	@./tests/codegen/run_codegen_compound_literal_storage.sh ./$(BIN)
endif

codegen-builtin-expect: $(BIN)
ifeq ($(DISABLE_CODEGEN),1)
	@echo "Skipping codegen-builtin-expect (codegen disabled)"
else
	@./tests/codegen/run_codegen_builtin_expect.sh ./$(BIN)
endif

codegen-weak-linkage: $(BIN)
ifeq ($(DISABLE_CODEGEN),1)
	@echo "Skipping codegen-weak-linkage (codegen disabled)"
else
	@./tests/codegen/run_codegen_weak_linkage.sh ./$(BIN)
endif

statement-expr-codegen: $(BIN)
ifeq ($(DISABLE_CODEGEN),1)
	@echo "Skipping statement-expr-codegen (codegen disabled)"
else
	@./tests/codegen/run_statement_expr_codegen.sh ./$(BIN)
endif

semantic-typedef: $(BIN)
	@./tests/syntax/run_semantic_typedef.sh ./$(BIN)

semantic-initializer: $(BIN)
	@./tests/syntax/run_semantic_initializer.sh ./$(BIN)

semantic-undeclared: $(BIN)
	@./tests/syntax/run_semantic_undeclared.sh ./$(BIN)

semantic-bool: $(BIN)
	@./tests/syntax/run_semantic_bool.sh ./$(BIN)

semantic-invalid-arith: $(BIN)
	@./tests/syntax/run_semantic_invalid_arith.sh ./$(BIN)

semantic-lvalue-errors: $(BIN)
	@./tests/syntax/run_semantic_lvalue_errors.sh ./$(BIN)

semantic-pointer-errors: $(BIN)
	@./tests/syntax/run_semantic_pointer_errors.sh ./$(BIN)

semantic-pointer-qualifier: $(BIN)
	@./tests/syntax/run_semantic_pointer_qualifier.sh ./$(BIN)

semantic-function-calls: $(BIN)
	@./tests/syntax/run_semantic_function_calls.sh ./$(BIN)

semantic-tag-conflicts: $(BIN)
	@./tests/syntax/run_semantic_tag_conflicts.sh ./$(BIN)

semantic-initializer-shapes: $(BIN)
	@./tests/syntax/run_semantic_initializer_shapes.sh ./$(BIN)

semantic-flow: $(BIN)
	@./tests/syntax/run_semantic_flow.sh ./$(BIN)

semantic-goto-reachable-label: $(BIN)
	@./tests/syntax/run_semantic_goto_reachable_label.sh ./$(BIN)

semantic-switch-grouped-cases: $(BIN)
	@./tests/syntax/run_semantic_switch_grouped_cases.sh ./$(BIN)

semantic-vla-errors: $(BIN)
	@./tests/syntax/run_semantic_vla_errors.sh ./$(BIN)

semantic-vla-block: $(BIN)
	@./tests/syntax/run_semantic_vla_block.sh ./$(BIN)

semantic-struct-definition-only: $(BIN)
	@./tests/syntax/run_semantic_struct_definition_only.sh ./$(BIN)

semantic-struct-inline-declarator: $(BIN)
	@./tests/syntax/run_semantic_struct_inline_declarator.sh ./$(BIN)

semantic-anon-record-flatten: $(BIN)
	@./tests/syntax/run_semantic_anon_record_flatten.sh ./$(BIN)

semantic-vla-param-decay: $(BIN)
	@./tests/syntax/run_semantic_vla_param_decay.sh ./$(BIN)

semantic-flexible-array: $(BIN)
	@./tests/syntax/run_semantic_flexible_array.sh ./$(BIN)

semantic-knr-function: $(BIN)
	@./tests/syntax/run_semantic_knr_function.sh ./$(BIN)

semantic-unsequenced: $(BIN)
	@./tests/syntax/run_semantic_unsequenced.sh ./$(BIN)

semantic-restrict-alias: $(BIN)
	@./tests/syntax/run_semantic_restrict_alias.sh ./$(BIN)

semantic-restrict-alias-fields: $(BIN)
	@./tests/syntax/run_semantic_restrict_alias_fields.sh ./$(BIN)

semantic-alignas: $(BIN)
	@./tests/syntax/run_semantic_alignas.sh ./$(BIN)

semantic-bitfield-layout: $(BIN)
	@./tests/syntax/run_semantic_bitfield_layout.sh ./$(BIN)

semantic-long-double-layout: $(BIN)
	@./tests/syntax/run_semantic_long_double_layout.sh ./$(BIN)

semantic-atomic-qualifier: $(BIN)
	@./tests/syntax/run_semantic_atomic_qualifier.sh ./$(BIN)

semantic-static-assert-member-array-size: $(BIN)
	@./tests/syntax/run_semantic_static_assert_member_array_size.sh ./$(BIN)

semantic-static-local-float-constexpr: $(BIN)
	@./tests/syntax/run_semantic_static_local_float_constexpr.sh ./$(BIN)

semantic-static-float-constexpr: $(BIN)
	@./tests/syntax/run_static_float_constexpr.sh ./$(BIN)

semantic-flexible-array-layout: $(BIN)
	@./tests/syntax/run_semantic_flexible_array_layout.sh ./$(BIN)

semantic-union-anon-member: $(BIN)
	@./tests/syntax/run_semantic_union_anon_member.sh ./$(BIN)

semantic-sizeof-alignof-vla: $(BIN)
	@./tests/syntax/run_sizeof_alignof_vla.sh ./$(BIN)

semantic-case-non-ice: $(BIN)
	@./tests/syntax/run_case_non_ice.sh ./$(BIN)

semantic-char-escape-consteval: $(BIN)
	@./tests/syntax/run_char_escape_consteval.sh ./$(BIN)

semantic-ternary-merge-ptr: $(BIN)
	@./tests/syntax/run_ternary_merge_ptr.sh ./$(BIN)

semantic-shift-unsigned: $(BIN)
	@./tests/syntax/run_shift_unsigned.sh ./$(BIN)

compound-literal-lvalues: $(BIN)
	@./tests/syntax/run_compound_literal_lvalues.sh ./$(BIN)

semantic-complex-declarators: $(BIN)
	@./tests/syntax/run_semantic_complex_declarators.sh ./$(BIN)

statement-expr-enabled: $(BIN)
	@./tests/parser/run_statement_expr_enabled.sh ./$(BIN)

statement-expr-default: $(BIN)
	@./tests/parser/run_statement_expr_default.sh ./$(BIN)

statement-expr-disabled: $(BIN)
	@./tests/parser/run_statement_expr_disabled.sh ./$(BIN)

spec-tests: $(BIN)
	@MallocNanoZone=0 DISABLE_CODEGEN=1 ./tests/spec/run_ast_golden.sh ./$(BIN)

parser-tests: union-decl initializer-expr typedef-chain designated-init control-flow \
              cast-grouped for_typedef comma-decl-init function-pointer \
              compound-literal-nested-braces switch-flow goto-flow \
              statement-expr-enabled statement-expr-default statement-expr-disabled recovery

syntax-tests: semantic-typedef semantic-initializer semantic-undeclared semantic-bool \
              semantic-invalid-arith semantic-lvalue-errors semantic-pointer-errors \
              semantic-pointer-qualifier semantic-function-calls semantic-tag-conflicts \
              semantic-initializer-shapes semantic-flow semantic-goto-reachable-label \
              semantic-switch-grouped-cases \
              semantic-vla-errors semantic-vla-block \
              semantic-struct-definition-only semantic-struct-inline-declarator \
	              semantic-vla-param-decay semantic-flexible-array \
	              semantic-knr-function semantic-unsequenced semantic-restrict-alias \
	              semantic-restrict-alias-fields \
	              semantic-alignas semantic-bitfield-layout semantic-long-double-layout \
	              semantic-atomic-qualifier \
	              semantic-static-assert-member-array-size \
	              semantic-static-local-float-constexpr \
	              semantic-static-float-constexpr \
	              semantic-flexible-array-layout semantic-union-anon-member \
	              semantic-anon-record-flatten \
	              semantic-sizeof-alignof-vla \
	              semantic-case-non-ice semantic-char-escape-consteval \
              semantic-ternary-merge-ptr semantic-shift-unsigned \
              compound-literal-lvalues \
              semantic-complex-declarators

codegen-tests: pointer-arith codegen-pointer-deref codegen-pointer-diff codegen-pointer-diff-decay \
               codegen-pointer-to-array codegen-logical-short-circuit \
               codegen-function-pointer-call codegen-compound-literal-pointer-decay \
               codegen-bitfield codegen-alignas codegen-flex-memset codegen-flex-lvalue \
               codegen-flex-struct-array \
               codegen-ternary-merge codegen-shift codegen-varargs-call \
               codegen-callconv-declspec codegen-switch-dense codegen-switch-sparse \
               codegen-opaque-pointer codegen-const-globals \
               codegen-compound-literal-storage codegen-builtin-expect codegen-weak-linkage \
               statement-expr-codegen

test-binary-smoke: $(BIN)
	@BINARY_LEVEL=smoke python3 tests/binary/run_binary.py ./$(BIN)

test-binary-io: $(BIN)
	@BINARY_LEVEL=io python3 tests/binary/run_binary.py ./$(BIN)

test-binary-link: $(BIN)
	@BINARY_LEVEL=link python3 tests/binary/run_binary.py ./$(BIN)

test-binary-sdl: $(BIN)
	@BINARY_LEVEL=sdl python3 tests/binary/run_binary.py ./$(BIN)

test-binary-stdio: $(BIN)
	@BINARY_LEVEL=stdio python3 tests/binary/run_binary.py ./$(BIN)

test-binary-math: $(BIN)
	@BINARY_LEVEL=math python3 tests/binary/run_binary.py ./$(BIN)

test-binary-fortify: $(BIN)
	@BINARY_LEVEL=fortify python3 tests/binary/run_binary.py ./$(BIN)

test-binary-abi: $(BIN)
	@BINARY_LEVEL=abi python3 tests/binary/run_binary.py ./$(BIN)

test-binary-corpus: $(BIN)
	@BINARY_LEVEL=corpus python3 tests/binary/run_binary.py ./$(BIN)

test-binary-diff: $(BIN)
	@BINARY_LEVEL=diff python3 tests/binary/run_binary.py ./$(BIN)

test-binary-wave: $(BIN)
	@if [ -z "$(WAVE)" ]; then echo "ERROR: provide WAVE=<n>"; exit 2; fi
	@if [ -n "$(BINARY_WAVE_BUCKET)" ]; then \
		BINARY_MANIFEST="$(BINARY_WAVE_BUCKET)-wave$(WAVE).json" python3 tests/binary/run_binary.py ./$(BIN); \
	else \
		BINARY_MANIFEST="wave$(WAVE).json" python3 tests/binary/run_binary.py ./$(BIN); \
	fi

test-binary: test-binary-smoke test-binary-io test-binary-link test-binary-sdl test-binary-stdio test-binary-math test-binary-fortify test-binary-abi test-binary-corpus test-binary-diff

test-binary-id: $(BIN)
	@if [ -z "$(ID)" ]; then echo "ERROR: provide ID=<test_id>"; exit 2; fi
	@BINARY_FILTER="$(ID)" python3 tests/binary/run_binary.py ./$(BIN)

binary-regen: $(BIN)
	@if [ -z "$(TEST)" ]; then echo "ERROR: provide TEST=<test_id>"; exit 2; fi
	@if [ "$(CONFIRM)" != "YES" ]; then echo "ERROR: set CONFIRM=YES"; exit 2; fi
	@UPDATE_BINARY=1 BINARY_FILTER="$(TEST)" python3 tests/binary/run_binary.py ./$(BIN)

test: spec-tests parser-tests syntax-tests codegen-tests preprocessor-tests integration-diags-pack integration-std-atomic integration-std-atomic-link
preprocessor-tests: $(BIN)
	@MallocNanoZone=0 ./tests/preprocessor/run_pp_stringify_paste.sh ./$(BIN)
	@MallocNanoZone=0 ./tests/preprocessor/run_pp_variadic.sh ./$(BIN)
	@MallocNanoZone=0 ./tests/preprocessor/run_pp_expr.sh
	@MallocNanoZone=0 ./tests/preprocessor/run_pp_if.sh
	@MallocNanoZone=0 ./tests/preprocessor/run_pp_ifdef_keyword.sh ./$(BIN)
	@MallocNanoZone=0 ./tests/preprocessor/run_pp_nested.sh ./$(BIN)
	@MallocNanoZone=0 ./tests/preprocessor/run_pp_include_stringize.sh ./$(BIN)
	@MallocNanoZone=0 ./tests/preprocessor/run_pp_include_basic.sh ./$(BIN)
	@MallocNanoZone=0 ./tests/preprocessor/run_pp_include_search.sh ./$(BIN)
	@MallocNanoZone=0 ./tests/preprocessor/run_pp_include_next.sh ./$(BIN)
	@MallocNanoZone=0 ./tests/preprocessor/run_pp_include_pragma_once.sh ./$(BIN)
	@MallocNanoZone=0 ./tests/preprocessor/run_pp_preserve.sh ./$(BIN)
	@MallocNanoZone=0 ./tests/preprocessor/run_pp_deps.sh ./$(BIN)
	@./tests/preprocessor/run_pp_macro_adjacent.sh ./$(BIN)
	@./tests/preprocessor/run_pp_builtins.sh ./$(BIN)
	@./tests/preprocessor/run_pp_trigraph_digraph.sh ./$(BIN)
	@./tests/preprocessor/run_pp_macro_recursion.sh ./$(BIN)
	@./tests/preprocessor/run_pp_recursive_guard.sh ./$(BIN)
	@./tests/preprocessor/run_pp_macro_depth.sh ./$(BIN)
	@./tests/preprocessor/run_pp_system_include.sh ./$(BIN)
	@./tests/preprocessor/run_pp_external_system_include.sh ./$(BIN)
	@./tests/preprocessor/run_pp_cli_define_string_macro.sh ./$(BIN)
	@./tests/preprocessor/run_pp_cli_define_string_macro_escaped.sh ./$(BIN)
	@./tests/preprocessor/run_pp_float_limits_macro.sh ./$(BIN)
	@./tests/preprocessor/run_pp_builtin_more.sh ./$(BIN)
	@./tests/preprocessor/run_pp_pragma_stdc.sh ./$(BIN)

shim-build-shadow:
	@$(MAKE) SHIM_MODE=shadow all

shim-parse-smoke: shim-build-shadow
	@echo "Running shim parse smoke with $(BIN) (SHIM_MODE=shadow)..."
	@SYSTEM_INCLUDE_PATHS="$(SYS_SHIMS_OVERLAY):$${SYSTEM_INCLUDE_PATHS:-}" \
		./tests/preprocessor/run_pp_system_include.sh ./fisics_shim_shadow
	@SYSTEM_INCLUDE_PATHS="$(SYS_SHIMS_OVERLAY):$${SYSTEM_INCLUDE_PATHS:-}" \
		./tests/preprocessor/run_pp_include_search.sh ./fisics_shim_shadow

shim-parse-parity:
	@echo "Building baseline compiler..."
	@$(MAKE) SHIM_MODE=off all
	@echo "Building shim-shadow compiler..."
	@$(MAKE) SHIM_MODE=shadow all
	@echo "Comparing baseline vs shim-shadow system include behavior..."
	@./tests/preprocessor/run_pp_system_include.sh ./fisics
	@SYSTEM_INCLUDE_PATHS="$(SYS_SHIMS_OVERLAY):$${SYSTEM_INCLUDE_PATHS:-}" \
		./tests/preprocessor/run_pp_system_include.sh ./fisics_shim_shadow
	@./tests/preprocessor/run_pp_include_search.sh ./fisics
	@SYSTEM_INCLUDE_PATHS="$(SYS_SHIMS_OVERLAY):$${SYSTEM_INCLUDE_PATHS:-}" \
		./tests/preprocessor/run_pp_include_search.sh ./fisics_shim_shadow
	@echo "shim parity checks passed (baseline and shadow paths both valid)."

shim-parse-parity-quiet:
	@echo "Building baseline compiler..."
	@$(MAKE) SHIM_MODE=off all
	@echo "Building shim-shadow compiler..."
	@$(MAKE) SHIM_MODE=shadow all
	@./tests/preprocessor/run_pp_shim_quiet_parity.sh ./fisics ./fisics_shim_shadow "$(SYS_SHIMS_OVERLAY)" "$(SYS_SHIMS_INCLUDE)"

shim-language-profile:
	@echo "Building baseline compiler..."
	@$(MAKE) SHIM_MODE=off all
	@echo "Building shim-shadow compiler..."
	@$(MAKE) SHIM_MODE=shadow all
	@./tests/preprocessor/run_pp_shim_language_profile.sh ./fisics ./fisics_shim_shadow "$(SYS_SHIMS_OVERLAY)" "$(SYS_SHIMS_INCLUDE)" "$(SHIM_PROFILE_ID)" "$(SHIM_PROFILE_VERSION)"

shim-language-profile-negative:
	@echo "Building shim-shadow compiler..."
	@$(MAKE) SHIM_MODE=shadow all
	@./tests/preprocessor/run_pp_shim_language_profile_negative.sh ./fisics_shim_shadow "$(SYS_SHIMS_OVERLAY)" "$(SYS_SHIMS_INCLUDE)" "$(SHIM_PROFILE_ID)" "$(SHIM_PROFILE_VERSION)"

shim-s6-gate:
	@$(MAKE) shim-language-profile
	@$(MAKE) shim-language-profile-negative
	@echo "fisiCs S6 shim profile gate passed."

shim-gate:
	@$(MAKE) shim-parse-parity-quiet
	@$(MAKE) -C $(SYS_SHIMS_DIR) conformance
	@echo "fisiCs shim gate passed."

FRONTEND_TEST_SRCS := $(wildcard tests/unit/frontend_api*.c)
FRONTEND_TEST_BINS := $(patsubst tests/unit/%.c,$(BUILD_DIR)/tests/%,$(FRONTEND_TEST_SRCS))
FRONTEND_CONTRACT_TEST_SRCS := $(wildcard tests/unit/frontend_api_contract_*.c)
FRONTEND_CONTRACT_TEST_BINS := $(patsubst tests/unit/%.c,$(BUILD_DIR)/tests/%,$(FRONTEND_CONTRACT_TEST_SRCS))

$(BUILD_DIR)/tests/%: tests/unit/%.c $(LIB_FRONTEND)
	@mkdir -p $(BUILD_DIR)/tests
	$(CC) $(CFLAGS) $(INCLUDES) $< -L. -lfisics_frontend -o $@ $(LDFLAGS)

frontend-api-test: $(LIB_FRONTEND) $(FRONTEND_TEST_BINS)
	@echo "Running frontend API tests..."
	@for t in $(FRONTEND_TEST_BINS); do \
		out=$$(mktemp); \
		if ! $$t >$$out 2>&1; then \
			echo "FAIL $$t"; \
			cat $$out; \
			rm -f $$out; \
			exit 1; \
		fi; \
		rm -f $$out; \
	done; \
	echo "Frontend API tests: PASS"

frontend-contract-test: $(LIB_FRONTEND) $(FRONTEND_CONTRACT_TEST_BINS)
	@echo "Running frontend contract bucket tests..."
	@for t in $(FRONTEND_CONTRACT_TEST_BINS); do \
		out=$$(mktemp); \
		if ! $$t >$$out 2>&1; then \
			echo "FAIL $$t"; \
			cat $$out; \
			rm -f $$out; \
			exit 1; \
		fi; \
		rm -f $$out; \
	done; \
	echo "Frontend contract bucket: PASS"

HARNESS_SRC := tests/harness/frontend_harness.c
HARNESS_BIN := $(BUILD_DIR)/tests/frontend_harness

$(HARNESS_BIN): $(HARNESS_SRC) $(LIB_FRONTEND)
	@mkdir -p $(BUILD_DIR)/tests
	$(CC) $(CFLAGS) $(INCLUDES) $(HARNESS_SRC) -L. -lfisics_frontend -o $@ $(LDFLAGS)

frontend-harness: $(HARNESS_BIN)
	@$(HARNESS_BIN)

REAL_PROJECTS_STAGE_A_RUNNER := tests/real_projects/runners/run_project_compile_tests.py
REAL_PROJECTS_STAGE_B_RUNNER := tests/real_projects/runners/run_project_link_subset_tests.py
REAL_PROJECTS_STAGE_C_RUNNER := tests/real_projects/runners/run_project_full_build_tests.py
REAL_PROJECTS_STAGE_D_RUNNER := tests/real_projects/runners/run_project_runtime_smoke_tests.py
REAL_PROJECTS_STAGE_E_RUNNER := tests/real_projects/runners/run_project_golden_behavior_tests.py
REAL_PROJECTS_STAGE_F_RUNNER := tests/real_projects/runners/run_project_perf_telemetry_tests.py
REAL_PROJECT ?= datalab
REAL_PROJECT_RUNS ?= 5

realproj-stage-a:
	@python3 $(REAL_PROJECTS_STAGE_A_RUNNER) --project $(REAL_PROJECT)

realproj-stage-a-self:
	@python3 $(REAL_PROJECTS_STAGE_A_RUNNER) --project fisiCs

realproj-stage-a-repeat:
	@i=1; \
	while [ $$i -le $(REAL_PROJECT_RUNS) ]; do \
		echo "realproj-stage-a repeat $$i/$(REAL_PROJECT_RUNS) (project=$(REAL_PROJECT))"; \
		python3 $(REAL_PROJECTS_STAGE_A_RUNNER) --project $(REAL_PROJECT) || exit $$?; \
		i=$$((i + 1)); \
	done

realproj-stage-b:
	@python3 $(REAL_PROJECTS_STAGE_B_RUNNER) --project $(REAL_PROJECT)

realproj-stage-c:
	@python3 $(REAL_PROJECTS_STAGE_C_RUNNER) --project $(REAL_PROJECT)

realproj-stage-d:
	@python3 $(REAL_PROJECTS_STAGE_D_RUNNER) --project $(REAL_PROJECT)

realproj-stage-e:
	@python3 $(REAL_PROJECTS_STAGE_E_RUNNER) --project $(REAL_PROJECT)

realproj-stage-f:
	@python3 $(REAL_PROJECTS_STAGE_F_RUNNER) --project $(REAL_PROJECT)

tests: test frontend-api-test

# === Phony Targets ===
.PHONY: all clean run examples examples-hello examples-sdl \
        release-clean release-contract release-build release-stage release-manifest release-archive release-archive-zip release-archive-tgz \
        release-manifest-from-stage release-archive-zip-from-stage release-archive-tgz-from-stage \
        release-sign release-notarize release-verify release-pkg release-all \
        release-bridge-prepare release-bridge-verify release-bridge \
        union-decl initializer-expr typedef-chain designated-init control-flow \
        cast-grouped for_typedef function-pointer pointer-arith switch-flow \
        goto-flow semantic-typedef semantic-initializer semantic-undeclared \
        semantic-bool semantic-invalid-arith semantic-lvalue-errors semantic-pointer-errors \
        semantic-pointer-qualifier semantic-function-calls semantic-tag-conflicts \
        semantic-initializer-shapes semantic-flow semantic-struct-definition-only \
        semantic-vla-param-decay codegen-pointer-deref codegen-pointer-diff \
        codegen-pointer-to-array codegen-logical-short-circuit \
        compound-literal-lvalues \
        codegen-function-pointer-call codegen-compound-literal-pointer-decay \
        codegen-bitfield \
        statement-expr-enabled statement-expr-default statement-expr-disabled recovery preprocessor-tests frontend-harness \
        frontend-contract-test \
        statement-expr-codegen codegen-bitfield semantic-anon-record-flatten \
        parser-tests syntax-tests codegen-tests spec-tests test tests semantic-alignas codegen-flex-lvalue codegen-flex-struct-array \
        semantic-static-assert-member-array-size semantic-static-local-float-constexpr \
        test-binary test-binary-smoke test-binary-io test-binary-link test-binary-sdl test-binary-stdio test-binary-math test-binary-fortify test-binary-abi test-binary-corpus test-binary-diff test-binary-wave test-binary-id binary-regen \
        integration-diags-pack integration-std-atomic integration-std-atomic-link ci-guardrails \
        realproj-stage-a realproj-stage-a-self realproj-stage-a-repeat realproj-stage-b realproj-stage-c realproj-stage-d realproj-stage-e realproj-stage-f \
        shim-build-shadow shim-parse-smoke shim-parse-parity shim-parse-parity-quiet shim-language-profile shim-language-profile-negative shim-s6-gate shim-gate
