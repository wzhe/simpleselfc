#include "defs.h"
#define extern_
#include "data.h"
#undef extern_
#include "decl.h"
#include <errno.h>

#include <unistd.h>


// Initialise global variables
static void init(){
  O_dumpAST = 0;
  O_keepasm = 0;   // If true, keep any assembly files
  O_assemble = 0;  // If true, assemble the assembly files
  O_dolink = 1;    // If true, link the object files
  O_verbose = 0;   // If true, print info on compilation stages

  Line = 1;
  Putback = '\n';

  Functionid = NULL;

  Infile = NULL;
  Outfile = NULL;

  Globs = 0;
  Locls = NSYMBOLS - 1;
  Globalhead = Globaltail = NULL;
  Localhead = Localtail = NULL;
  Parmhead = Parmtail = NULL;
}

// Print out a usage if started incorrectly
static void usage(char *prog) {
  fprintf(stderr, "Usage: %s [-vcST] [-o outfile] file [file ...]\n", prog);
  fprintf(stderr, "\t-v give verbose output of the compilation stages\n");
  fprintf(stderr, "\t-c generate object files but don't link them\n");
  fprintf(stderr, "\t-S generate assembly files but don't link them\n");
  fprintf(stderr, "\t-T dump the AST trees for each input file\n");
  fprintf(stderr, "\t-o outfile, produce the outfile executable file\n");
  exit(1);
}
static char* alter_suffix(char *str, char suffix) {
  char *posn;
  char *newstr;

  if ((newstr = strdup(str)) == NULL) return (NULL);

  if ((posn = strrchr(newstr, '.')) == NULL) return (NULL);

  posn++;
  if (*posn == '\0') return (NULL);

  *posn++ = suffix;
  *posn = '\0';
  return (newstr);
}

static char* do_compile(char *filename) {
  Outfilename = alter_suffix(filename, 's');
  if (Outfilename == NULL) {
    fprintf(stderr, "Error: %s has no suffix, try c on the end\n", filename);
    exit(1);
  }

  if ((Infile = fopen(filename, "r")) == NULL) {
    fprintf(stderr, "Unable to open %s:%s\n", filename, strerror(errno));
    exit(1);
  }

  // Create the output file
  if ((Outfile = fopen(Outfilename, "w")) == NULL) {
    fprintf(stderr, "Unable to create %s: %s\n", Outfilename, strerror(errno));
    exit(1);
  }

  Line = 1;
  Putback = '\n';
  clear_symtable();
  scan(&Token);
  genpreamble();
  global_declarations();
  genpostamble();

  fclose(Outfile);
  fclose(Infile);
  return (Outfilename);
}


char* do_assemble(char *filename) {
  char cmd[TEXTLEN];
  int err;

  char *outfilename = alter_suffix(filename, 'o');
  if (Outfilename == NULL) {
    fprintf(stderr, "Error: %s has no suffix, try .s on the end\n", filename);
    exit(1);
  }

  snprintf(cmd, TEXTLEN, "%s %s %s", ASCMD, outfilename, filename);
  if (O_verbose) printf("%s\n", cmd);
  err = system(cmd);
  if (err != 0) {
    fprintf(stderr, "Assemblyof %s failed", outfilename);
    exit(1);
  }
  return (outfilename);
}

void do_link(char *outfilename, char *objlist[]) {
  int cnt, size = TEXTLEN;
  char cmd[TEXTLEN], *cptr;
  int err;

  cptr = cmd;
  cnt = snprintf(cptr, size, "%s %s ", LDCMD, outfilename);
  cptr += cnt; size -= cnt;

  while (*objlist != NULL) {
    cnt = snprintf(cptr, size, "%s ", *objlist);
    cptr += cnt;
    size -= cnt;
    objlist++;
  }

  if (O_verbose) printf("%s\n", cmd);
  err = system(cmd);
  if (err != 0) {
    fprintf(stderr, "Linking failed");
    exit(1);
  }
}

int main(int argc, char* argv[]) {
  int i,j;
  char *asmfile;
  char *objfile;
  char *objlist[MAXOBJ];
  char *outfilename = "a.out";
  int objcnt = 0;
 
  init();

  for (i = 1; i < argc; i++) {
    if (argv[i][0] != '-') break;
    for (j = 1; argv[i][0] == '-' && argv[i][j]; j++) {
      switch (argv[i][j]) {
      case 'T': O_dumpAST = 1; break;
      case 'o':
	outfilename = argv[++i];
	break;
      case 'c':
	O_assemble = 1;
	O_keepasm = 0;
	O_dolink = 0;
	break;
      case 'S':
	O_assemble = 0;
	O_keepasm = 1;
	O_dolink = 0;
	break;
      case 'v': O_verbose = 1; break;
      default:
	usage(argv[0]);
      }
    }
  }

  if (i >= argc) usage(argv[0]);

  while (i < argc) {
    asmfile = do_compile(argv[i]);

    if (O_dolink || O_assemble) {
      objfile = do_assemble(asmfile);
      if (objcnt == (MAXOBJ -2)) {
	fprintf(stderr, "Too many object files for the compiler to handle\n");
	exit(1);
      }
      objlist[objcnt++] = objfile;
      objlist[objcnt] = NULL;
    }

    if (!O_keepasm) unlink(asmfile);
    i++;
  }

  if (O_dolink) {
    do_link(outfilename, objlist);

    // If we don't need to keep the object, just remove it
    if (!O_keepasm) {
      for (i = 0; objlist[i] != NULL; i++) {
	unlink(objlist[i]);
      }
    }
    for (i = 0; objlist[i] != NULL; i++) {
      free(objlist[i]);
    }
  }
  return (0);
}
