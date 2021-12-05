#ifndef DATA_H_
#define DATA_H_


#ifndef extern_
#define extern_ extern
#endif

extern_ int O_dumpAST;   // If true, dump the AST tree
extern_ int O_keepasm;   // If true, keep any assembly files
extern_ int O_assemble;  // If true, assemble the assembly files
extern_ int O_dolink;    // If true, link the object files
extern_ int O_verbose;   // If true, print info on compilation stages

extern_ FILE *Infile;
extern_ FILE *Outfile;
extern_ char* Outfilename;

extern_ int Line;
extern_ int Putback;
extern_ int Functionid;

extern_ struct token Token;
extern_ char Text[TEXTLEN + 1];
extern_ struct symtables Symtable[NSYMBOLS];
extern_ int Globs;
extern_ int Locls;

#endif // DATA_H_
