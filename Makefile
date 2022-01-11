TARGET= ssc
SRCS= cg.c decl.c expr.c gen.c main.c misc.c scan.c stmt.c sym.c tree.c types.c

# Define the location of the include directory
INCDIR=/tmp/include_ssc
BINDIR=/tmp/

$(TARGET): $(SRCS)
	mkdir -p $(INCDIR)
	rsync -a include/. $(INCDIR)
	cc -o $(TARGET) -g -Wall -Wextra  -DINCDIR=\"$(INCDIR)\" $^

clean:
	rm -f $(TARGET) *.o a.out *.s

test: $(TARGET) tests/runtests.sh
	(cd tests; chmod +x runtests.sh; ./runtests.sh)

$(NUM): $(TARGET)
	./$(TARGET) -T -v tests/input$(NUM).c
