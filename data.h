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
extern_ char* Infilename;
extern_ char* Outfilename;

extern_ int Line;
extern_ int Putback;

extern_ int Looplevel;
extern_ int Switchlevel;
extern_ struct symtable *Functionid;

extern_ struct token Token;
extern_ char Text[TEXTLEN + 1];

extern_ struct symtable *Globalhead, *Globaltail;
extern_ struct symtable *Localhead, *Localtail;
extern_ struct symtable *Parmhead, *Parmtail;
extern_ struct symtable *Memberhead, *Membertail;   // Temp list of struct/union members
extern_ struct symtable *Structhead, *Structtail;   // List of struct types
extern_ struct symtable *Unionhead, *Uniontail;
extern_ struct symtable *Enumhead, *Enumtail;
extern_ struct symtable *Typedefhead, *Typedeftail;

#endif // DATA_H_
