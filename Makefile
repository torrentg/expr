#-D_POSIX_C_SOURCE=200809L -Wpedantic 
# -Wsuggest-attribute=pure -Wsuggest-attribute=const -Wsuggest-attribute=noreturn -Wsuggest-attribute=format -Wsuggest-attribute=malloc
CFLAGS= -std=c11 -Wall -Wextra -Wnull-dereference -D_DEFAULT_SOURCE
LDFLAGS= -lm

all: tests

tests: expr.h expr.c tests.c
	$(CC) -g -O0 $(CFLAGS) -DRUNNING_ON_VALGRIND -o tests tests.c $(LDFLAGS)

coverage: tests.c
	$(CC) --coverage -O0 $(CFLAGS) -o tests-coverage tests.c -lgcov $(LDFLAGS)
	./tests-coverage
	[ -d coverage ] || mkdir coverage
	lcov --no-external -d . -o coverage/coverage.info -c
	genhtml -o coverage coverage/coverage.info

performance: expr.h expr.c performance.c
	$(CC) -O2 -DNDEBUG $(CFLAGS) -o performance expr.c performance.c $(LDFLAGS)
	./performance tmp/dataset/data.csv

profiler: expr.h expr.c performance.c
	$(CC) -pg -DNDEBUG $(CFLAGS) -o profiler expr.c performance.c $(LDFLAGS)
	rm -f gmon.out && ./profiler tmp/dataset/data.csv > /dev/null && gprof profiler gmon.out > profiler.gmon

valgrind: tests
	valgrind --tool=memcheck --leak-check=yes ./tests

cppcheck: expr.h
	cppcheck --enable=all --language=c -D__GNUC__ -DDEBUG --check-level=exhaustive --suppress=missingIncludeSystem --suppress=unusedFunction --suppress=checkersReport expr.c

loc:
	cloc expr.h expr.c tests.c

clean: 
	rm -f tests
	rm -f tests-coverage
	rm -f *.gcda *.gcno
	rm -rf coverage/
	rm -f performance
	rm -f profiler
	rm -f gmon.out
	rm -f profiler.gmon
	rm -f perf.data

