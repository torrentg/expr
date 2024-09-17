CFLAGS := -std=c11 -Wall -Wextra -Wnull-dereference -D_DEFAULT_SOURCE
LDFLAGS := -lm
SRC_DIR := src
TEST_DIR := test
BUILD_DIR := build
EXAMPLES_DIR := examples

all: tests examples

$(BUILD_DIR):
	mkdir -p $@

.PHONY: tests
tests: $(BUILD_DIR) $(BUILD_DIR)/tests
$(BUILD_DIR)/tests: $(SRC_DIR)/expr.h  $(SRC_DIR)/expr.c  $(TEST_DIR)/tests.c
	$(CC) -g -O0 $(CFLAGS) -I$(SRC_DIR) -DRUNNING_ON_VALGRIND -o $@ $(TEST_DIR)/tests.c $(LDFLAGS)

.PHONY: examples
examples: $(BUILD_DIR) $(BUILD_DIR)/basic $(BUILD_DIR)/calc
$(BUILD_DIR)/basic: $(SRC_DIR)/expr.h  $(SRC_DIR)/expr.c  $(EXAMPLES_DIR)/basic.c
	$(CC) -g $(CFLAGS) -I$(SRC_DIR) -o $@ $(EXAMPLES_DIR)/basic.c $(SRC_DIR)/expr.c $(LDFLAGS)
$(BUILD_DIR)/calc: $(SRC_DIR)/expr.h  $(SRC_DIR)/expr.c  $(EXAMPLES_DIR)/calc.c
	$(CC) -g $(CFLAGS) -I$(SRC_DIR) -o $@ $(EXAMPLES_DIR)/calc.c $(SRC_DIR)/expr.c $(EXAMPLES_DIR)/linenoise.c $(LDFLAGS)

.PHONY: coverage
coverage: $(BUILD_DIR) $(BUILD_DIR)/tests-coverage
$(BUILD_DIR)/tests-coverage: $(SRC_DIR)/expr.h  $(SRC_DIR)/expr.c  $(TEST_DIR)/tests.c
	$(CC) --coverage -O0 $(CFLAGS) -I$(SRC_DIR) -o $@ $(TEST_DIR)/tests.c -lgcov $(LDFLAGS)
	cd $(BUILD_DIR); [ -d coverage ] || mkdir coverage
	cd $(BUILD_DIR); ./tests-coverage
	cd $(BUILD_DIR); lcov -b ../ -d ./ -o coverage/coverage.info -c
	cd $(BUILD_DIR); genhtml -o coverage coverage/coverage.info
	echo "See results in file $(BUILD_DIR)/coverage/index.html"

.PHONY: performance
performance: $(BUILD_DIR) $(BUILD_DIR)/performance
$(BUILD_DIR)/performance: $(SRC_DIR)/expr.h  $(SRC_DIR)/expr.c  $(TEST_DIR)/performance.c
	$(CC) -O2 -DNDEBUG $(CFLAGS) -I$(SRC_DIR) -o $@ $(SRC_DIR)/expr.c $(TEST_DIR)/performance.c $(LDFLAGS)
	$(BUILD_DIR)/performance tmp/dataset/data.csv

.PHONY: profiler
profiler: $(BUILD_DIR) $(BUILD_DIR)/profiler
$(BUILD_DIR)/profiler: $(SRC_DIR)/expr.h  $(SRC_DIR)/expr.c  $(TEST_DIR)/performance.c
	$(CC) -pg -DNDEBUG $(CFLAGS) -I$(SRC_DIR) -o $@ $(SRC_DIR)/expr.c $(TEST_DIR)/performance.c $(LDFLAGS)
	cd $(BUILD_DIR); rm -f gmon.out profiler.gmon
	cd $(BUILD_DIR); ./profiler ../tmp/dataset/data.csv > /dev/null
	cd $(BUILD_DIR); gprof ./profiler gmon.out > profiler.gmon
	echo "See results in file $(BUILD_DIR)/profiler.gmon"

.PHONY: valgrind
valgrind: tests
	valgrind --tool=memcheck --leak-check=yes $(BUILD_DIR)/tests

.PHONY: cppcheck
cppcheck:
	cppcheck --enable=all --language=c --check-level=exhaustive --suppress=missingIncludeSystem --suppress=unusedFunction --suppress=checkersReport -I$(SRC_DIR) $(SRC_DIR)/expr.c

.PHONY: loc
loc:
	cloc $(SRC_DIR)

.PHONY: clean
clean: 
	rm -rvf $(BUILD_DIR)/*
