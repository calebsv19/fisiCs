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
SYS_SHIMS_DIR := ../shared/sys_shims
CORE_BASE_DIR := ../shared/core/core_base
CORE_IO_DIR := ../shared/core/core_io
CORE_DATA_DIR := ../shared/core/core_data
CORE_PACK_DIR := ../shared/core/core_pack
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

INCLUDES := -Isrc -Isrc/Lexer -Isrc/Parser -Isrc/Syntax -I$(CORE_BASE_DIR)/include -I$(CORE_IO_DIR)/include -I$(CORE_DATA_DIR)/include -I$(CORE_PACK_DIR)/include
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

integration-compile-only: $(BIN)
	@./tests/integration/compile_only.sh ./$(BIN)

integration-compile-link: $(BIN)
	@./tests/integration/compile_and_link.sh ./$(BIN)

integration-diags-pack: $(BIN)
	@./tests/integration/run_emit_diags_pack.sh ./$(BIN)

integration: integration-compile-only integration-compile-link

# Final C99 behavior suite
.PHONY: final final-update final-id final-prefix final-glob final-bucket final-manifest final-wave final-runtime
final: $(BIN)
	@python3 tests/final/run_final.py ./$(BIN)

final-update: $(BIN)
	@UPDATE_FINAL=1 python3 tests/final/run_final.py ./$(BIN)

# Exact-id slice (example: make final-id ID=14__runtime_multitu_mixed_abi_call_chain)
final-id: $(BIN)
	@if [ -z "$(ID)" ]; then echo "ERROR: provide ID=<test_id>"; exit 2; fi
	@FINAL_FILTER="$(ID)" python3 tests/final/run_final.py ./$(BIN)

# Prefix slice (example: make final-prefix PREFIX=14__)
final-prefix: $(BIN)
	@if [ -z "$(PREFIX)" ]; then echo "ERROR: provide PREFIX=<id_prefix>"; exit 2; fi
	@FINAL_PREFIX="$(PREFIX)" python3 tests/final/run_final.py ./$(BIN)

# Glob slice (example: make final-glob GLOB='14__runtime_multitu_*')
final-glob: $(BIN)
	@if [ -z "$(GLOB)" ]; then echo "ERROR: provide GLOB=<fnmatch_pattern>"; exit 2; fi
	@FINAL_GLOB="$(GLOB)" python3 tests/final/run_final.py ./$(BIN)

# Bucket slice by metadata bucket field (example: make final-bucket BUCKET=runtime-surface)
final-bucket: $(BIN)
	@if [ -z "$(BUCKET)" ]; then echo "ERROR: provide BUCKET=<bucket_name>"; exit 2; fi
	@FINAL_BUCKET="$(BUCKET)" python3 tests/final/run_final.py ./$(BIN)

# Manifest shard slice (example: make final-manifest MANIFEST=14-runtime-surface-wave43-multitu-abi-stress-promotions.json)
final-manifest: $(BIN)
	@if [ -z "$(MANIFEST)" ]; then echo "ERROR: provide MANIFEST=<manifest_name_or_token>"; exit 2; fi
	@FINAL_MANIFEST="$(MANIFEST)" python3 tests/final/run_final.py ./$(BIN)

# Wave slice (runtime-focused by default).
# Examples:
#   make final-wave WAVE=43
#   make final-wave WAVE=43 WAVE_BUCKET=14-runtime-surface
WAVE_BUCKET ?= 14-runtime-surface
final-wave: $(BIN)
	@if [ -z "$(WAVE)" ]; then echo "ERROR: provide WAVE=<number>"; exit 2; fi
	@FINAL_MANIFEST_GLOB="$(WAVE_BUCKET)-wave$(WAVE)-*.json" python3 tests/final/run_final.py ./$(BIN)

# Runtime convenience slice (all bucket-14 tests)
final-runtime: $(BIN)
	@FINAL_PREFIX="14__" python3 tests/final/run_final.py ./$(BIN)

# === Run the compiled binary ===
run: src/Lexer/keyword_lookup.c $(BIN)
	@MallocNanoZone=0 ./$(BIN)

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

statement-expr-disabled: $(BIN)
	@./tests/parser/run_statement_expr_disabled.sh ./$(BIN)

spec-tests: $(BIN)
	@MallocNanoZone=0 DISABLE_CODEGEN=1 ./tests/spec/run_ast_golden.sh ./$(BIN)

parser-tests: union-decl initializer-expr typedef-chain designated-init control-flow \
              cast-grouped for_typedef comma-decl-init function-pointer \
              compound-literal-nested-braces switch-flow goto-flow \
              statement-expr-enabled statement-expr-disabled recovery

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
	              semantic-static-float-constexpr \
	              semantic-flexible-array-layout semantic-union-anon-member \
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
               codegen-compound-literal-storage codegen-builtin-expect \
               statement-expr-codegen

test: spec-tests parser-tests syntax-tests codegen-tests preprocessor-tests integration-diags-pack
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
	@$(MAKE) -C ../shared/sys_shims conformance
	@echo "fisiCs shim gate passed."

FRONTEND_TEST_SRCS := $(wildcard tests/unit/frontend_api*.c)
FRONTEND_TEST_BINS := $(patsubst tests/unit/%.c,$(BUILD_DIR)/tests/%,$(FRONTEND_TEST_SRCS))

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

HARNESS_SRC := tests/harness/frontend_harness.c
HARNESS_BIN := $(BUILD_DIR)/tests/frontend_harness

$(HARNESS_BIN): $(HARNESS_SRC) $(LIB_FRONTEND)
	@mkdir -p $(BUILD_DIR)/tests
	$(CC) $(CFLAGS) $(INCLUDES) $(HARNESS_SRC) -L. -lfisics_frontend -o $@ $(LDFLAGS)

frontend-harness: $(HARNESS_BIN)
	@$(HARNESS_BIN)

tests: test frontend-api-test

# === Phony Targets ===
.PHONY: all clean run \
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
        statement-expr-enabled statement-expr-disabled recovery preprocessor-tests frontend-harness \
        statement-expr-codegen codegen-bitfield \
        parser-tests syntax-tests codegen-tests spec-tests test tests semantic-alignas codegen-flex-lvalue codegen-flex-struct-array \
        integration-diags-pack \
        shim-build-shadow shim-parse-smoke shim-parse-parity shim-parse-parity-quiet shim-language-profile shim-language-profile-negative shim-s6-gate shim-gate
