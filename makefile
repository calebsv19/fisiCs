# === Config ===
CC := cc
LLVM_CONFIG := llvm-config

LLVM_CFLAGS := $(shell $(LLVM_CONFIG) --cflags)
LLVM_LDFLAGS := $(shell $(LLVM_CONFIG) --ldflags)
LLVM_LIBS := $(shell $(LLVM_CONFIG) --libs core)

CFLAGS := -Wall -Wextra -Wpedantic -g3 -O0 -fno-omit-frame-pointer -fsanitize=address,undefined $(LLVM_CFLAGS)
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
	@./tests/run_union_decl.sh ./$(BIN)

initializer-expr: $(BIN)
	@./tests/run_initializer_expr.sh ./$(BIN)

typedef-chain: $(BIN)
	@./tests/run_typedef_chain.sh ./$(BIN)

designated-init: $(BIN)
	@./tests/run_designated_init.sh ./$(BIN)

control-flow: $(BIN)
	@./tests/run_control_flow.sh ./$(BIN)

cast-grouped: $(BIN)
	@./tests/run_cast_grouped.sh ./$(BIN)

for_typedef: $(BIN)
	@./tests/run_for_typedef.sh ./$(BIN)

function-pointer: $(BIN)
	@./tests/run_function_pointer.sh ./$(BIN)

semantic-typedef: $(BIN)
	@./tests/run_semantic_typedef.sh ./$(BIN)

semantic-initializer: $(BIN)
	@./tests/run_semantic_initializer.sh ./$(BIN)

semantic-undeclared: $(BIN)
	@./tests/run_semantic_undeclared.sh ./$(BIN)

semantic-bool: $(BIN)
	@./tests/run_semantic_bool.sh ./$(BIN)

tests: union-decl initializer-expr typedef-chain designated-init control-flow cast-grouped for-typedef function-pointer semantic-typedef semantic-initializer semantic-undeclared semantic-bool

# === Phony Targets ===
.PHONY: all clean run union-decl initializer-expr typedef-chain designated-init control-flow cast-grouped for-typedef function-pointer semantic-typedef semantic-initializer semantic-undeclared semantic-bool tests
