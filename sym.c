#include "defs.h"
#include "data.h"
#include "decl.h"


static struct symtables*  newsym(char *name, int type, int stype, int clas, int labelorsize, int posn) {
  struct symtables* sym = (struct symtables *)malloc(sizeof(struct symtables));
  if (sym == NULL) fatal("Bad malloc in newsym");
						      
  sym->name = strdup(name);
  sym->type = type;
  sym->stype = stype;
  sym->clas = clas;
  sym->size = labelorsize;
  sym->posn = posn;
  sym->next = NULL;
  sym->member = NULL;

  if (clas == C_GLOBAL)
    genglobsym(sym);
  return sym;
}

static void appendsym(struct symtables **head, struct symtables **tail, struct symtables *node) {
  if (head == NULL || tail == NULL || node == NULL) {
    fatal("Either head, tail or node is NULL in appends");
  }

  if (*tail) {
    (*tail)->next = node;
    *tail = node;
  } else {
    *head = *tail = node;
  }
  node->next = NULL;
}

struct symtables* findsyminlist(char *s, struct symtables* list) {
  while(list) {
    if ((list->name != NULL) && !strcmp(s, list->name)) {
      return list;
    }
    list = list->next;
  }
  return (NULL);
}

struct symtables* findglob(char *s) {
  struct symtables* node;
  node = findsyminlist(s, Globalhead);
  return (node);
}

struct symtables* findlocl(char *s) {
  struct symtables* node;
  if (Functionid) {
    node = findsyminlist(s, Functionid->member);
    if (node != NULL) return node;
  }
  node = findsyminlist(s, Localhead);
  return (node);
}

struct symtables* findsym(char *s) {
  struct symtables* node;
  node = findlocl(s);
  if (node != NULL) return node;
  node = findglob(s);
  return (node);
}

struct symtables* addglob(char *name, int type, int stype, int clas, int size) {
  struct symtables *sym = newsym(name, type, stype, clas, size, 0);
  appendsym(&Globalhead, &Globaltail, sym);

  return (sym);
}

// Add a local symbol to the symbol table. Set up its:
// Return the slot number in the symbol table
struct symtables* addlocl(char *name, int type, int stype, int clas, int size) {
  struct symtables *sym = newsym(name, type, stype, clas, size, 0);
  appendsym(&Localhead, &Localtail, sym);

  return (sym);
}

struct symtables* addparm(char *name, int type, int stype, int clas, int size) {
  struct symtables *sym = newsym(name, type, stype, clas, size, 0);
  appendsym(&Parmhead, &Parmtail, sym);
  return (sym);
}

void freelocalsym(){
  Localhead = Localtail = NULL;
  Parmhead = Parmtail = NULL;
  Functionid = NULL;
}

void clear_symtable() {
  Globalhead = Globaltail = NULL;
  Localhead = Localtail = NULL;
  Parmhead = Parmtail = NULL;
  Functionid = NULL;
}

void showsym(struct symtables* sym) {
  struct symtables* m;
  if (sym == NULL) {
    printf("null\n");
    return;
  }
  int first = 1;
  while(sym) {
    if (!first) {
      printf("\n->");
    } else {
      first = 0;
    }
    printf("sym:%s ", sym->name);
    if (sym->member) {
      m = sym->member;
      printf("member:");
      while(m) {
	printf("m:%s ", m->name);
	m = m->next;
      }
      printf("\n");
    }
    sym = sym->next;
  }

}
