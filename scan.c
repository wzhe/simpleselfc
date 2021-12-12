#include "defs.h"
#include "decl.h"
#include "data.h"

// A pointer to a rejected token
static struct token *Rejtoken = NULL;

// Reject the token that we just scanned
void reject_token(struct token *t) {
    if (Rejtoken != NULL) fatal("Can't reject token twice");
    Rejtoken = t;
}

// Get the next character from the input file
static int next(void) {
    int c;
    int l;
    if (Putback) {
        c = Putback;
        Putback = 0;
        return c;
    }
    c = fgetc(Infile);
    while (c == '#') {
      scan(&Token);
      if (Token.token != T_INTLIT)
	fatals("Expecting pre-processor line number, got", Text);
      l = Token.intvalue;

      scan(&Token);
      if (Token.token != T_STRLIT)
	fatals("Expecting pre-processor file name, got", Text);

      if (Text[0] != '<') {  // If this is a real file name
	if (strcmp(Text, Infilename)) {
	  Infilename = strdup(Text);
	}
	Line = l;  
      }
      while((c = fgetc(Infile)) != '\n'); // Skip to the end of the line
      c = fgetc(Infile);
	
    }
    if ('\n' == c) Line++;
    return c;
}

// Put back an unwated character
static void putback(int c) {
    Putback = c;
}


// Skip past input that we don't need to deal with i.e. whitespace, newlines.
// Return the first character we do need to deal with.
static int skip(void) {
    int c;
    c = next();
    while(' ' == c || '\t' == c || '\n' == c || '\r' == c || '\f' == c) {
        c = next();
    }
    return c;
}

// Return the position of character c in string s,
// or -1 if c not found
static int chrpos(char *s, int c) {
    char *p;
    p = strchr(s, c);
    return (p ? p - s : -1);
}

// Scan and return and integer literal value from the input file.
// Store the value as a string in Text.
static int scanint(int c) {
    int k, val = 0;

    while ((k = chrpos("0123456789", c)) >= 0) {
        val = val * 10 + k;
        c = next();
    }

    // We hit a non-integer character, put it back;
    putback(c);
    return val;
}

// Scan and return and integer literal value from the input file.
// Store the value as a char in Text.
static int scanchar() {
    int c;
    c = next();
    if (c == '\\') {
        switch (c = next()) {
            case 'a': return '\a';
            case 'b': return '\b';
            case 'f': return '\f';
            case 'n': return '\n';
            case 'r': return '\r';
            case 't': return '\t';
            case 'v': return '\v';
            case '\\': return '\\';
            case '"': return '"';
            case '\'': return '\'';
            default:
                fatalc("unknown escape sequence", c);
        }
    }

    return (c);
}

// Scan in a string literal from the input file,
// and store it in buf[]. Return the length of the string.

static int scanstr(char *buf) {
    int i, c;

    for (i = 0; i < TEXTLEN - 1; i++) {
        // Get the next char and append to buf
        // Return when we hit the ending double quote
        if ((c = scanchar()) == '"') {
            buf[i] = '\0';
            return (i);
        }
        buf[i] = c;
    }

    // Ran out of buf[] space
    fatal("String literal too long");
    return (0);
}

// Scan an identifier from the input file and
// store it in buf[]. Return the identifier's length
static int scanident(int c, char *buf, int lim) {
    int i = 0;

    // Allow digits, alpha and underscores
    while (isalpha(c) || isdigit(c) || '_' == c) {
        if (lim - 1 == i) {
            printf("identifier too long on line %d\n", Line);
            exit(1);
        } else if (i < lim - 1) {
            buf[i++] = c;
        }
        c = next();
    }
    // We hit a non-valid character, put it back.
    // NUL-terminate the buf[] and return the length
    putback(c);
    buf[i] = '\0';
    return (i);
}


// Given a word from the input, return the matching
// keyword token number or 0 if it's not a keyword.
// Switch on the first letter so that we don't have
// to waste time strcmp()ing against all the keywords.
static int keyword(char *s) {
    switch (*s) {
        case 'c':
            if (!strcmp(s, "char"))
                return (T_CHAR);
            break;
        case 'e':
            if (!strcmp(s, "else"))
                return (T_ELSE);
            if (!strcmp(s, "enum"))
                return (T_ENUM);
            if (!strcmp(s, "extern"))
                return (T_EXTERN);
            break;
        case 'f':
            if (!strcmp(s, "for"))
                return (T_FOR);
            break;
        case 'i':
            if (!strcmp(s, "if"))
                return (T_IF);
            if (!strcmp(s, "int"))
                return (T_INT);
            break;
        case 'l':
            if (!strcmp(s, "long"))
                return (T_LONG);
            break;
        case 'r':
            if (!strcmp(s, "return"))
                return (T_RETURN);
            break;
        case 's':
            if (!strcmp(s, "struct"))
                return (T_STRUCT);
            break;
        case 't':
            if (!strcmp(s, "typedef"))
                return (T_TYPEDEF);
            break;
        case 'u':
            if (!strcmp(s, "union"))
                return (T_UNION);
            break;
        case 'v':
            if (!strcmp(s, "void"))
                return (T_VOID);
            break;
        case 'w':
            if (!strcmp(s, "while"))
                return (T_WHILE);
            break;
    }
    return (0);
}

static void showtoken(struct token *t) {
    printf("Token %s", tokenstr(t->token));
    if (t->token == T_INTLIT) printf(",value %d", t->intvalue);
    else if (t->token == T_IDENT) printf(",value %s", Text);
    printf("\n");
}


// Scan and return the next token found in the input.
// Return 1 if token valid, 0 if no tokens left.
int scan(struct token *t) {
    int c, tokentype;

    // If we have any rejected token, return it
    if (Rejtoken != NULL) {
        t = Rejtoken;
        Rejtoken = NULL;
        return (1);
    }

    // Skip whitespace
    c = skip();

    // Determine the token based on the input character
    switch (c) {
        case EOF:
            t->token = T_EOF;
            return (0);
        case '{':
            t->token = T_LBRACE;
            break;
        case '}':
            t->token = T_RBRACE;
            break;
        case '(':
            t->token = T_LPAREN;
            break;
        case ')':
            t->token = T_RPAREN;
            break;
        case '[':
            t->token = T_LBRACKET;
            break;
        case ']':
            t->token = T_RBRACKET;
            break;
        case '+':
            if ((c = next()) == '+') {
                t->token = T_INC;
            } else {
                putback(c);
                t->token = T_PLUS;
            }
            break;
        case '-':
            if ((c = next()) == '-') {
                t->token = T_DEC;
	    } else if (c == '>') {
                t->token = T_ARROW;
            } else {
                putback(c);
                t->token = T_MINUS;
            }
            break;
        case '*':
            t->token = T_STAR;
            break;
        case '/':
            t->token = T_SLASH;
            break;
        case ';':
            t->token = T_SEMI;
            break;
        case ',':
            t->token = T_COMMA;
            break;
        case '=':
            if ((c = next()) == '=') {
                t->token = T_EQ;
            } else {
                putback(c);
                t->token = T_ASSIGN;
            }
            break;
        case '!':
            if ((c = next()) == '=') {
                t->token = T_NE;
            } else {
                putback(c);
                t->token = T_LOGNOT;
            }
            break;
        case '<':
            if ((c = next()) == '=') {
                t->token = T_LE;
            } else if (c == '<') {
                t->token = T_LSHIFT;
            } else {
                putback(c);
                t->token = T_LT;
            }
            break;
        case '>':
            if ((c = next()) == '=') {
                t->token = T_GE;
            } else if (c == '>') {
                t->token = T_RSHIFT;
            } else {
                putback(c);
                t->token = T_GT;
            }
            break;
        case '&':
            if ((c = next()) == '&') {
                t->token = T_LOGAND;
            } else {
                putback(c);
                t->token = T_AMPER;
            }
            break;
        case '|':
            if ((c = next()) == '|') {
                t->token = T_LOGOR;
            } else {
                putback(c);
                t->token = T_OR;
            }
            break;
        case '^':
            t->token = T_XOR;
            break;
        case '~':
            t->token = T_INVERT;
            break;
        case '.':
            t->token = T_DOT;
            break;
        case '\'':
            // If it's a quote, scan in the literal character value
            // and the trailing quote
            t->intvalue = scanchar();
            t->token = T_INTLIT;
            if (next() != '\'')
                fatal("Expected '\\'' at end of char literal");
            break;
        case '"':
            // Scan in a literal string
            scanstr(Text);
            t->token = T_STRLIT;
            break;
        default:
            // if it's a digit, scan the literal integer value
            if (isdigit(c)) {
                t->intvalue = scanint(c);
                t->token = T_INTLIT;
                break;
            } else if (isalpha(c) || '_' == c) {
                // Read in a keyword or identifier
                scanident(c, Text, TEXTLEN);

                // If it's a recognised keyword, return that token
                if ((tokentype = keyword(Text))) {
                    t->token = tokentype;
                    break;
                }
                t->token = T_IDENT;
                break;
            }
    }
    // Debug: Print what we are about to do
    if (O_dumpAST) showtoken(t);
    // We found a token
    return (1);
}
