TARGET= ssc
SRCS= cg.c decl.c expr.c gen.c main.c misc.c scan.c stmt.c sym.c tree.c types.c
$(TARGET): $(SRCS)
	cc -o $(TARGET) -g -Wall -Wextra $^

clean:
	rm -f $(TARGET) *.o out *.s

test: $(TARGET) tests/runtests.sh
	(cd tests; chmod +x runtests.sh; ./runtests.sh)

$(NUM): $(TARGET)
	./$(TARGET) -T -v -o out tests/input$(NUM).c
