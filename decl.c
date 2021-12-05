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
static int param_declaration(int id) {
  int type, param_id;
  int orig_paramcnt;
  int paramcnt = 0;

  // Add 1 to id so that it's either zero (no prototype), or
  // it's the position of the zeroth existing parameter in the symbol table
  param_id = id + 1;

  // Get any existing prototype parameter count
  if (param_id)
    orig_paramcnt = Symtable[id].nelems;

  // Loop until the final right parenthese
  while (Token.token != T_RPAREN) {
    // Get the type and identifier
    // and add it to the symbol table
    type = parse_type();
    ident();

    // We have an existing prototype.
    // Check that this type matches the prototype.
    if (param_id) {
      if (type != Symtable[id].type)
	fatald("Type doesn't match prototype for parameter", orig_paramcnt + 1);
      param_id++;
    } else {
      // Add a new parmeter to the new prototype
      var_declaration(type, C_PARAM);
    }

    paramcnt++;

    // Must have a ',' or ')' at this point
    switch (Token.token) {
    case T_COMMA: scan(&Token); break;
    case T_RPAREN:break;
    default:
      fatals("Unexpected token in parameter list", tokenstr(Token.token));
    }
  }
  // Check that the number of parameters in this list matches
  // any existing prototype
  if ((id != -1) && (paramcnt != orig_paramcnt))
    fatals("Parameter count mismatch for function", Symtable[id].name);
  return paramcnt;
}

void var_declaration(int type, int clas){

  if (Token.token == T_LBRACKET) {
    // Skip the '['
    scan(&Token);

    if (Token.token == T_INTLIT) {
      // Add this as a know array and generate its space in assembly.
      // We treat the array as a pointer to its element's type
      if (clas == C_LOCAL) {
	fatal("For now, declaration of local arrays is no implemented");
      } else {
	addglob(Text, pointer_to(type), S_ARRAY, clas, 0, Token.intvalue);
      }
      scan(&Token);
      match(T_RBRACKET, "]");
    } else {
      fatal("Missing arrary size.");
    } 
  } else {
    // Add this as a known scalar
    // and generate its space in assembly
    if (clas == C_LOCAL) {
      if (addlocl(Text, type, S_VARIABLE, clas, 1) == -1)
	fatals("Dumplicate local variable declaration", Text);
    } else {
      addglob(Text, type, S_VARIABLE, clas, 0, 1);
    }
  }
}

// Parse trhe declaration of a simplistic function
struct ASTnode* function_declaration(int type){
  struct ASTnode *tree, *finalstmt;
  int nameslot;
  int endlabel;
  int paramcnt;
  int id;

  // Text has the identifier's name. If this exist and is a function,
  // get the id. Otherwise, set id to -1
  if ((id = findsym(Text)) != -1) {
    if (Symtable[id].stype != S_FUNCTION)
      id = -1;
  }

  // If this is a new function declaration, 
  // get a label-id for the end label, and add the function
  // to the symbol table
  if (id == -1) {
    endlabel = genlabel();
    // Add the function to the symbol table
    nameslot = addglob(Text, type, S_FUNCTION, C_GLOBAL, endlabel, 0);
  }

  lparen();
  paramcnt = param_declaration(id);
  rparen();

  // If this is a new function declaration, update the function
  // symbol entry with the number of parameters
  if (id == -1)
    Symtable[nameslot].nelems = paramcnt;

  if (Token.token == T_SEMI) {
    scan(&Token);
    return (NULL);
  }
  
  // This is not just a prototype.
  // Copy the global parameters to be local parameters
  if (id == -1)
    id = nameslot;

  copyfuncparams(id);

  Functionid = nameslot;
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
      var_declaration(type, C_GLOBAL);
      semi();
    }

    // Stop when we have reached EOF
    if (Token.token == T_EOF)
      break;
  }
}
