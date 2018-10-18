SOURCES = stack.c node.c parse.c eval.c htable.c string.c
TESTS = test_stack.c
BIN = tvle
CFLAGS = -O0

.PHONY: run clean all
all: $(BIN)
run: $(BIN)
	./$^
$(BIN): $(SOURCES)
	gcc $(CFLAGS) -g -o $@ $^
test_stack: test_stack.c node.c stack.c htable.c
	gcc $(CFLAGS) -g -o test $^
	./test
test_vec: test_vec.c
	gcc $(CFLAGS) -g -o test $^
	./test
clean:
	rm $(BIN)
tgz:
	tar czvf tvle.tgz *.c *.h Makefile
