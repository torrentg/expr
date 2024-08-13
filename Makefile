#-D_POSIX_C_SOURCE=200809L -Wpedantic 
CFLAGS= -std=c11 -Wall -Wextra -Wnull-dereference -D_DEFAULT_SOURCE

all: tests

tests: expr.h expr.c tests.c
	$(CC) -g -O0 $(CFLAGS) -DRUNNING_ON_VALGRIND -o tests tests.c $(LDFLAGS)

coverage: tests.c
	$(CC) --coverage -O0 $(CFLAGS) -o tests-coverage tests.c -lgcov $(LDFLAGS)
	./tests-coverage
	[ -d coverage ] || mkdir coverage
	lcov --no-external -d . -o coverage/coverage.info -c
	genhtml -o coverage coverage/coverage.info

valgrind: tests
	valgrind --tool=memcheck --leak-check=yes ./tests

cppcheck: expr.h
	cppcheck --enable=all  --suppress=missingIncludeSystem --suppress=unusedFunction --suppress=checkersReport expr.c

loc:
	cloc expr.h expr.c tests.c

clean: 
	rm -f tests
	rm -f tests-coverage
	rm -f *.gcda *.gcno
	rm -rf coverage/
