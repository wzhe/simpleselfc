#include "defs.h"
#include "data.h"
#include "decl.h"

// BNF
//
// global_declarations: global_declarations
//                    | global_declaration global_declarations
//                    ;
//
// global_declaration : function_declaration
//                    | var_declaration
//                    ;
//
// var_declaration      : type identifier_list ';' ;
//
// function_declaration : type identifier '(' expression ')' compound_statement ;
//
// type               : type_keyword opt_pointer ;
//
// type_keyword       : 'void' | 'char' | 'int' | 'long' ;
//
// opt_pointer        : <empty> | '*' opt_pointer;
//
// identifier_list    : identifier | identifier ',' identifier_list;
//
// compound_statement:  '{' '}'
//                   |  '{' statement '}'
//                   |  '{' statement statement '}'
//                   ;
//
// statement         :  print_statment
//                   |  var_declaration
//                   |  assignment_statemnt
//                   |  if_statement
//                   |  while_statement
//                   |  for_statement
//                   |  function_declaration
//                   |  function_call
//                   |  return_statement
//                   ;
//
//
// assignment_statement :  identifier '=' expression ';' ;
//
// if_statemenet     : if_head
//                   | if_head 'else' compound_statement
//                   ;
//
// if_head           : 'if' '(' true_false_expression ')' compound_statement ;
//
// while_statemenet  : 'while' '(' true_false_expression ')' compound_statement ;
//
// for_statemenet    : 'for' '('preop_statement ';' true_false_expression  ';' postop_statement ')' compound_statement ;
//
// preop_statement   : statement ;  (for now)
//
// postop_statement  : statement ;  (for now)
//
// function_call     : identifier '(' expression ')' ;
//
// return_statement  : 'return'  '(' expression ')' ;
//
// identifier        : T_IDENT ;
//


// Parse an IF statement including any optional ELSE clause
// and return its AST
static struct ASTnode* if_statement(void) {
    struct ASTnode *condAST, *trueAST, *falseAST = NULL;

    // Ensure we have 'if' '('
    match(T_IF, "if");
    lparen();

    // Parse the following expression
    // and the ')' following.
    // Ensure the tree's operation is a comparison.
    condAST = binexpr(0);

    if (condAST->op < A_EQ || condAST->op > A_GE) {
        condAST = mkastunary(A_TOBOOL, condAST, NULL, 0, condAST->type);
    }

    rparen();

    // Get the AST for the compound statement
    trueAST = compound_statement();

    // If we have an 'else', skip it and get the AST
    // for the compound statement
    if (Token.token == T_ELSE) {
        scan(&Token);
        falseAST = compound_statement();
    }
    return (mkastnode(A_IF, condAST, trueAST, falseAST, NULL, 0, P_NONE));
}

// Parse an WHILE statement
// and return its AST
static struct ASTnode* while_statement(void) {
    struct ASTnode *condAST, *bodyAST;

    // Ensure we have 'while' '('
    match(T_WHILE, "while");
    lparen();

    // Parse the following expression
    // and the ')' following.
    // Ensure the tree's operation is a comparison.
    condAST = binexpr(0);

    if (condAST->op < A_EQ || condAST->op > A_GE) {
        condAST = mkastunary(A_TOBOOL, condAST, NULL, 0, condAST->type);
    }

    rparen();

    Looplevel++;
    // Get the AST for the compound statement
    bodyAST = compound_statement();
    Looplevel--;

    return (mkastnode(A_WHILE, condAST, NULL, bodyAST, NULL, 0, P_NONE));
}

// Parse an FOR statement
// and return its AST
// augmented WHILE loop:
//     preop_statement;
//     while (true_false_expression) {
//        compound_statement;
//        postop_statement;
//     }
static struct ASTnode* for_statement(void) {
    struct ASTnode *condAST, *bodyAST;
    struct ASTnode *preopAST, *postopAST;
    struct ASTnode *tree;

    // Ensure we have 'for' '('
    match(T_FOR, "for");
    lparen();

    // Get the condition and the ';'
    preopAST = single_statement();
    semi();

    // Parse the following expression
    // and the ')' following.
    // Ensure the tree's operation is a comparison.
    condAST = binexpr(0);

    if (condAST->op < A_EQ || condAST->op > A_GE) {
        condAST = mkastunary(A_TOBOOL, condAST, NULL, 0, condAST->type);
    }
    semi();

    postopAST = single_statement();
    rparen();

    // Get the AST for the compound statement
    Looplevel++;
    bodyAST = compound_statement();
    Looplevel--;

    // For now, all four sub-trees have to be non-NULL.
    // Later on, we'll change the semantics for when some are mising.

    // Glue the compound statement and the postop tree
    tree = mkastnode(A_GLUE, bodyAST, NULL, postopAST, NULL, 0, P_NONE);

    // Make a WHILE loop with the condition and this new body
    tree = mkastnode(A_WHILE, condAST, NULL, tree, NULL, 0, P_NONE);

    // And glue the preop tree to the A_WHILE tree
    return (mkastnode(A_GLUE, preopAST, NULL, tree, NULL, 0, P_NONE));
}

static struct ASTnode* return_statement(){
    struct ASTnode *tree;

    // Can't return a value if function returns P_VOID
    if (Functionid->type == P_VOID)
        fatals("Can't return from a void function", Functionid->name);

    // Match a 'print' as the first token.
    match(T_RETURN, "return");

    lparen();

    // Parse the following expression
    // and generate the assembly code
    tree = binexpr(0);

    // Ensure this is compatible with the function's type
    tree = modify_type(tree, Functionid->type, 0);
    if (tree == NULL) fatal("Incompatible type to return");

    tree = mkastunary(A_RETURN, tree, NULL, 0, P_VOID);

    rparen();
    return (tree);
}

static struct ASTnode* break_statement() {
    if (Looplevel == 0)
        fatal("no loop to break out from");
    scan(&Token);
    return (mkastleaf(A_BREAK, NULL, 0, 0));
}

static struct ASTnode* continue_statement() {
    if (Looplevel == 0)
        fatal("no loop to continue out from");
    scan(&Token);
    return (mkastleaf(A_CONTINUE, NULL, 0, 0));
}

static struct ASTnode* switch_statement() {
    struct ASTnode *left, *n, *c, *casetree = NULL, *casetail;
    int inloop = 1, casecount = 0;
    int seendefault = 0;
    int ASTop, casevalue;

    // Skip the 'switch' and '('
    scan(&Token);
    lparen();

    // Get the switch expression, the ')' and the '{'
    left = binexpr(0);
    rparen();
    lbrace();

    // Ensure that this is of int type
    if (!inttype(left->type))
        fatal("Switch expression is not of integer type");

    // Build an A_SWITCH subtree with the expression as the child
    n = mkastunary(A_SWITCH, left, NULL, 0, 0);

    // Now parse the cases
    Switchlevel++;
    while (inloop) {
        switch (Token.token) {
            // Leave the oop when we hit a '}'
            case T_RBRACE: {
                if (casecount == 0)
                    fatal("No cases in switch");
                inloop = 0;
                break;
            }
            case T_CASE:
            case T_DEFAULT:
                if (seendefault)
                    fatal("case or default after existing default");
                if (Token.token == T_DEFAULT) {
                    ASTop = A_DEFAULT;
                    seendefault = 1;
                    scan(&Token);
                } else {
                    ASTop = A_CASE;
                    scan(&Token);
                    left = binexpr(0);
                    // Ensure the case value is an integer literal
                    if (left->op != A_INTLIT)
                        fatal("Expecting integer literal for case value");
                    casevalue = left->intvalue;

                    // Walk the list of existing case value to ensure
                    // that there isn't a duplicate case value
                    for (c = casetree; c != NULL; c = c->right)
                        if (casevalue == c->intvalue)
                            fatal("Duplicate case value");
                }
                // Scant the ':' and get the compound expression
                match(T_COLON, ":");
                left = compound_statement();
                casecount++;

                // Build a sub-tree with the compound statement as left child
                // and link it in to the growing A_CASE tree
                if (casetree == NULL) {
                    casetree = casetail = mkastunary(ASTop,  left, NULL, casevalue, 0);
                } else {
                    casetail->right = mkastunary(ASTop, left, NULL, casevalue, 0);
                    casetail = casetail->right;
                }
                break;
            default:
                fatald("Unexpected token in switch", Token.token);
                break;
        }
    }

    Switchlevel--;
    n->intvalue = casecount;
    n->right = casetree;
    rbrace();
    return (n);
}

// Parse a single statement
// and return its AST
struct ASTnode* single_statement(void) {
    int type;
    int clas = C_LOCAL;
    struct symtable *ctype;
    switch(Token.token) {
        case T_IDENT:
            if (findtypedef(Text) == NULL)
                return (binexpr(0));
        case T_CHAR:
        case T_INT:
        case T_LONG:
        case T_STRUCT:
        case T_UNION:
        case T_ENUM:
        case T_TYPEDEF:
            type = parse_type(&ctype, &clas);
            ident();
            var_declaration(type,ctype, clas);
            semi();
            return (NULL);            // No AST generated here
            // case T_IDENT:
            //return assignment_statement();
        case T_IF:
            return (if_statement());
        case T_WHILE:
            return (while_statement());
        case T_FOR:
            return (for_statement());
        case T_BREAK:
            return (break_statement());
        case T_CONTINUE:
            return (continue_statement());
        case T_RETURN:
            return (return_statement());
        case T_SWITCH:
            return (switch_statement());
        default:
            return (binexpr(0));
            // fatals("Syntax error, token", tokenstr(Token.token));
    }
}
// Parse one or more statements
// and return its AST
struct ASTnode* compound_statement(void) {
    struct ASTnode  *left = NULL;
    struct ASTnode  *tree = NULL;

    // Require a left curly bracket
    lbrace();

    while(1) {
        tree = single_statement();

        // Some satements must be followed by a semicolon
        if (tree != NULL &&
            (tree->op == A_ASSIGN
             || tree->op == A_FUNCCALL
             || tree->op == A_RETURN
             || tree->op == A_BREAK
             || tree->op == A_CONTINUE
            )) {
            semi();
        }

        // For each new tree, either save it in left
        // if left is empty, or glue the left and the new tree together
        if (tree) {
            if (left == NULL) {
                left = tree;
            } else {
                left = mkastnode(A_GLUE, left, NULL, tree, NULL, 0, P_NONE);
            }
        }

        // When we hit a right curly bracket,
        // skip past it and return the AST
        if (Token.token == T_RBRACE) {
            rbrace();
            return (left);
        }
    }
}
