#include "defs.h"
#include "data.h"
#include "decl.h"

// Generate and return a new label number
int genlabel(void) {
    static int id = 1;
    return (id++);
}

//     perform the opposite comparison
//     jump to L1 if true
//     perform the first block of code
//     jump to L2
// L1:
//     perform the other block of code
// L2:
//
// Generate the code for an IF statement
// and an optional ELSE clause
static int genIFAST(struct ASTnode *n) {
    int Lfalse, Lend;

    // Generate two labels: one for the fasle
    // compound statement, and one fro the end of the
    // overall IF statement.
    // When there is no ELSE clause, Lfase is the ending label!
    Lfalse = genlabel();
    if (n->right) {
        Lend = genlabel();
    }

    // Generate the condition code followed by a zero jump
    // to the false label. We cheat by sending the Lfalse
    // label as a register.
    genAST(n->left, Lfalse, n->op);
    genfreeregs();

    genAST(n->mid, NOREG, n->op);
    genfreeregs();

    // If there is an optional ELSE caluse,
    // generate the jump to skip to the end
    if (n->right) {
        cgjump(Lend);
    }

    // Now the false label
    cglabel(Lfalse);

    // Optional ELSE clause:generate the false
    // compound statement and the end label
    if (n->right) {
        genAST(n->right, NOREG, n->op);
        genfreeregs();
        cglabel(Lend);
    }
    return (NOREG);
}


// Lstart: evaluate condition
//         jump to Lend if condition false
//         statements
//         jump to Lstart
// Lend:
//
// Generate the code for an WHILE statement
static int genWHILEAST(struct ASTnode *n) {
    int Lstart, Lend;

    // Generate two labels: start and end labels
    Lstart = genlabel();
    Lend = genlabel();

    // Now the start label
    cglabel(Lstart);

    // Generate the condition code followed by a zero jump
    // to the end label.
    genAST(n->left, Lend, n->op);
    genfreeregs();

    genAST(n->right, NOREG, n->op);
    genfreeregs();

    // Finally output the jump back to the condition,
    // and the end label
    cgjump(Lstart);
    cglabel(Lend);

    return (NOREG);
}
// Given an AST, the register (if any) that holds
// the previous rvalue, and the AST op of the parent.
// generate assembly code recursively.
// Return the register id with the tree's final value
int genAST(struct ASTnode *n, int label, int parentASTop){
    int leftreg, rightreg;

    switch (n->op) {
        case A_IF: return (genIFAST(n));
        case A_WHILE: return (genWHILEAST(n));
        case A_FUNCTION:
            cgfuncpreamble(Gsym[n->v.id].name);
            genAST(n->left, NOREG, n->op);
            cgfuncpostamble(n->v.id);
            return (NOREG);
        case A_GLUE:
            // Do each child statement, and free the
            // registers after each child.
            genAST(n->left, NOREG, n->op);
            genfreeregs();
            genAST(n->right, NOREG, n->op);
            genfreeregs();
            return (NOREG);
    }

    //Get the left and right sub-tree values
    if (n->left) leftreg = genAST(n->left, NOREG, n->op);
    if (n->right) rightreg = genAST(n->right, leftreg, n->op);

    switch (n->op) {
        case A_ADD: return (cgadd(leftreg, rightreg));
        case A_SUBTRACT: return (cgsub(leftreg, rightreg));
        case A_MULTIPLY: return (cgmul(leftreg, rightreg));
        case A_DIVIDE: return (cgdiv(leftreg, rightreg));
        case A_EQ:
        case A_NE:
        case A_LT:
        case A_GT:
        case A_LE:
        case A_GE:
            if (parentASTop == A_IF || parentASTop == A_WHILE) {
                return (cgcompare_and_jump(n->op, leftreg, rightreg, label));
            } else {
                return (cgcompare_and_set(n->op, leftreg, rightreg));
            }
        case A_AND: return (cgand(leftreg, rightreg));
        case A_OR: return (cgor(leftreg, rightreg));
        case A_XOR: return (cgxor(leftreg, rightreg));
        case A_LSHIFT: return (cgshl(leftreg, rightreg));
        case A_RSHIFT: return (cgshr(leftreg, rightreg));
        case A_POSTINC:
            // Load the variable's value into a registre,
            // then increment it
            return (cgloadglob(n->v.id, n->op));
        case A_POSTDEC:
            // Load the variable's value into a registre,
            // then increment it
            return (cgloadglob(n->v.id, n->op));
        case A_PREINC:
            // Load and increment the variable's value into a registre,
            return (cgloadglob(n->left->v.id, n->op));
        case A_PREDEC:
            // Load and increment the variable's value into a registre,
            return (cgloadglob(n->left->v.id, n->op));
        case A_NEGATE:
            return (cgnegate(leftreg));
        case A_INVERT:
            return (cginvert(leftreg));
        case A_LOGNOT:
            return (cglognot(leftreg));
        case A_TOBOOL:
            // If the parent AST node is an A_IF or A_WHILE, generate
            // a compare followed by a jum. Otherwise, set the registre
            // to 0 or 1 based on it's zeroeness or non-zeroeness
            return (cgboolean(leftreg, parentASTop, label));

        case A_INTLIT: return (cgloadint(n->v.intvalue));
        case A_STRLIT:
            return (cgloadglobstr(n->v.id));
        case A_IDENT:
          // Load our value if we are an rvalue
          // or we are being dereferenced
          if (n->rvalue || parentASTop == A_DEREF)
            return (cgloadglob(n->v.id, label));
          else
            return (NOREG);
        case A_DEREF:
          // If we are an rvalue, dereference to get the value we point at
          // otherwise leave it for A_SSIGN to store through the pointer
          if (n->rvalue)
            return (cgderef(leftreg, n->left->type));
          else
            return (leftreg);
        case A_ASSIGN:
          // Are we assigning to an identifier or through a pointer?
          switch (n->right->op) {
          case A_IDENT: return (cgstorglob(leftreg, n->right->v.id));
          case A_DEREF: return (cgstorderef(leftreg, rightreg, n->right->type));
          default:  fatald("Can't A_SSIGN in genAST, op", n->op);
          }
          break;
        case A_RETURN:
            cgreturn(leftreg, Functionid);
            return (NOREG);
        case A_FUNCCALL:
            return (cgcall(leftreg, n->v.id));
        case A_ADDR:
            return (cgaddress(n->v.id));
        case A_WIDEN:
            // Widen the child's type to the parent's type
            return (cgwiden(leftreg, n->left->type, n->type));
        case A_SCALE:
            // Small optimisation: user shift if the
            // scale value is a known power of two
            switch (n->v.size) {
                case 2: return (cgshlconst(leftreg, 1));
                case 4: return (cgshlconst(leftreg, 2));
                case 8: return (cgshlconst(leftreg, 3));
                default:
                    // Load a register with the size and
                    // multiply the leftreg by this size
                    rightreg = cgloadint(n->v.size);
                    return (cgmul(leftreg, rightreg));
            }
            break;

        default:
            fatals("Unknown AST operator, oper:", asttypestr(n->op));
    }
    return (NOREG);
}

void generatecode(struct ASTnode *n) {
    int reg;

    cgpreamble();
    reg = genAST(n, NOREG, -1);
    cgprintint(reg); // Print the register with the result as an int
    cgpostamble();
}

int genglobstr(char *strvalue) {
    int l = genlabel();
    cgglobstr(l, strvalue);
    return (l);
}

void genpreamble() { cgpreamble(); }
void genpostamble() { cgpostamble(); }
void genfreeregs() { freeall_registers(); }
void genprintint(int reg) { cgprintint(reg); }
void genglobsym(int id) { cgglobsym(id); }
int genprimsize(int type) { return cgprimsize(type); }
