#include "defs.h"
#include "data.h"
#include "decl.h"


static struct symtable*  newsym(char *name, int type, struct symtable *ctype, int stype, int clas, int labelorsize, int posn) {
  struct symtable* sym = (struct symtable *)malloc(sizeof(struct symtable));
  if (sym == NULL) fatal("Bad malloc in newsym");
						      
  sym->name = strdup(name);
  sym->type = type;
  sym->stype = stype;
  sym->ctype = ctype;
  sym->clas = clas;
  sym->size = labelorsize;
  sym->posn = posn;
  sym->next = NULL;
  sym->member = NULL;

  if (clas == C_GLOBAL)
    genglobsym(sym);
  return sym;
}

static void appendsym(struct symtable **head, struct symtable **tail, struct symtable *node) {
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

struct symtable* findsyminlist(char *s, struct symtable* list) {
  while(list) {
    if ((list->name != NULL) && !strcmp(s, list->name)) {
      return list;
    }
    list = list->next;
  }
  return (NULL);
}

struct symtable* findglob(char *s) {
  struct symtable* node;
  node = findsyminlist(s, Globalhead);
  return (node);
}

struct symtable* findlocl(char *s) {
  struct symtable* node;
  if (Functionid) {
    node = findsyminlist(s, Functionid->member);
    if (node != NULL) return node;
  }
  node = findsyminlist(s, Localhead);
  return (node);
}

struct symtable* findsym(char *s) {
  struct symtable* node;
  node = findlocl(s);
  if (node != NULL) return node;
  node = findglob(s);
  return (node);
}

struct symtable* findstruct(char *s) {
  struct symtable* node;
  node = findsyminlist(s, Structhead);
  return (node);
}

struct symtable* findmember(char *s) {
  struct symtable* node;
  node = findsyminlist(s, Memberhead);
  return (node);
}

struct symtable* findunion(char *s) {
  struct symtable* node;
  node = findsyminlist(s, Unionhead);
  return (node);
}

struct symtable* findenumtype(char *s) {
  struct symtable* node = Enumhead;
  while(node) {
    if ((node->name != NULL) && !strcmp(s, node->name) && node->clas == C_ENUMTYPE) {
      return (node);
    }
    node = node->next;
  }
  return (node);
}

struct symtable* findenumval(char *s) {
  struct symtable* node = Enumhead;
  while(node) {
    if ((node->name != NULL) && !strcmp(s, node->name) && node->clas == C_ENUMVAL) {
      return (node);
    }
    node = node->next;
  }
  return (node);
}
struct symtable* findtypedef(char *s) {
  struct symtable* node;
  node = findsyminlist(s, Typedefhead);
  return (node);
}

struct symtable* addglob(char *name, int type, struct symtable *ctype, int stype, int size) {
  struct symtable *sym = newsym(name, type, ctype, stype, C_GLOBAL, size, 0);
  appendsym(&Globalhead, &Globaltail, sym);

  return (sym);
}

// Add a local symbol to the symbol table. Set up its:
// Return the slot number in the symbol table
struct symtable* addlocl(char *name, int type, struct symtable *ctype, int stype, int size) {
  struct symtable *sym = newsym(name, type, ctype, stype, C_LOCAL, size, 0);
  appendsym(&Localhead, &Localtail, sym);

  return (sym);
}

struct symtable* addparm(char *name, int type, struct symtable *ctype, int stype, int size) {
  struct symtable *sym = newsym(name, type, ctype, stype, C_PARAM, size, 0);
  appendsym(&Parmhead, &Parmtail, sym);
  return (sym);
}

struct symtable* addmember(char *name, int type, struct symtable *ctype, int stype, int size) {
  struct symtable *sym = newsym(name, type, ctype, stype, C_MEMBER, size, 0);
  appendsym(&Memberhead, &Membertail, sym);
  return (sym);
}

struct symtable* addstruct(char *name, int type, struct symtable *ctype, int stype, int size) {
  struct symtable *sym = newsym(name, type, ctype, stype, C_STRUCT, size, 0);
  appendsym(&Structhead, &Structtail, sym);
  return (sym);
}

struct symtable* addunion(char *name, int type, struct symtable *ctype, int stype, int size) {
  struct symtable *sym = newsym(name, type, ctype, stype, C_STRUCT, size, 0);
  appendsym(&Unionhead, &Uniontail, sym);
  return (sym);
}

struct symtable* addenum(char *name, int clas, int val) {
  struct symtable *sym = newsym(name, P_INT, NULL, 0, clas, 0, val);
  appendsym(&Enumhead, &Enumtail, sym);
  return (sym);
}

struct symtable* addtypedef(char *name, int type, struct symtable *ctype) {
  struct symtable *sym = newsym(name, type, ctype, 0, 0, 0, 0);
  appendsym(&Typedefhead, &Typedeftail, sym);
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

void showsym(struct symtable* sym) {
  struct symtable* m;
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
