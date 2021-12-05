#ifndef DECL_H_
#define DECL_H_

// Function prototypes for all compiler files
//
// scan.c
int scan(struct token *t);
void reject_token(struct token *t);

// tree.c
struct ASTnode* mkastnode(int op, struct ASTnode *left, struct ASTnode* mid,
                          struct ASTnode *right, int intvale, int type);

struct ASTnode* mkastleaf(int op, int intvale, int type);

struct ASTnode* mkastunary(int op, struct ASTnode *left, int intvale, int type);

void show(struct ASTnode* tree);

// expr.c
struct ASTnode* binexpr(int ptp);
struct ASTnode* funccall(void);

// gen.c
int genAST(struct ASTnode *n, int label, int parentASTop);
void genpreamble();
void genpostamble();
void genfreeregs();
void genprintint(int reg);
void genglobsym(int id);
int genprimsize(int type);
int genlabel(void);
int genglobstr(char *strvalue);

// stmt.c
struct ASTnode* single_statement(void);
struct ASTnode* compound_statement(void);

// misc.c
void match(int t, char *what);
void semi(void);
void lbrace(void);
void rbrace(void);
void lparen(void);
void rparen(void);
void ident(void);

void fatal(char *s);
void fatals(char *s1, char *s2);
void fatald(char *s, int d);
void fatalc(char *s, int c);

char* tokenstr(int tok);
char* asttypestr(int asttype);
char* typestr(int type);

// sym.c
int findglob(char *s);
int findlocl(char *s);
int addglob(char *name, int type, int stype, int clas, int size);
int addlocl(char *name, int type, int stype, int clas, int size);
int findsym(char *s);
void copyfuncparams(int slot);
void freelocalsym();
void clear_symtable();

// decl.c
int parse_type(void);
void var_declaration(int type, int clas);
struct ASTnode* function_declaration(int type);
void global_declarations(void);

// types.c

// Given an AST tree and a type which we want it to become,
// possibly modify the tree by widening or scaling so that
// it is compatible with this type.
// Return the original tree if no changes occurred,
// a modified tree, or NULL if the tree is not compatible
// with the given type.
// If this will be part of a binary operation, the AST op is not zero.
struct ASTnode* modify_type(struct ASTnode *tree, int rtype, int op);
//int type_compatible(int *left, int  *right, int onlyright);
int pointer_to(int type);
int value_at(int type);
int inttype(int type);
int ptrtype(int type);

#endif // DECL_H_
