#include "defs.h"
#include "data.h"
#include "decl.h"


static void updatesym(int slot, char *name, int type, int stype, int clas, int label, int size, int posn) {
  if (slot < 0 || slot >= NSYMBOLS)
    fatald("Invalid symbol slot number in updatesym()", slot);

  if (Symtable[slot].name != NULL) free(Symtable[slot].name);

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
    if (Symtable[i].clas == C_PARAM) continue;
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

int addglob(char *name, int type, int stype, int clas, int label, int size) {
  int slot;

  if ((slot = findglob(name)) != -1) {
    return slot;
  }

  slot = newglob();
  updatesym(slot,name, type, stype, clas, label, size, 0);
  if (clas == C_GLOBAL) genglobsym(slot);

  return (slot);
}

// Add a local symbol to the symbol table. Set up its:
// Return the slot number in the symbol table
int addlocl(char *name, int type, int stype, int clas, int size) {
  int localslot;

  if ((localslot = findglob(name)) != -1) {
    return localslot;
  }

  localslot = newlocl();
  updatesym(localslot, name, type, stype, clas, 0, size, 0);
  return (localslot);
}


void freelocalsym(){
  Locls = NSYMBOLS - 1;
}

// Given a function's slot number, copy the global parameters
// from its prototype to be local parameters
void copyfuncparams(int slot) {
  int i, id = slot + 1;
  for (i = 0; i < Symtable[slot].nelems; i++, id++) {
    addlocl(Symtable[id].name, Symtable[id].type, Symtable[id].stype, Symtable[id].clas, Symtable[id].size);
  }
}
