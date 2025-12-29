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

CFLAGS = -Wall -Wextra -Wpedantic -g3 -O0 -fno-omit-frame-pointer -fsanitize=address,undefined $(LLVM_CFLAGS) $(CODEGEN_DEFS) $(PARSER_DEFS) -DDEFAULT_INCLUDE_PATHS=\"$(DEFAULT_INCLUDE_PATHS)\"
LDFLAGS := -fsanitize=address,undefined $(LLVM_LDFLAGS) $(LLVM_LIBS)

INCLUDES := -Isrc -Isrc/Lexer -Isrc/Parser -Isrc/Syntax
SRC_DIR := src
BUILD_DIR := build
BIN := fisics
INCLUDE_PATHS ?= include
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

OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))
LIB_FRONTEND := libfisics_frontend.a
# Include codegen in the archive so downstream IDEs don’t see undefined symbols.
FRONTEND_OBJS := $(filter-out $(BUILD_DIR)/main.o $(BUILD_DIR)/Compiler/object_emit.o,$(OBJS))
# Only keep the object emitter separate for the CLI binary.
BACKEND_OBJS := $(BUILD_DIR)/Compiler/object_emit.o

# === Default Target ===
all: $(BIN)

# Frontend-only targets
.PHONY: frontend frontend-clean frontend-rebuild
frontend: $(LIB_FRONTEND)
frontend-clean:
	@echo "Cleaning frontend library..."
	rm -f $(LIB_FRONTEND)
	rm -f $(FRONTEND_OBJS)
frontend-rebuild: frontend-clean frontend

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
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# === Clean: remove binary + build artifacts ===
clean:
	@echo "Cleaning..."
	rm -rf $(BUILD_DIR) $(BIN) $(LIB_FRONTEND)
	rm -f src/Lexer/keyword_lookup.c

integration-compile-only: $(BIN)
	@./tests/integration/compile_only.sh ./$(BIN)

integration-compile-link: $(BIN)
	@./tests/integration/compile_and_link.sh ./$(BIN)

integration: integration-compile-only integration-compile-link

# === Run the compiled binary ===
run: src/Lexer/keyword_lookup.c $(BIN)
	@./$(BIN)

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

function-pointer: $(BIN)
	@./tests/parser/run_function_pointer.sh ./$(BIN)

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

semantic-vla-errors: $(BIN)
	@./tests/syntax/run_semantic_vla_errors.sh ./$(BIN)

semantic-vla-block: $(BIN)
	@./tests/syntax/run_semantic_vla_block.sh ./$(BIN)

semantic-struct-definition-only: $(BIN)
	@./tests/syntax/run_semantic_struct_definition_only.sh ./$(BIN)

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

semantic-alignas: $(BIN)
	@./tests/syntax/run_semantic_alignas.sh ./$(BIN)

semantic-bitfield-layout: $(BIN)
	@./tests/syntax/run_semantic_bitfield_layout.sh ./$(BIN)

semantic-long-double-layout: $(BIN)
	@./tests/syntax/run_semantic_long_double_layout.sh ./$(BIN)

semantic-flexible-array-layout: $(BIN)
	@./tests/syntax/run_semantic_flexible_array_layout.sh ./$(BIN)

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

statement-expr-enabled: $(BIN)
	@./tests/parser/run_statement_expr_enabled.sh ./$(BIN)

statement-expr-disabled: $(BIN)
	@./tests/parser/run_statement_expr_disabled.sh ./$(BIN)

spec-tests: $(BIN)
	@DISABLE_CODEGEN=1 ./tests/spec/run_ast_golden.sh ./$(BIN)

parser-tests: union-decl initializer-expr typedef-chain designated-init control-flow \
              cast-grouped for_typedef function-pointer switch-flow goto-flow \
              statement-expr-enabled statement-expr-disabled recovery

syntax-tests: semantic-typedef semantic-initializer semantic-undeclared semantic-bool \
              semantic-invalid-arith semantic-lvalue-errors semantic-pointer-errors \
              semantic-pointer-qualifier semantic-function-calls semantic-tag-conflicts \
              semantic-initializer-shapes semantic-flow semantic-vla-errors semantic-vla-block \
              semantic-struct-definition-only semantic-vla-param-decay semantic-flexible-array \
              semantic-knr-function semantic-unsequenced semantic-restrict-alias \
              semantic-alignas semantic-bitfield-layout semantic-long-double-layout \
              semantic-flexible-array-layout \
              semantic-sizeof-alignof-vla \
              semantic-case-non-ice semantic-char-escape-consteval \
              semantic-ternary-merge-ptr semantic-shift-unsigned \
              compound-literal-lvalues

codegen-tests: pointer-arith codegen-pointer-deref codegen-pointer-diff \
               codegen-function-pointer-call codegen-compound-literal-pointer-decay \
               codegen-bitfield codegen-alignas codegen-flex-memset \
               codegen-ternary-merge codegen-shift codegen-varargs-call \
codegen-callconv-declspec codegen-switch-dense codegen-switch-sparse \
               codegen-opaque-pointer codegen-const-globals \
               codegen-compound-literal-storage codegen-builtin-expect \
               statement-expr-codegen

test: spec-tests parser-tests syntax-tests codegen-tests preprocessor-tests
preprocessor-tests: $(BIN)
	@./tests/preprocessor/run_pp_stringify_paste.sh ./$(BIN)
	@./tests/preprocessor/run_pp_variadic.sh ./$(BIN)
	@./tests/preprocessor/run_pp_expr.sh
	@./tests/preprocessor/run_pp_if.sh
	@./tests/preprocessor/run_pp_nested.sh ./$(BIN)
	@./tests/preprocessor/run_pp_include_basic.sh ./$(BIN)
	@./tests/preprocessor/run_pp_include_search.sh ./$(BIN)
	@./tests/preprocessor/run_pp_include_next.sh ./$(BIN)
	@./tests/preprocessor/run_pp_include_pragma_once.sh ./$(BIN)
	@./tests/preprocessor/run_pp_preserve.sh ./$(BIN)
	@./tests/preprocessor/run_pp_deps.sh ./$(BIN)
	@./tests/preprocessor/run_pp_macro_adjacent.sh ./$(BIN)
	@./tests/preprocessor/run_pp_builtins.sh ./$(BIN)
	@./tests/preprocessor/run_pp_trigraph_digraph.sh ./$(BIN)
	@./tests/preprocessor/run_pp_macro_recursion.sh ./$(BIN)
	@./tests/preprocessor/run_pp_macro_depth.sh ./$(BIN)
	@./tests/preprocessor/run_pp_system_include.sh ./$(BIN)
	@./tests/preprocessor/run_pp_builtin_more.sh ./$(BIN)
	@./tests/preprocessor/run_pp_pragma_stdc.sh ./$(BIN)

FRONTEND_TEST_SRCS := $(wildcard tests/unit/frontend_api*.c)
FRONTEND_TEST_BINS := $(patsubst tests/unit/%.c,build/tests/%,$(FRONTEND_TEST_SRCS))

build/tests/%: tests/unit/%.c $(LIB_FRONTEND)
	@mkdir -p build/tests
	$(CC) $(CFLAGS) $(INCLUDES) $< -L. -lfisics_frontend -o $@ $(LDFLAGS)

frontend-api-test: $(LIB_FRONTEND) $(FRONTEND_TEST_BINS)
	@echo "Running frontend API tests..."
	@for t in $(FRONTEND_TEST_BINS); do echo "==> $$t"; $$t; done

HARNESS_SRC := tests/harness/frontend_harness.c
HARNESS_BIN := build/tests/frontend_harness

$(HARNESS_BIN): $(HARNESS_SRC) $(LIB_FRONTEND)
	@mkdir -p build/tests
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
        compound-literal-lvalues \
        codegen-function-pointer-call codegen-compound-literal-pointer-decay \
        codegen-bitfield \
        statement-expr-enabled statement-expr-disabled recovery preprocessor-tests frontend-harness \
        statement-expr-codegen codegen-bitfield \
        parser-tests syntax-tests codegen-tests spec-tests test tests semantic-alignas
