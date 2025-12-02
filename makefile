# === Config ===
CC := cc
LLVM_CONFIG := llvm-config

LLVM_CFLAGS := $(shell $(LLVM_CONFIG) --cflags)
LLVM_LDFLAGS := $(shell $(LLVM_CONFIG) --ldflags)
LLVM_LIBS := $(shell $(LLVM_CONFIG) --libs core)

CODEGEN_DEBUG ?= 0
CODEGEN_TRACE ?= 0
PARSER_DEBUG ?= 0
DISABLE_CODEGEN ?= 1

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

CFLAGS := -Wall -Wextra -Wpedantic -g3 -O0 -fno-omit-frame-pointer -fsanitize=address,undefined $(LLVM_CFLAGS) $(CODEGEN_DEFS) $(PARSER_DEFS)
LDFLAGS := -fsanitize=address,undefined $(LLVM_LDFLAGS) $(LLVM_LIBS)

INCLUDES := -Isrc -Isrc/Lexer -Isrc/Parser -Isrc/Syntax
SRC_DIR := src
BUILD_DIR := build
BIN := compiler

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

OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

# === Default Target ===
all: $(BIN)

# === Link object files into final binary ===
$(BIN): $(OBJS)
	@echo "Linking $@..."
	$(CC) $^ -o $@ $(LDFLAGS)

# === Compile each .c file into .o only if changed ===
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# === Clean: remove binary + build artifacts ===
clean:
	@echo "Cleaning..."
	rm -rf $(BUILD_DIR) $(BIN)
	rm -f src/Lexer/keyword_lookup.c

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

compound-literal-lvalues: $(BIN)
	@./tests/syntax/run_compound_literal_lvalues.sh ./$(BIN)

statement-expr-enabled: $(BIN)
	@./tests/parser/run_statement_expr_enabled.sh ./$(BIN)

statement-expr-disabled: $(BIN)
	@./tests/parser/run_statement_expr_disabled.sh ./$(BIN)

spec-tests: $(BIN)
	@./tests/spec/run_ast_golden.sh ./$(BIN)

parser-tests: union-decl initializer-expr typedef-chain designated-init control-flow \
              cast-grouped for_typedef function-pointer switch-flow goto-flow \
              statement-expr-enabled statement-expr-disabled recovery

syntax-tests: semantic-typedef semantic-initializer semantic-undeclared semantic-bool \
              semantic-invalid-arith semantic-lvalue-errors semantic-pointer-errors \
              semantic-pointer-qualifier semantic-function-calls semantic-tag-conflicts \
              semantic-initializer-shapes semantic-flow semantic-vla-errors semantic-vla-block \
              compound-literal-lvalues

codegen-tests: pointer-arith codegen-pointer-deref codegen-pointer-diff statement-expr-codegen

test: spec-tests parser-tests syntax-tests codegen-tests

tests: test

# === Phony Targets ===
.PHONY: all clean run \
        union-decl initializer-expr typedef-chain designated-init control-flow \
        cast-grouped for_typedef function-pointer pointer-arith switch-flow \
        goto-flow semantic-typedef semantic-initializer semantic-undeclared \
        semantic-bool semantic-invalid-arith semantic-lvalue-errors semantic-pointer-errors \
        semantic-pointer-qualifier semantic-function-calls semantic-tag-conflicts \
        semantic-initializer-shapes semantic-flow codegen-pointer-deref codegen-pointer-diff \
        compound-literal-lvalues \
        statement-expr-enabled statement-expr-disabled recovery \
        statement-expr-codegen \
        parser-tests syntax-tests codegen-tests spec-tests test tests
