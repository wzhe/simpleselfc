#include "defs.h"
#include "data.h"
#include "decl.h"


static void updatesym(int slot, char *name, int type, int stype, int clas, int label, int size, int posn) {
  if (slot < 0 || slot >= NSYMBOLS)
    fatald("Invalid symbol slot number in updatesym()", slot);

  //if (Symtable[slot]) free(Symtable[slot]);
  Symtable[slot].name = strdup(name);
  Symtable[slot].type = type;
  Symtable[slot].stype = stype;
  Symtable[slot].clas = clas;
  Symtable[slot].endlabel = label;
  Symtable[slot].size = size;
  Symtable[slot].posn = posn;
}

int findglob(char *s) {
  int i;
  for (i = 0; i < Globs; i++) {
    if (*s == *Symtable[i].name && !strcmp(Symtable[i].name, s)) {
      return (i);
    }
  }
  return (-1);
}

static int newglob(void) {
  int p;
  if ((p = Globs++) >= Locls)
    fatal("Too many global symbols");
  return (p);
}

int findlocl(char *s) {
  int i;
  for (i = Locls + 1; i < NSYMBOLS; i++) {
    if (*s == *Symtable[i].name && !strcmp(Symtable[i].name, s)) {
      return (i);
    }
  }
  return (-1);
}

int findsym(char *s) {
  int i;
  i = findlocl(s);
  if (i == -1) {
    i = findglob(s);
  }
  return (i);
}

static int newlocl(void) {
  int p;
  if ((p = Locls--) <= Globs)
    fatal("Too many local symbols");
  return (p);
}

int addglob(char *name, int type, int stype, int label, int size) {
  int y;

  if ((y = findglob(name)) != -1) {
    return y;
  }

  y = newglob();
  updatesym(y,name, type, stype, C_GLOBAL, label, size, 0);

  return (y);
}

// Add a local symbol to the symbol table. Set up its:
// +type: char, int etc.
// +structurl type: var, function, array etc.
// +size: number of elements
// +endlabel: if this is a function
// Return the slot number in the symbol table
int addlocl(char *name, int type, int stype, int label, int size) {
  int slot, posn;

  if ((slot = findglob(name)) != -1) {
    return slot;
  }

  slot = newlocl();
  posn = gengetlocaloffset(type, 0);  // XXX 0 for now 
  updatesym(slot, name, type, stype, C_LOCAL, label, size, posn);
  return (slot);
}

