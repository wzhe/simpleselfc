#include "defs.h"
#include "data.h"
#include "decl.h"
#include <unistd.h>

void match(int t, char *what) {
    if (Token.token == t) {
        scan(&Token);
    } else {
        printf("%s expected on line %d\n", what, Line);
        exit(1);
    }
}

void semi(void) {
    match(T_SEMI, ";");
}
void ident(void) {
    match(T_IDENT, "identifier");
}

void lbrace(void) {
    match(T_LBRACE, "{");
}
void rbrace(void) {
    match(T_RBRACE, "}");
}
void lparen(void) {
    match(T_LPAREN, "(");
}
void rparen(void) {
    match(T_RPAREN, ")");
}

void fatal(char *s){
    fprintf(stderr, "%s on line %d\n", s, Line);
    fclose(Outfile);
    unlink(Outfilename);
    exit(1);
}
void fatals(char *s1, char *s2){
    fprintf(stderr, "%s:%s on line %d\n", s1, s2, Line);
    fclose(Outfile);
    unlink(Outfilename);
    exit(1);
}
void fatald(char *s, int d) {
    fprintf(stderr, "%s:%d on line %d\n", s, d, Line);
    fclose(Outfile);
    unlink(Outfilename);
    exit(1);
}
void fatalc(char *s, int c) {
    fprintf(stderr, "%s:%c on line %d\n", s, c, Line);
    fclose(Outfile);
    unlink(Outfilename);
    exit(1);
}

char* tokenstr(int tok) {

    static char *tokstr[] = {
    "EOF",
    "=",
    "||",
    "&&",
    "|",
    "^",
    "&",
    "==", "!=",
    "<", ">", "<=", ">=",
    "<<", ">>",
    "+", "-",
    "*", "/",
    "++", "--",
    "~", "!",
    "void", "char", "int", "long",
    "id",
    "intlit",
    "strlit",
    ";", ",",
    "{", "}",
    "(", ")",
    "[", "]",
// key word
    "if", "else",
    "while",
    "for",
    "return",
};
    if ((sizeof(tokstr) / sizeof(tokstr[0])) != T_SIZE)
        fatal("miss some token str, please check");

    if (tok < 0 || tok > T_SIZE) {
        fatald("Unknown token", tok);
    }
    return tokstr[tok];

}

char* asttypestr(int asttype) {
    static char *asttypestr[] = {
    "EOF",
    "=",
    "||",
    "&&",
    "|",
    "^",
    "&",
    "==", "!=",
    "<", ">", "<=", ">=",
    "<<", ">>",
    "+", "-",
    "*", "/",
    "~",
    "!",
    "&addr",
    "*deref",
    "-neg",
    "++A", "--A",
    "A++", "A--",
    "tobool",
    "void", "char", "int", "long",
    "id",
    "intlit",
    "strlit",
    "if", "else",
    "while",
    "for",
    "functiondecl",
    "return",
    "call",
    "widen",
    "scale",
    "glue",
};
    if ((sizeof(asttypestr) / sizeof(asttypestr[0])) != A_SIZE)
        fatal("miss some asttype str, please check");

    if (asttype < A_ASSIGN || asttype >= A_SIZE) {
        fatald("Unknown asttype", asttype);
    }
    return asttypestr[asttype];

}

char* typestr(int type) {
    static char *typestr[] = {
    "none", "void", "char", "int", "long",
    "voidptr", "charptr", "intptr", "longptr",
};
    if ((sizeof(typestr) / sizeof(typestr[0])) != P_SIZE)
        fatal("miss some type str, please check");
      

    if (type < 0 || type >= P_SIZE ) {
        fatald("Unknown typeen", type);
    }

    return typestr[type];
}

#if 0
static char *stypestr[] = {
"variable", "function",
};
#endif
