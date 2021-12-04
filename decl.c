#include "defs.h"
#include "data.h"
#include "decl.h"

// Parse the current token and
// return a primitive type enum value.
int parse_type(void) {
  int type;
  switch (Token.token) {
  case T_VOID:
    type = P_VOID;
    break;
  case T_CHAR:
    type = P_CHAR;
    break;
  case T_INT:
    type = P_INT;
    break;
  case T_LONG:
    type = P_LONG;
    break;
  default:
    fatald("Illegal type, token", Token.token);
  }

  // Scan in one or more further '*' tokens
  // and determine the correct pointer type
  while (1) {
    scan(&Token);
    if (Token.token != T_STAR) break;
    type = pointer_to(type);
  }

  // We leave with the next token already scanned
  return type;
}

void var_declaration(int type, int islocal){

  if (Token.token == T_LBRACKET) {
    // Skip the '['
    scan(&Token);

    if (Token.token == T_INTLIT) {
      if (islocal) {
	addlocl(Text, pointer_to(type), S_ARRAY, 0, Token.intvalue);
      } else {
	addglob(Text, pointer_to(type), S_ARRAY, 0, Token.intvalue);
      }
      scan(&Token);
      match(T_RBRACKET, "]");
    } else {
      fatal("Missing arrary size.");
    } 
  } else {
    if (islocal) {
      addlocl(Text, type, S_VARIABLE, 0, 1);
    } else {
      addglob(Text, type, S_VARIABLE, 0, 1);
    }
  }
  semi();
}

  // Parse trhe declaration of a simplistic function
  struct ASTnode* function_declaration(int type){
    struct ASTnode *tree, *finalstmt;
    int nameslot;
    int endlabel;

    // Get a label-id for the end label
    endlabel = genlabel();
    // Add the function to the symbol table
    nameslot = addglob(Text, type, S_FUNCTION, endlabel, 0);
    Functionid = nameslot;

    genresetlocals(); // Reset position of new locals

    lparen();
    rparen();

    // Get the AST tree for the compound statement
    tree = compound_statement();

    // If the function type isn't P_VOID, check that the last
    // AST operation in the compound statement was a return statement
    if (type != P_VOID) {
      // Error if no statemets in the function
      if (tree == NULL)
	fatal("No statemets in function with non-void type");

      // Check that the last AST operation in the compound statement
      // wa a return statement.
      finalstmt = (tree->op == A_GLUE) ? tree->right : tree;
      if (finalstmt == NULL || finalstmt->op != A_RETURN)
	fatals("No return for unction with non-void type", Symtable[Functionid].name);
    }

    // Return an A_FUNCTION node which has the function's nameslot
    // and the compound statement sub-tree
    return mkastunary(A_FUNCTION, tree, nameslot, P_VOID);
  }

  // Parse one or more global declarations,
  // ethier variables or functions
  void global_declarations(void) {
    struct ASTnode *tree;
    int type;

    while (1) {
      // We have to read past the type and identifier to see either
      // a '(' for a function declaration
      // or a',' or ';' for a variables declaration.
      // Text is filled in by the ident() call.
      type = parse_type();
      ident();

      if (Token.token == T_LPAREN) {
	tree = function_declaration(type);
	if (Outdump) show(tree);
	genAST(tree, NOREG, 0);
      } else {
	var_declaration(type, 0);
      }

      // Stop when we have reached EOF
      if (Token.token == T_EOF)
	break;
    }
  }
