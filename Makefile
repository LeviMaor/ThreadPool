CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -pedantic
LDFLAGS = -pthread
INCLUDES = -Iinclude
SRC_DIR = src
TEST_DIR = test
BUILD_DIR = build

# List all source files
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
TEST_SRCS := $(wildcard $(TEST_DIR)/*.cpp)

# List all object files
OBJS := $(SRCS:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
TEST_OBJS := $(TEST_SRCS:$(TEST_DIR)/%.cpp=$(BUILD_DIR)/%.o)

# The main target
all: main test_runner

# Main executable
main: $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

# Test runner
test_runner: $(filter-out $(BUILD_DIR)/main.o, $(OBJS)) $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

# Rule for compiling source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

# Rule for compiling test files
$(BUILD_DIR)/%.o: $(TEST_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

# Ensure build directory exists
$(BUILD_DIR):
	mkdir -p $@

# Clean rule
clean:
	rm -rf $(BUILD_DIR) main test_runner

.PHONY: all clean

