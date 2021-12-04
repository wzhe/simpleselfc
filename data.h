#ifndef DATA_H_
#define DATA_H_


#ifndef extern_
#define extern_ extern
#endif

extern_ int Line;
extern_ int Putback;
extern_ int Outdump;
extern_ int Functionid;
extern_ FILE *Infile;
extern_ FILE *Outfile;
extern_ struct token Token;

extern_ char Text[TEXTLEN + 1];
extern_ struct symtables Symtable[NSYMBOLS];
extern_ int Globs;
extern_ int Locls;

#endif // DATA_H_
