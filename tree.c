#include "defs.h"
#include "decl.h"
#include "data.h"

// Build and return a generic AST node
struct ASTnode* mkastnode(int op, struct ASTnode *left, struct ASTnode* mid,
			  struct ASTnode *right, struct symtable *sym,int intvalue, int type){
  struct ASTnode *n;

  n = (struct ASTnode*) malloc(sizeof(struct ASTnode));
  if (n == NULL) {
    fatal("Unable to malloc in mkastnode()\n");
  }

  n->op = op;
  n->type = type;
  n->rvalue = 0;
  n->left = left;
  n->mid = mid;
  n->right = right;
  n->sym = sym;
  n->intvalue = intvalue;
  return (n);

}

// Make an AST leaf node
struct ASTnode* mkastleaf(int op, struct symtable *sym, int intvalue, int type) {
  return (mkastnode(op,NULL, NULL, NULL, sym, intvalue, type));
}

// Make a unary AST node: only one child
struct ASTnode* mkastunary(int op, struct ASTnode *left, struct symtable* sym, int intvalue,int type) {
  return (mkastnode(op, left, NULL, NULL, sym, intvalue, type));
}

void freeast(struct ASTnode* node) {
  if (node == NULL) return;
  freeast(node->left);
  freeast(node->mid);
  freeast(node->right);

  free(node);
}

int havechild(struct ASTnode* tree) {
  return tree->left || tree->right || tree->mid;
}


void shownode(struct ASTnode* node,
	      char* prefix, char* childprefix) {
  char *buf = NULL;
  char *bufchild = NULL;
  int buflen = 0;
  printf("%s", prefix);

  if (node == NULL) {
    printf("null\n");
    return;
  }

  printf("%s", asttypestr(node->op));
  if (node->op == A_INTLIT) {
    printf(" v:%d", node->intvalue);
  } else if (node->op == A_IDENT
	     || node->op == A_FUNCTION
	     || node->op == A_FUNCCALL
	     || node->op == A_ADDR
	     ) {
    printf(" name:%s", node->sym->name);
    printf(" clas:%d", node->sym->clas);
  }
  printf(" size:%d", node->size);
  printf(" type:%s", typestr(node->type));
  printf(" %s", node->rvalue ? "rvalue" : "lvalue");
  printf("\n");
  if (havechild(node)){
    buflen = strlen(childprefix) + sizeof("|----");
    buf = (char *)malloc(buflen);
    if (buf == NULL) fatal("Bad malloc");
    sprintf(buf, "%s%s", childprefix, "|----");
    bufchild = (char *)malloc(buflen);
    if (bufchild == NULL) fatal("Bad malloc");
    sprintf(bufchild, "%s%s", childprefix, "|    ");
    if (node->left)
      shownode(node->left, buf, bufchild);
    if (node->mid)
      shownode(node->mid, buf, bufchild);
    if (node->right)
      shownode(node->right, buf, bufchild);
    free(buf);
    free(bufchild);
  }
}

void show(struct ASTnode* tree) {
  shownode(tree, "", "");
}
