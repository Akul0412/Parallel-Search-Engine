# Detect OS and set appropriate compiler/flags
UNAME_S := $(shell uname -s)

# Check if MPI is requested
ifdef MPI
    # Use MPI compiler
    CC = mpicc
    ifeq ($(UNAME_S),Darwin)
        # macOS with libomp
        ifeq ($(shell test -d /opt/homebrew && echo "yes"),yes)
            LIBOMP_PATH := $(shell /opt/homebrew/bin/brew --prefix libomp 2>/dev/null || echo "/opt/homebrew/opt/libomp")
        else
            LIBOMP_PATH := $(shell brew --prefix libomp 2>/dev/null || echo "/usr/local/opt/libomp")
        endif
        CFLAGS = -Wall -Wextra -std=c11 -O2 -Xpreprocessor -fopenmp -I$(LIBOMP_PATH)/include -DUSE_MPI
        LDFLAGS = -L$(LIBOMP_PATH)/lib -lomp
    else
        # Linux with MPI
        CFLAGS = -Wall -Wextra -std=c11 -O2 -fopenmp -DUSE_MPI
        LDFLAGS = -fopenmp -lm
    endif
else
    # Standard OpenMP build (no MPI)
    ifeq ($(UNAME_S),Darwin)
        # macOS with libomp - try to find libomp path
        # Check both /opt/homebrew (ARM64) and /usr/local (x86_64)
        ifeq ($(shell test -d /opt/homebrew && echo "yes"),yes)
            LIBOMP_PATH := $(shell /opt/homebrew/bin/brew --prefix libomp 2>/dev/null || echo "/opt/homebrew/opt/libomp")
        else
            LIBOMP_PATH := $(shell brew --prefix libomp 2>/dev/null || echo "/usr/local/opt/libomp")
        endif
        CC = clang
        CFLAGS = -Wall -Wextra -std=c11 -O2 -Xpreprocessor -fopenmp -I$(LIBOMP_PATH)/include
        LDFLAGS = -L$(LIBOMP_PATH)/lib -lomp
    else
        # Linux
        CC = gcc
        CFLAGS = -Wall -Wextra -std=c11 -O2 -fopenmp
        LDFLAGS = -fopenmp -lm
    endif
endif

# Directories
SRC_DIR = src
BUILD_DIR = build
DOCS_DIR = documents
WEB_DIR = web

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

STRING_MATCH_TARGET = $(BUILD_DIR)/string_match

# Default target
all: $(STRING_MATCH_TARGET)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build string matching executable
$(STRING_MATCH_TARGET): $(BUILD_DIR)/string_match.o $(BUILD_DIR)/search_engine_string_match.o | $(BUILD_DIR)
	$(CC) $(BUILD_DIR)/string_match.o $(BUILD_DIR)/search_engine_string_match.o -o $(STRING_MATCH_TARGET) $(LDFLAGS)

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

# Create sample documents directory
setup:
	mkdir -p $(DOCS_DIR)
	@echo "Sample documents directory created. Add your text files here."

# Run the search engine
run: $(TARGET)
	./$(TARGET)

.PHONY: all clean setup run

