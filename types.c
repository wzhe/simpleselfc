#include "defs.h"
#include "data.h"
#include "decl.h"

int inttype(int type) {
    switch (type) {
        case P_CHAR:
        case P_INT:
        case P_LONG:
            return (1);
    }
    return (0);
}

static int ptrtype(int type) {
    switch (type) {
        case P_VOIDPTR:
        case P_CHARPTR:
        case P_INTPTR:
        case P_LONGPTR:
            return (1);
    }
    return (0);

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
        if (rsize > lsize) return (mkastunary(A_WIDEN, tree, 0, rtype));

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
            if (rsize > 1) return (mkastunary(A_SCALE, tree, rsize, rtype));
            else if (rsize == 1) return (tree);
        }
    }

    // If we get here, the types are not compatible
    return (NULL);

}



// Given two primitive types,
// return true if they are compatible, or false otherwise.
// Also return either zero or an A_WIDEN operation
// if one has to be widened to match the other.
// If onlyright is true, olny widen left to right.
int type_compatible(int *left, int  *right, int onlyright) {
    #if Debug
    printf("comp type: %s, %s\n", typestr(*left), typestr(*right));
    #endif

    int leftsize, rightsize;

    // Void not compatible with anything
    if ((*left) == P_VOID || (*right) == P_VOID) return (0);

    // Same types, they are compatible
    if (*left == *right) {
        *left = *right = 0;
        return (1);
    }

    leftsize = genprimsize(*left);
    rightsize = genprimsize(*right);

    // Types with zero size are not compatible with anything
    if (leftsize == 0 || rightsize == 0) return (0);

    // Widen P_CHAR to P_INT as required
    if (leftsize < rightsize) {
        *left = A_WIDEN;
        *right = 0;
        return (1);
    }
    if (rightsize < leftsize) {
        if (onlyright) return (0);
        *left = 0;
        *right = A_WIDEN;
        return (1);
    }

    // Anything remaining is compatible
    *left = *right = 0;
    return (1);

}

// Given a primitive type, return
// the type which is a point to it
int pointer_to(int type) {
    int newtype;
    switch (type) {
        case P_VOID:
            newtype = P_VOIDPTR;
            break;
        case P_CHAR:
            newtype = P_CHARPTR;
            break;
        case P_INT:
            newtype = P_INTPTR;
            break;
        case P_LONG:
            newtype = P_LONGPTR;
            break;
        default:
            fatald("Unrecoginsed in pointer_to: type", type);
    }

    return (newtype);
}

// Given a primitive pointer type, return
// the type which it points to
int value_at(int type) {
    int newtype;
    switch (type) {
        case P_VOIDPTR:
            newtype = P_VOID;
            break;
        case P_CHARPTR:
            newtype = P_CHAR;
            break;
        case P_INTPTR:
            newtype = P_INT;
            break;
        case P_LONGPTR:
            newtype = P_LONG;
            break;
        default:
            fatald("Unrecoginsed in value_at: type", type);
    }

    return (newtype);
}
