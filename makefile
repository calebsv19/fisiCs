# === Config ===
CC := cc
LLVM_CONFIG := llvm-config

LLVM_CFLAGS := $(shell $(LLVM_CONFIG) --cflags)
LLVM_LDFLAGS := $(shell $(LLVM_CONFIG) --ldflags)
LLVM_LIBS := $(shell $(LLVM_CONFIG) --libs core)

CFLAGS := -Wall -Wextra -Wpedantic -g -O0 $(LLVM_CFLAGS)
LDFLAGS := $(LLVM_LDFLAGS) $(LLVM_LIBS)

INCLUDES := -Isrc -Isrc/Lexer -Isrc/Parser
SRC_DIR := src
BUILD_DIR := build
BIN := compiler


# === Find all .c files recursively ===
SRCS := $(shell find $(SRC_DIR) -name '*.c')
OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

# === Default Target ===
all: $(BIN)

# === Link all object files into the final binary ===
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

# === Run the compiled binary ===
run: $(BIN)
	@./$(BIN)

# === Phony Targets ===
.PHONY: all clean run

