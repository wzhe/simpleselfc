#include "defs.h"
#include "data.h"
#include "decl.h"


// param_declaration: <null>
//                  | variable_declaration
//                  | variable_declaration ',' variable_declaration
// Parse the parameters in parenthese after the function name.
// Add them as a symbols to the symbol table and return the number of parameters
static int param_declaration(struct symtable *funcsym) {
  int type;
  struct symtable *ctype = NULL;
  struct symtable *param = NULL;
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
    type = parse_type(&ctype);
    ident();

    // We have an existing prototype.
    // Check that this type matches the prototype.
    if (param != NULL) {
      if (type != param->type)
	fatals("Type doesn't match prototype for parameter", param->name);
      param = param->next;
    } else {
      // Add a new parmeter to the new prototype
      var_declaration(type, ctype, C_PARAM);
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
static int var_declaration_list(struct symtable *funcsym, int clas, int separate_token, int end_token) {
  int type;
  int paramcnt = 0;
  struct symtable *ctype; 
  struct symtable *protoptr = NULL; 

  if (funcsym != NULL) {
    protoptr = funcsym->member;
  }

  while(Token.token != end_token) {
    
    type = parse_type(&ctype);
    ident();
    if (protoptr != NULL) {
      if (type != protoptr->type)
	fatals("Type doesn't match prototype for paramter", protoptr->name);
      protoptr = protoptr->next;
    } else {
      var_declaration(type, ctype, clas);
    }
    paramcnt++;
  
    if ((Token.token != separate_token) && (Token.token != end_token))
      fatals("Unexpected token in parameter list", tokenstr(Token.token)); 
    if (Token.token == separate_token)
      scan(&Token);
  }

  if ((funcsym != NULL) && (paramcnt != funcsym->nelems))
    fatals("Parameter count mismatch for function", funcsym->name);

  return (paramcnt);
}

static struct symtable* struct_declaration(void) {
  struct symtable *ctype = NULL;
  struct symtable *m;
  int offset;

  // Skip the struct keyword
  scan(&Token);

  // See if there is a following struct name
  if (Token.token == T_IDENT) {
    ctype = findstruct(Text);
    scan(&Token);
  }

  // If the next token isn't an LBRACE, this is the usage
  // of an existing struct type. Return the pointer to the type.
  if (Token.token != T_LBRACE) {
    if (ctype == NULL)
      fatals("unknown struct type", Text);
    return (ctype);
  }
  if (ctype)
    fatals("previously defined struct", Text);

  ctype = addstruct(Text, P_STRUCT, NULL, 0, 0);
  scan(&Token);

  // Scan in the list of members and attach to the struct type's node
  var_declaration_list(NULL, C_MEMBER, T_SEMI, T_RBRACE);

  rbrace();

  ctype->member = Memberhead;
  Memberhead = Membertail = NULL;
  // Set the offset of the initial member
  // and find the first free byte after it
  m = ctype->member;
  m->posn = 0;
  offset = typesize(m->type, m->ctype);
  for (m = m->next; m != NULL; m = m->next) {
    // Set the offset for this member
    m->posn = genalign(m->type, offset, 1);

    // Get the offest of the next free byte after this member
    offset += typesize(m->type, m->ctype);
  }

  ctype->size = offset;
  return (ctype);
}
// Parse the current token and
// return a primitive type enum value.
int parse_type(struct symtable **ctype) {
  int type;
  switch (Token.token) {
  case T_VOID:
    type = P_VOID;
    scan(&Token);
    break;
  case T_CHAR:
    type = P_CHAR;
    scan(&Token);
    break;
  case T_INT:
    type = P_INT;
    scan(&Token);
    break;
  case T_LONG:
    type = P_LONG;
    scan(&Token);
    break;
  case T_STRUCT:
    type = P_STRUCT;
    *ctype = struct_declaration();
    break;
  default:
    fatals("Illegal type, token", tokenstr(Token.token));
  }

  // Scan in one or more further '*' tokens
  // and determine the correct pointer type
  while (1) {
    if (Token.token != T_STAR) break;
    type = pointer_to(type);
    scan(&Token);
  }

  // We leave with the next token already scanned
  return (type);
}



struct symtable* var_declaration(int type, struct symtable* ctype, int clas){
  struct symtable *sym = NULL;

  switch (clas) {
  case C_GLOBAL:
    if (findglob(Text) != NULL) 
      fatals("Duplicate local variable declaration", Text);
    break;
  case C_LOCAL:
  case C_PARAM:
    if (findlocl(Text) != NULL) 
      fatals("Duplicate local variable declaration", Text);
    break;
  case C_MEMBER:
    if (findmember(Text) != NULL)
      fatals("Duplicate struct/union member declaration", Text);
    break;
  default:
    fatals("Unkonw Type clas", Text);
  }
  if (Token.token == T_LBRACKET) {
    // Skip the '['
    scan(&Token);

    if (Token.token == T_INTLIT) {
      // Add this as a know array and generate its space in assembly.
      // We treat the array as a pointer to its element's type
      if (clas == C_GLOBAL) {
	addglob(Text, pointer_to(type), ctype, S_ARRAY, Token.intvalue);
      } else {
	fatal("For now, declaration of non-global arrays is not implemented");
      }
      scan(&Token);
      match(T_RBRACKET, "]");
    } else {
      fatal("Missing arrary size.");
    } 
  } else {
    // Add this as a known scalar
    // and generate its space in assembly
    switch (clas) {
    case C_GLOBAL: 
      sym = addglob(Text, type, ctype, S_VARIABLE, 1);
      break;
    case C_LOCAL: 
      sym = addlocl(Text, type, ctype, S_VARIABLE, 1);
      break;
    case C_PARAM: 
      sym = addparm(Text, type, ctype, S_VARIABLE, 1);
      break;
    case C_MEMBER:
      sym = addmember(Text, type, ctype, S_VARIABLE, 1);
      break;
    
    }
  }
  return (sym);
}

// Parse trhe declaration of a simplistic function
struct ASTnode* function_declaration(int type){
  struct ASTnode *tree, *finalstmt;
  struct symtable *oldfuncsym, *newfuncsym = NULL;
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
    newfuncsym = addglob(Text, type, NULL, S_FUNCTION, endlabel);
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
  struct symtable *ctype;
  int type;

  while (1) {
    if (Token.token == T_EOF)
      break;

    type = parse_type(&ctype);

    // We might have just parsed a struct declaration
    // with no associated variable. The next token
    // magit be a ';'. Loop back if it is.
    if (type == P_STRUCT && Token.token == T_SEMI) {
      scan(&Token);
      continue;
    }

    // We have to read past the identifier to
    // see either a '(' for a function declaration
    // or a ',' or ';' for a variable declaration.
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
      var_declaration(type, ctype, C_GLOBAL);
      semi();
    }

    // Stop when we have reached EOF
    if (Token.token == T_EOF)
      break;
  }
}
