CXX ?= g++
CXXFLAGS ?= -O2 -Wall -Wextra -std=c++17
CPPFLAGS ?= -Iinclude
CXX_VERSION_LINE := $(shell $(CXX) --version 2>/dev/null | sed -n '1p')
GIT_COMMIT := $(shell git rev-parse HEAD 2>/dev/null || printf unknown)
GIT_BRANCH := $(shell git rev-parse --abbrev-ref HEAD 2>/dev/null || printf unknown)
GIT_DIRTY := $(shell if [ -n "$$(git status --porcelain --untracked-files=all 2>/dev/null)" ]; then printf 1; else printf 0; fi)
BENCHMARK_BUILD_CPPFLAGS := \
	-DBOIJ_BUILD_COMMIT=\"$(GIT_COMMIT)\" \
	-DBOIJ_BUILD_BRANCH=\"$(GIT_BRANCH)\" \
	-DBOIJ_BUILD_DIRTY=$(GIT_DIRTY) \
	-DBOIJ_BUILD_PROFILE=\"benchmark-release\" \
	'-DBOIJ_BUILD_COMPILER_COMMAND="$(CXX)"' \
	'-DBOIJ_BUILD_COMPILER_VERSION="$(CXX_VERSION_LINE)"' \
	'-DBOIJ_BUILD_COMPILER_FLAGS="$(CXXFLAGS)"'

BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
BIN_DIR := $(BUILD_DIR)/bin

DEFAULT_BINS := bad_one_generator parse_huge_output remove_duplicates test_program tell_which_violations find_big_ones boij_soderberg_calculator algorithm_test_driver benchmark_driver
EXTRA_BINS := L_finder quick_run find_172 foo

.PHONY: all clean $(DEFAULT_BINS) $(EXTRA_BINS) benchmark-smoke benchmark-standard \
	benchmark-headline benchmark-validate benchmark-tools-test FORCE

all: $(DEFAULT_BINS)

$(BIN_DIR) $(OBJ_DIR)/apps $(OBJ_DIR)/src $(OBJ_DIR)/benchmark:
	mkdir -p $@

$(OBJ_DIR)/apps/%.o: apps/%.cc | $(OBJ_DIR)/apps
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/src/%.o: src/%.cc | $(OBJ_DIR)/src
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

FORCE:

$(OBJ_DIR)/benchmark/benchmark_driver.o: apps/benchmark_driver.cc FORCE | $(OBJ_DIR)/benchmark
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(BENCHMARK_BUILD_CPPFLAGS) -c apps/benchmark_driver.cc -o $@

$(OBJ_DIR)/benchmark/%.o: src/%.cc FORCE | $(OBJ_DIR)/benchmark
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
boij_soderberg_calculator: $(BIN_DIR)/boij_soderberg_calculator
algorithm_test_driver: $(BIN_DIR)/algorithm_test_driver
benchmark_driver: $(BIN_DIR)/benchmark_driver

$(BIN_DIR)/bad_one_generator: $(OBJ_DIR)/apps/bad_one_generator.o $(OBJ_DIR)/src/seq_funcs.o $(OBJ_DIR)/src/binom.o $(OBJ_DIR)/src/algorithm_helpers.o | $(BIN_DIR)
	$(CXX) $^ -o $@

$(BIN_DIR)/parse_huge_output: $(OBJ_DIR)/apps/parse_huge_output.o $(OBJ_DIR)/src/seq_funcs.o $(OBJ_DIR)/src/binom.o | $(BIN_DIR)
	$(CXX) $^ -o $@

$(BIN_DIR)/remove_duplicates: $(OBJ_DIR)/apps/remove_duplicates.o $(OBJ_DIR)/src/seq_funcs.o $(OBJ_DIR)/src/binom.o | $(BIN_DIR)
	$(CXX) $^ -o $@

$(BIN_DIR)/test_program: $(OBJ_DIR)/apps/test_program.o $(OBJ_DIR)/src/seq_funcs.o $(OBJ_DIR)/src/binom.o $(OBJ_DIR)/src/test_funcs.o | $(BIN_DIR)
	$(CXX) $^ -o $@

$(BIN_DIR)/tell_which_violations: $(OBJ_DIR)/apps/tell_which_violations.o $(OBJ_DIR)/src/seq_funcs.o $(OBJ_DIR)/src/binom.o $(OBJ_DIR)/src/test_funcs.o | $(BIN_DIR)
	$(CXX) $^ -o $@

$(BIN_DIR)/L_finder: $(OBJ_DIR)/apps/L_finder.o $(OBJ_DIR)/src/seq_funcs.o $(OBJ_DIR)/src/binom.o $(OBJ_DIR)/src/test_funcs.o $(OBJ_DIR)/src/search_algorithms.o | $(BIN_DIR)
	$(CXX) $^ -o $@

$(BIN_DIR)/quick_run: $(OBJ_DIR)/apps/quick_run.o $(OBJ_DIR)/src/seq_funcs.o $(OBJ_DIR)/src/binom.o $(OBJ_DIR)/src/test_funcs.o | $(BIN_DIR)
	$(CXX) $^ -o $@

$(BIN_DIR)/find_172: $(OBJ_DIR)/apps/find_172.o $(OBJ_DIR)/src/seq_funcs.o $(OBJ_DIR)/src/binom.o $(OBJ_DIR)/src/test_funcs.o | $(BIN_DIR)
	$(CXX) $^ -o $@

$(BIN_DIR)/foo: $(OBJ_DIR)/apps/foo.o | $(BIN_DIR)
	$(CXX) $^ -o $@

$(BIN_DIR)/boij_soderberg_calculator: $(OBJ_DIR)/apps/boij_soderberg_calculator.o $(OBJ_DIR)/src/seq_funcs.o $(OBJ_DIR)/src/binom.o $(OBJ_DIR)/src/test_funcs.o $(OBJ_DIR)/src/algorithm_helpers.o | $(BIN_DIR)
	$(CXX) $^ -o $@

$(BIN_DIR)/find_big_ones: $(OBJ_DIR)/apps/find_big_ones.o $(OBJ_DIR)/src/seq_funcs.o $(OBJ_DIR)/src/binom.o $(OBJ_DIR)/src/test_funcs.o $(OBJ_DIR)/src/search_algorithms.o | $(BIN_DIR)
	$(CXX) $^ -o $@

$(BIN_DIR)/algorithm_test_driver: $(OBJ_DIR)/apps/algorithm_test_driver.o $(OBJ_DIR)/src/seq_funcs.o $(OBJ_DIR)/src/binom.o $(OBJ_DIR)/src/test_funcs.o $(OBJ_DIR)/src/algorithm_helpers.o $(OBJ_DIR)/src/search_algorithms.o $(OBJ_DIR)/src/test_harness.o | $(BIN_DIR)
	$(CXX) $^ -o $@

$(BIN_DIR)/benchmark_driver: $(OBJ_DIR)/benchmark/benchmark_driver.o $(OBJ_DIR)/benchmark/seq_funcs.o $(OBJ_DIR)/benchmark/binom.o $(OBJ_DIR)/benchmark/algorithm_helpers.o | $(BIN_DIR)
	$(CXX) $^ -o $@

benchmark-smoke: $(BIN_DIR)/benchmark_driver
	python3 benchmarks/run_benchmarks.py --profile=smoke --binary=$(BIN_DIR)/benchmark_driver --compiler=$(CXX) --compiler-flags='$(CXXFLAGS)'

benchmark-standard: $(BIN_DIR)/benchmark_driver
	python3 benchmarks/run_benchmarks.py --profile=standard --binary=$(BIN_DIR)/benchmark_driver --compiler=$(CXX) --compiler-flags='$(CXXFLAGS)'

benchmark-headline: $(BIN_DIR)/benchmark_driver
	python3 benchmarks/run_benchmarks.py --profile=headline --binary=$(BIN_DIR)/benchmark_driver --compiler=$(CXX) --compiler-flags='$(CXXFLAGS)'

benchmark-validate:
	@test -n "$(BUNDLE)" || (echo "Usage: make benchmark-validate BUNDLE=benchmarks/runs/<run-id>" >&2; exit 2)
	python3 benchmarks/validate_bundle.py $(BUNDLE)

benchmark-tools-test: $(BIN_DIR)/benchmark_driver
	python3 -m unittest discover -s benchmarks -p 'test_*.py'


clean:
	rm -rf $(BUILD_DIR)
