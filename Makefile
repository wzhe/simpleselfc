TARGET= comp1
SRCS= cg.c decl.c expr.c gen.c main.c misc.c scan.c stmt.c sym.c tree.c types.c
$(TARGET): $(SRCS)
	cc -o $(TARGET) -g -Wall -Wextra $^

clean:
	rm -f $(TARGET) *.o out out.s

test: $(TARGET) tests/runtests.sh
	(cd tests; chmod +x runtests.sh; ./runtests.sh)

test22: $(TARGET)
	./$(TARGET) -T tests/input22.c
	cc -o out out.s lib/printint.c
	./out
