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
static int param_declaration(struct symtables *funcsym) {
  int type;
  struct symtables *param = NULL;
  int paramcnt = 0;

  /* printf("func:%s, param:cnt:%d\n", funcsym->name, n->nelems); */
  // Add 1 to id so that it's either zero (no prototype), or
  // it's the position of the zeroth existing parameter in the symbol table
  if (funcsym != NULL)
    param = funcsym->member;

  // Loop until the final right parenthese
  while (Token.token != T_RPAREN) {
    // Get the type and identifier
    // and add it to the symbol table
    type = parse_type();
    ident();

    // We have an existing prototype.
    // Check that this type matches the prototype.
    if (param != NULL) {
      if (type != param->type)
	fatals("Type doesn't match prototype for parameter", param->name);
      param = param->next;
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
  if ((param != NULL) && (paramcnt != funcsym->nelems))
    fatals("Parameter count mismatch for function", funcsym->name);
  return (paramcnt);
}

void var_declaration(int type, int clas){

  switch (clas) {
  case C_GLOBAL:
    if (findglob(Text) != NULL) 
      fatals("Duplicate local variable declaration", Text);
    break;
  case C_LOCAL:
  case C_PARAM:
  default:
    if (findlocl(Text) != NULL) 
      fatals("Duplicate local variable declaration", Text);
  }
  if (Token.token == T_LBRACKET) {
    // Skip the '['
    scan(&Token);

    if (Token.token == T_INTLIT) {
      // Add this as a know array and generate its space in assembly.
      // We treat the array as a pointer to its element's type
      if (clas == C_LOCAL || clas == C_PARAM) {
	fatal("For now, declaration of local arrays is not implemented");
      } else {
	addglob(Text, pointer_to(type), S_ARRAY, clas, Token.intvalue);
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
      addlocl(Text, type, S_VARIABLE, clas, 1);
    } else if (clas == C_PARAM) {
      addparm(Text, type, S_VARIABLE, clas, 1);
    } else if (clas == C_GLOBAL) {
      addglob(Text, type, S_VARIABLE, clas, 1);
    }
  }
}

// Parse trhe declaration of a simplistic function
struct ASTnode* function_declaration(int type){
  struct ASTnode *tree, *finalstmt;
  struct symtables *oldfuncsym, *newfuncsym = NULL;
  int endlabel, paramcnt;

  // Text has the identifier's name. If this exist and is a function,
  // get the id. Otherwise, set id to -1
  if ((oldfuncsym = findsym(Text)) != NULL) {
    if (oldfuncsym->stype != S_FUNCTION)
      oldfuncsym = NULL;
  }

  // If this is a new function declaration, 
  // get a label-id for the end label, and add the function
  // to the symbol table
  if (oldfuncsym == NULL) {
    endlabel = genlabel();
    // Add the function to the symbol table
    newfuncsym = addglob(Text, type, S_FUNCTION, C_GLOBAL, endlabel);
  }

  lparen();
  paramcnt = param_declaration(oldfuncsym);
  rparen();

  // If this is a new function declaration, update the function
  // symbol entry with the number of parameters
  if (newfuncsym) {
    newfuncsym->nelems = paramcnt;
    newfuncsym->member = Parmhead;
    oldfuncsym = newfuncsym;
  }

  // clear out the parameter list
  Parmhead = Parmtail = NULL;

  //showsym(newfuncsym);
  if (Token.token == T_SEMI) {
    scan(&Token);
    return (NULL);
  }
  
  // This is not just a prototype.
  // Copy the global parameters to be local parameters
  /* copyfuncparams(funcsym); */

  Functionid = oldfuncsym;
  // Get the AST tree for the compound statement
  tree = compound_statement();

  // If the function type isn't P_VOID, check that the last
  // AST operation in the compound statement was a return statement
  if (type != P_VOID) {
    // Error if no statements in the function
    if (tree == NULL)
      fatal("No statements in function with non-void type");

    // Check that the last AST operation in the compound statement
    // wa a return statement.
    finalstmt = (tree->op == A_GLUE) ? tree->right : tree;
    if (finalstmt == NULL || finalstmt->op != A_RETURN)
      fatals("No return for function with non-void type", oldfuncsym->name);
  }

  // Return an A_FUNCTION node which has the function's nameslot
  // and the compound statement sub-tree
  return mkastunary(A_FUNCTION, tree,  oldfuncsym, 0, P_VOID);
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
      if (tree == NULL) {
	// Only a function prototype
	continue;
      }
      if (O_dumpAST) show(tree);
      genAST(tree, NOREG, 0);
      // Now free the 
      freeast(tree);
      freelocalsym();
    } else {
      var_declaration(type, C_GLOBAL);
      semi();
    }

    // Stop when we have reached EOF
    if (Token.token == T_EOF)
      break;
  }
}
