#include "defs.h"
#include "data.h"
#include "decl.h"


// Operator precedence for each token
static int OpPrec[] = {
0, // T_EOF,
10, //T_ASSIGN,    // =
20, //T_LOGOR,     // ||
30, //T_LOGAND,    // &&
40, //T_OR,        // |
50, //T_XOR,       // ^
60, //T_AMPER,     // &
70, 70, //T_EQ, T_NE,  // ==, !=
80, 80, 80, 80, //T_LT, T_GT, T_LE, T_GE,
90, 90, //T_LSHIFT, T_RSHIFT, // << , >>
100, 100, //T_PLUS, T_MINUS,
110, 110, //T_STAR, T_SLASH,
// don't need unary operator
};

// Check that we have a binary operator and
// return its precedence
static int op_precedence(int tokentype) {
    int prec; 
    if (tokentype >= T_VOID) {
      fatald("Token with no precedence in op_precedence:", tokentype);
    }
    prec = OpPrec[tokentype];
    return (prec);
}

static int rightassoc(int tokentype) {
  if (tokentype == T_ASSIGN) {
    return (1);
  }
  return (0);
}

// Convert a token into an AST operation.
// We rely on a 1:1 mapping from token to AST operation
static int binthop(int tok) {
    if (tok > T_EOF && tok < T_INTLIT) {
        return tok;
    }
    fatals("Syntax error, token", tokenstr(tok));
    return tok;
}

// Parse a function call with a single expression
// argument and return its AST
struct ASTnode* funccall(void) {
    struct ASTnode* tree;
    int id;

    // Check that the identifier has been defined,
    // then make a leaf node for it.
    // XXX Add structural type test, should confirm the identifier is really an S_FUNCTION
    if ((id = findsym(Text)) == -1) {
        fatals("Undeclared function", Text);
    }

    // Get the '('
    lparen();

    // Parse the following expression
    tree = binexpr(0);

    // Build the function call AST node.
    // Store the function's return type as this node's type.
    // Also record the function's symbol-id
    tree = mkastunary(A_FUNCCALL, tree, id, Symtable[id].type);

    // Get the ')'
    rparen();

    return tree;
}

// Parse the index into an array and
// return an AST tree for it
static struct ASTnode* array_access(void) {
  struct ASTnode *left, *right;
    int id;

    // Check that the identifier has been defined,
    // then make a leaf node for it.
    if ((id = findglob(Text)) == -1) {
        fatals("Undeclared array", Text);
    }
    left = mkastleaf(A_ADDR, id, Symtable[id].type);

    // Skip the '['
    scan(&Token);

    right = binexpr(0);

    // Get the ']'
    match(T_RBRACKET, "]");

    // Ensure that this is of int type
    if (!inttype(right->type))
      fatal("Array index is not of integer type");

    // Return an AST tree where the array's base hase the offset
    // added to it, and dereference the element. Still an lvalue
    // at this point.
    left = mkastnode(A_ADD, left, NULL, right, 0, Symtable[id].type);
    left = mkastunary(A_DEREF, left, 0, value_at(left->type));

    return (left);
}

// Parse a postfix expression and return an AST node resresenting it.
// The identifier is already in Text.
static struct ASTnode* postfix(void) {
    struct ASTnode* n;
    int id;
    // This could be a variable or a function call.
    // Scan in the next token to find out;
    scan(&Token);

    // It's a '(', so a function call
    if (Token.token == T_LPAREN)
        return (funccall());

    // It's a '[', so an array reference
    if (Token.token == T_LBRACKET)
        return (array_access());

    // A variable. Check that the variable exists.
    id = findsym(Text);
    if (id == -1 || Symtable[id].stype != S_VARIABLE)
        fatals("Unknown variable", Text);

    switch (Token.token) {
        case T_INC:
            scan(&Token);
            n = mkastleaf(A_POSTINC, id, Symtable[id].type);
            break;
        case T_DEC:
            scan(&Token);
            n = mkastleaf(A_POSTDEC, id, Symtable[id].type);
            break;
        default:
            n = mkastleaf(A_IDENT, id, Symtable[id].type);
    }

    return (n);
}
// Parse a primary factor and return an AST node representing it.
static struct ASTnode* primary(void) {
    struct ASTnode *n = NULL;
    int id;

    switch (Token.token) {
        case T_INTLIT:
            if ((Token.intvalue >= 0) && (Token.intvalue < 256))
                n = mkastleaf(A_INTLIT, Token.intvalue, P_CHAR);
            else
                n = mkastleaf(A_INTLIT, Token.intvalue, P_INT);
            break;
        case T_STRLIT:
            // For a STRLIT token, generate the assembly for it.
            // Then make a leaf AST node for it. id is the string's label
            id = genglobstr(Text);
            n = mkastleaf(A_STRLIT, id, P_CHARPTR);
            break;
        case T_IDENT:
            return (postfix());
       case T_LPAREN:
         // skip the '('
            scan(&Token);
            n = binexpr(0);

            rparen();
            return (n);
        default:
            fatals("Syntax error, token", tokenstr(Token.token));
    }
    scan(&Token);
    return (n);
}

// Parse a primary factor and return an AST node representing it.
static struct ASTnode* prefix(void) {
    struct ASTnode *tree = NULL;

    switch (Token.token) {
        case T_AMPER:
            scan(&Token);
            tree = prefix();
            if (tree->op != A_IDENT) {
                fatal("& operator must be followed by an identifier");
            }
            // Now change the type to A_ADDR and
            // change the type to pointer
            tree->op = A_ADDR;
            tree->type = pointer_to(tree->type);
            break;
        case T_STAR:
            scan(&Token);
            tree = prefix();
            if (tree->op != A_IDENT && tree->op != A_DEREF) {
                fatals("* operator must be followed by an identifier or *, asttype", asttypestr(tree->op));
            }

            // Prepend an A_DEREF operation to the tree
            tree = mkastunary(A_DEREF, tree, 0, value_at(tree->type));
            break;
        case T_MINUS:
            scan(&Token);
            tree = prefix();
            tree->rvalue = 1;
            // check
            tree = mkastunary(A_NEGATE, tree, 0, P_INT);
            break;
        case T_INVERT:
            scan(&Token);
            tree = prefix();
            // check
            tree->rvalue = 1;
            tree = mkastunary(A_INVERT, tree, 0, tree->type);
            break;
        case T_LOGNOT:
            scan(&Token);
            tree = prefix();
            tree->rvalue = 1;
            tree = mkastunary(A_LOGNOT, tree, 0, tree->type);
            break;
        case T_INC:
            scan(&Token);
            tree = prefix();
            if (tree->op != A_IDENT ) {
                fatals("++ operator must be followed by an identifier ,asttype", asttypestr(tree->op));
            }
            tree = mkastunary(A_PREINC, tree, 0, tree->type);
            break;
        case T_DEC:
            scan(&Token);
            tree = prefix();
            // check
            if (tree->op != A_IDENT ) {
                fatals("-- operator must be followed by an identifier ,asttype", asttypestr(tree->op));
            }
            tree = mkastunary(A_PREDEC, tree, 0, tree->type);
            break;
        default:
            tree = primary();
    }
    return (tree);
}


// Return an AST tree whose root is a binary operator
// Parameter ptp is the prevision token's precedence.
struct ASTnode* binexpr(int ptp) {
    struct ASTnode *left, *right;
    struct ASTnode *ltemp, *rtemp;
    int tokentype;
    int ASTop;

    // Get the inteer literal on the left.
    // Fetch the next token at the same time.
    left = prefix();

    // If no tokens left, return into a node type
    tokentype = Token.token;
    if (tokentype == T_SEMI || tokentype == T_RPAREN || tokentype == T_RBRACKET) {
      left->rvalue = 1;
        return (left);
    }

    // While the precedence of this token is more than
    // that of the previous token precedence,
    // or it's right associative and equal to the
    // previous token's precedence.
    while (op_precedence(tokentype) > ptp ||
           (rightassoc(tokentype) && op_precedence(tokentype) == ptp)) {
        // Fetch in the next integer literal
        scan(&Token);

        // Recursively call binexpr() with the precedence
        // of our token to build a sub-tree
        right = binexpr(OpPrec[tokentype]);

        // Ensure the two typs are compatible
        ASTop = binthop(tokentype);

        if (ASTop == A_ASSIGN) {
          // Assignment
          // Make the right tree into an rvalue
          right->rvalue = 1;

          // Ensure the right's type matches the left
          right = modify_type(right, left->type, 0);
          if (right == NULL || left == NULL)
            fatal("Incompatible expression in assignment");

          // Make an assignment AST tree. However, switch left
          // and right around, so that the right expression's code
          // will be generated before the left expression.
          ltemp = left;
          left = right;
          right = ltemp;
        } else {
          // not in assignment, both is rvalue.
          left->rvalue = 1;
          right->rvalue = 1;

          ltemp = modify_type(left, right->type, ASTop);
          rtemp = modify_type(right, left->type, ASTop);

          if (ltemp == NULL && rtemp == NULL)
              fatal("Incompatible expression in expr");

          if (ltemp != NULL) left = ltemp;
          if (rtemp != NULL) right = rtemp;
        }

        // Join that sub-tree with ours. Convert the token
        // into an AST operation at the same time.
        left = mkastnode(ASTop, left, NULL, right, 0, left->type);

        // Update the details of the current token.
        // If no tokens left, return just the left node
        tokentype = Token.token;
        if (tokentype == T_SEMI || tokentype == T_RPAREN)
            return (left);
    }

    // Return the tree we have when the precedence is the same or lower
    return (left);
}
