SRC_DIR := code
OBJ_DIR := obj
BIN_DIR := bin

CXX := g++
CXXFLAGS := -I$(SRC_DIR)

PROGRAMS := bad_one_generator parse_huge_output remove_duplicates test_program tell_which_violations find_big_ones L_finder

COMMON_OBJS := $(OBJ_DIR)/seq_funcs.o $(OBJ_DIR)/binom.o
TEST_OBJS := $(OBJ_DIR)/test_funcs.o


all: $(addprefix $(BIN_DIR)/,$(PROGRAMS))

$(PROGRAMS): %: $(BIN_DIR)/%
	@true

$(BIN_DIR)/bad_one_generator: $(OBJ_DIR)/bad_one_generator.o $(COMMON_OBJS) | $(BIN_DIR)
	$(CXX) -o $@ $^

$(BIN_DIR)/parse_huge_output: $(OBJ_DIR)/parse_huge_output.o $(COMMON_OBJS) | $(BIN_DIR)
	$(CXX) -o $@ $^

$(BIN_DIR)/remove_duplicates: $(OBJ_DIR)/remove_duplicates.o $(COMMON_OBJS) | $(BIN_DIR)
	$(CXX) -o $@ $^

$(BIN_DIR)/test_program: $(OBJ_DIR)/test_program.o $(COMMON_OBJS) $(TEST_OBJS) | $(BIN_DIR)
	$(CXX) -o $@ $^

$(BIN_DIR)/tell_which_violations: $(OBJ_DIR)/tell_which_violations.o $(COMMON_OBJS) $(TEST_OBJS) | $(BIN_DIR)
	$(CXX) -o $@ $^

$(BIN_DIR)/L_finder: $(OBJ_DIR)/L_finder.o $(COMMON_OBJS) $(TEST_OBJS) | $(BIN_DIR)
	$(CXX) -o $@ $^

$(BIN_DIR)/find_big_ones: $(OBJ_DIR)/find_big_ones.o $(COMMON_OBJS) | $(BIN_DIR)
	$(CXX) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cc | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

.PHONY: all clean $(PROGRAMS)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
