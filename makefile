# === Config ===
CC := cc
LLVM_CONFIG := llvm-config

LLVM_CFLAGS := $(shell $(LLVM_CONFIG) --cflags)
LLVM_LDFLAGS := $(shell $(LLVM_CONFIG) --ldflags)
LLVM_LIBS := $(shell $(LLVM_CONFIG) --libs core)

CFLAGS := -Wall -Wextra -Wpedantic -g3 -O0 -fno-omit-frame-pointer -fsanitize=address,undefined $(LLVM_CFLAGS)
LDFLAGS := -fsanitize=address,undefined $(LLVM_LDFLAGS) $(LLVM_LIBS)

INCLUDES := -Isrc -Isrc/Lexer -Isrc/Parser
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

# === Phony Targets ===
.PHONY: all clean run

