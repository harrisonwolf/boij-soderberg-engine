CXX ?= g++
CXXFLAGS ?= -O2 -Wall -Wextra
CPPFLAGS ?= -Iinclude

BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
BIN_DIR := $(BUILD_DIR)/bin

DEFAULT_BINS := bad_one_generator parse_huge_output remove_duplicates test_program tell_which_violations find_big_ones
EXTRA_BINS := L_finder quick_run find_172 foo

.PHONY: all clean $(DEFAULT_BINS) $(EXTRA_BINS)

all: $(DEFAULT_BINS)

$(BIN_DIR) $(OBJ_DIR)/apps $(OBJ_DIR)/src:
	mkdir -p $@

$(OBJ_DIR)/apps/%.o: apps/%.cc | $(OBJ_DIR)/apps
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/src/%.o: src/%.cc | $(OBJ_DIR)/src
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

bad_one_generator: $(BIN_DIR)/bad_one_generator
parse_huge_output: $(BIN_DIR)/parse_huge_output
remove_duplicates: $(BIN_DIR)/remove_duplicates
test_program: $(BIN_DIR)/test_program
tell_which_violations: $(BIN_DIR)/tell_which_violations
find_big_ones: $(BIN_DIR)/find_big_ones
L_finder: $(BIN_DIR)/L_finder
quick_run: $(BIN_DIR)/quick_run
find_172: $(BIN_DIR)/find_172
foo: $(BIN_DIR)/foo

$(BIN_DIR)/bad_one_generator: $(OBJ_DIR)/apps/bad_one_generator.o $(OBJ_DIR)/src/seq_funcs.o $(OBJ_DIR)/src/binom.o | $(BIN_DIR)
	$(CXX) $^ -o $@

$(BIN_DIR)/parse_huge_output: $(OBJ_DIR)/apps/parse_huge_output.o $(OBJ_DIR)/src/seq_funcs.o $(OBJ_DIR)/src/binom.o | $(BIN_DIR)
	$(CXX) $^ -o $@

$(BIN_DIR)/remove_duplicates: $(OBJ_DIR)/apps/remove_duplicates.o $(OBJ_DIR)/src/seq_funcs.o $(OBJ_DIR)/src/binom.o | $(BIN_DIR)
	$(CXX) $^ -o $@

$(BIN_DIR)/test_program: $(OBJ_DIR)/apps/test_program.o $(OBJ_DIR)/src/seq_funcs.o $(OBJ_DIR)/src/binom.o $(OBJ_DIR)/src/test_funcs.o | $(BIN_DIR)
	$(CXX) $^ -o $@

$(BIN_DIR)/tell_which_violations: $(OBJ_DIR)/apps/tell_which_violations.o $(OBJ_DIR)/src/seq_funcs.o $(OBJ_DIR)/src/binom.o $(OBJ_DIR)/src/test_funcs.o | $(BIN_DIR)
	$(CXX) $^ -o $@

$(BIN_DIR)/find_big_ones: $(OBJ_DIR)/apps/find_big_ones.o $(OBJ_DIR)/src/seq_funcs.o $(OBJ_DIR)/src/binom.o | $(BIN_DIR)
	$(CXX) $^ -o $@

$(BIN_DIR)/L_finder: $(OBJ_DIR)/apps/L_finder.o $(OBJ_DIR)/src/seq_funcs.o $(OBJ_DIR)/src/binom.o $(OBJ_DIR)/src/test_funcs.o | $(BIN_DIR)
	$(CXX) $^ -o $@

$(BIN_DIR)/quick_run: $(OBJ_DIR)/apps/quick_run.o $(OBJ_DIR)/src/seq_funcs.o $(OBJ_DIR)/src/binom.o $(OBJ_DIR)/src/test_funcs.o | $(BIN_DIR)
	$(CXX) $^ -o $@

$(BIN_DIR)/find_172: $(OBJ_DIR)/apps/find_172.o $(OBJ_DIR)/src/seq_funcs.o $(OBJ_DIR)/src/binom.o $(OBJ_DIR)/src/test_funcs.o | $(BIN_DIR)
	$(CXX) $^ -o $@

$(BIN_DIR)/foo: $(OBJ_DIR)/apps/foo.o | $(BIN_DIR)
	$(CXX) $^ -o $@

clean:
	rm -rf $(BUILD_DIR)
