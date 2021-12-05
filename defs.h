#ifndef DEFS_H_
#define DEFS_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define ASCMD "as -o"
#define LDCMD "cc -o"
#define MAXOBJ 100

// define struct and enum

//#define Debug 0        // add flag in command line
#define TEXTLEN 512      // Max token len
#define NSYMBOLS 1024    // Max symbols num
#define NOREG -1         // Use NOREG when the AST generation functions have no register to return
#define NOLABEL 0        // Use NOLABEL when we have no label to pass to genAST()

//Token
enum {
      T_EOF,
      // precedence from low -> high
      T_ASSIGN,    // =
      T_LOGOR,     // ||
      T_LOGAND,    // &&
      T_OR,        // |
      T_XOR,       // ^
      T_AMPER,     // &
      T_EQ, T_NE,  // ==, !=
      T_LT, T_GT, T_LE, T_GE,
      T_LSHIFT, T_RSHIFT, // << , >>
      T_PLUS, T_MINUS,
      T_STAR, T_SLASH,

      T_INC, T_DEC, // ++,  --
      T_INVERT,     // ~
      T_LOGNOT,     // !
      // no precedence
      // Type keywords
      T_VOID, T_CHAR, T_INT, T_LONG,
      // Structural tokens
      T_IDENT,
      T_INTLIT,
      T_STRLIT,
      T_SEMI,
      T_COMMA,
      T_LBRACE, T_RBRACE,
      T_LPAREN, T_RPAREN,
      T_LBRACKET, T_RBRACKET,
      // Other keywords
      T_IF, T_ELSE,
      T_WHILE,
      T_FOR,
      T_RETURN,
      T_SIZE,  // Just count the token size;
};

// AST node types
enum {
      // must 1:1 map Token
      A_ASSIGN = 1,    // =
      A_LOGOR,     // ||
      A_LOGAND,    // &&
      A_OR,        // |
      A_XOR,       // ^
      A_AND,       // &
      A_EQ, A_NE,  // ==, !=
      A_LT, A_GT, A_LE, A_GE,
      A_LSHIFT, A_RSHIFT, // << , >>
      A_ADD, A_SUBTRACT,
      A_MULTIPLY, A_DIVIDE,

      A_INVERT,     // ~
      A_LOGNOT,     // !
      // end map
      A_ADDR,       // &
      A_DEREF,      // *
      A_NEGATE,     // -
      A_PREINC, A_PREDEC,   // ++A, --A
      A_POSTINC, A_POSTDEC, // A++, A--
      A_TOBOOL,
      // type
      A_VOID, A_CHAR, A_INT, A_LONG,
      // id
      A_IDENT,
      A_INTLIT,
      A_STRLIT,
      // key word
      A_IF, A_ELSE,
      A_WHILE,
      A_FOR,
      A_FUNCTION,
      A_RETURN,
      A_FUNCCALL,
      A_WIDEN,
      A_SCALE,
      A_GLUE,
      A_SIZE,  // Just count the eumn size;
};

// Primitive types
// The bottom 4 bits is an integer value that represents the level of indirection,
// e.g. 0 = no pointer, 1 = pointer, 2 = pointer pointer etc.
enum {
      P_NONE, P_VOID = 16, P_CHAR = 32, P_INT = 48, P_LONG = 64,
};

// Structural types
enum {
      S_VARIABLE, S_FUNCTION, S_ARRAY,
};

// Storage classes
enum {
      C_GLOBAL = 1,
      C_LOCAL,
      C_PARAM,        // Locall visible function parameter
};
//Token structure
struct token{
  int token;
  int intvalue;
};

// Abstract Syntax Tree structure
struct ASTnode{
  int op;
  int type;
  int rvalue;
  struct ASTnode *left;
  struct ASTnode *mid;
  struct ASTnode *right;
  union {
    int intvalue;     // For A_INTLIT, the integer value
    int id;           // For A_IDNT, the symbol slot number
    int size;         // For A_SCALE, the size to scale by
  };
};

// Symbol table structure
struct symtables {
  char *name;
  int type;
  int stype;
  int clas;
  union{
    int endlabel;
    int size;              // array element size
  };
  union{
    int posn;              //  from the stack base pointer
    int nelems;            // For functions, #of params. For structs, # of field
  };
};

#endif // DEFS_H_
