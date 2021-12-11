#include "defs.h"
#include "data.h"
#include "decl.h"

int inttype(int type) {
  return ((type & 0xf) == 0);
}

int ptrtype(int type) {
  return ((type & 0xf) != 0);
}

struct ASTnode* modify_type(struct ASTnode *tree, int rtype, int op) {
  int ltype;
  int lsize, rsize;

  ltype = tree->type;;
  if (O_dumpAST)
    printf("comp type: %s, %s\n", typestr(ltype), typestr(rtype));

  // Compare scalar int types
  if (inttype(ltype) && inttype(rtype)) {

    if (ltype == rtype) return (tree);

    // Get the sizes for each type
    lsize = genprimsize(ltype);
    rsize = genprimsize(rtype);

    // Tree's size too big
    if (lsize > rsize) return (NULL);

    // Widen to the right
    if (rsize > lsize) return (mkastunary(A_WIDEN, tree, NULL, 0, rtype));

  }

  // For pointers on the left
  if (ptrtype(ltype)) {
    // OK is smae type on right and not doing a binary op
    if (op == 0 && ltype == rtype) return (tree);
  }

  // We can scale only on A_DD or A_SUBTRACT operation
  if (op == A_ADD || op == A_SUBTRACT) {
    // Left is int type, right is pointer type and the size
    // of the original type is > 1: scale the left
    if (inttype(ltype) && ptrtype(rtype)) {
      rsize = genprimsize(value_at(rtype));
      if (rsize > 1) return (mkastunary(A_SCALE, tree, NULL, rsize, rtype));
      else if (rsize == 1) return (tree);
    }
  }

  // If we get here, the types are not compatible
  return (NULL);

}

// Given a primitive type, return
// the type which is a point to it
int pointer_to(int type) {
  if ((type & 0xf) == 0xf) {
    fatald("Unrecognised in pointer_to: Type", type);
  }
  return (type + 1);
}

// Given a primitive pointer type, return
// the type which it points to
int value_at(int type) {
  if (!ptrtype(type)) {
    fatald("Type isn't a pointer", type);
  }
  return (type - 1);
}

// Given a type and a composite type pointer,
// return the size of this type in bytes
int typesize(int type, struct symtable *ctype) {
  if (type == P_STRUCT)
    return (ctype->size);
  return (genprimsize(type));
}
