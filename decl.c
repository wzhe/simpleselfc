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

// param_declaration: <null>
//                  | varible_declaration
//                  | varible_declaration ',' varible_declaration
// Parse the parameters in parenthese after the function name.
// Add them as a symbols to the symbol table and return the number of parameters
static int param_declaration(void) {
  int type;
  int paramcnt = 0;

  // Loop until the final right parenthese
  while (Token.token != T_RPAREN) {
    // Get the type and identifier
    // and add it to the symbol table
    type = parse_type();
    ident();
    var_declaration(type, 1, 1);
    paramcnt++;

    // Must have a ',' or ')' at this point
    switch (Token.token) {
    case T_COMMA: scan(&Token); break;
    case T_RPAREN:break;
    default:
      fatals("Unexpected token in parameter list", tokenstr(Token.token));
    }
  }
  return paramcnt;
}

void var_declaration(int type, int islocal, int isparam){

  if (Token.token == T_LBRACKET) {
    // Skip the '['
    scan(&Token);

    if (Token.token == T_INTLIT) {
      if (islocal) {
	addlocl(Text, pointer_to(type), S_ARRAY, isparam, Token.intvalue);
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
      addlocl(Text, type, S_VARIABLE, isparam, 1);
    } else {
      addglob(Text, type, S_VARIABLE, 0, 1);
    }
  }
}

// Parse trhe declaration of a simplistic function
struct ASTnode* function_declaration(int type){
  struct ASTnode *tree, *finalstmt;
  int nameslot;
  int endlabel;
  int paramcnt;

  // Get a label-id for the end label
  endlabel = genlabel();
  // Add the function to the symbol table
  nameslot = addglob(Text, type, S_FUNCTION, endlabel, 0);
  Functionid = nameslot;

  lparen();
  paramcnt = param_declaration();
  Symtable[nameslot].nelems = paramcnt;
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
      var_declaration(type, 0, 0);
      semi();
    }

    // Stop when we have reached EOF
    if (Token.token == T_EOF)
      break;
  }
}
