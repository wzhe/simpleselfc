#include "defs.h"
#include "data.h"
#include "decl.h"

static int Globs = 0;

int findglob(char *s) {
    int i;
    for (i = 0; i < Globs; i++) {
        if (*s == *Gsym[i].name && !strcmp(Gsym[i].name, s)) {
            return (i);
        }
    }
    return (-1);
}

static int newglob(void) {
    int p;
    if ((p = Globs++) >= NSYMBOLS)
        fatal("Too many global symbols");
    return (p);
}

int addglob(char *name, int type, int stype, int label, int size) {
    int y;

    if ((y = findglob(name)) != -1) {
        return y;
    }

    y = newglob();
    Gsym[y].name = strdup(name);
    Gsym[y].type = type;
    Gsym[y].stype = stype;
    Gsym[y].endlabel = label;
    Gsym[y].size = size;
    return y;
}
