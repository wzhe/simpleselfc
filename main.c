#include "defs.h"
#define extern_
#include "data.h"
#undef extern_
#include "decl.h"
#include <errno.h>

// Initialise global variables
static void init(){
    Line = 1;
    Putback = '\n';
    Outdump = 0;
    Functionid = 0;
    Infile = NULL;
    Outfile = NULL;
    Globs = 0;
    Locls = NSYMBOLS - 1;
}

// Print out a usage if started incorrectly
static void usage(char *prog) {
    fprintf(stderr, "Usage: %s infile\n", prog);
    exit(1);
}

int main(int argc, char* argv[]) {
  int i,j;
 
  init();

  for (i = 1; i < argc; i++) {
    if (argv[i][0] != '-') break;
    for (j = 1; argv[i][j]; j++) {
      switch (argv[i][j]) {
      case 'T': Outdump = 1; break;
      default:
        usage(argv[0]);
      }
    }
  }

  if (i >= argc) usage(argv[0]);

  if ((Infile = fopen(argv[i], "r")) == NULL) {
    fprintf(stderr, "Unable to open %s:%s\n", argv[i], strerror(errno));
    exit(1);
  }

  // Create the output file
  if ((Outfile = fopen("out.s", "w")) == NULL) {
    fprintf(stderr, "Unable to create out.s: %s\n", strerror(errno));
    exit(1);
  }

  // For now, ensure that printint() is defined
  addglob("printint", P_INT, S_FUNCTION, C_GLOBAL, 0, 0);
  addglob("printchar", P_CHAR, S_FUNCTION, C_GLOBAL,0, 0);

  scan(&Token);
  genpreamble();
  global_declarations();

  fclose(Outfile);
  fclose(Infile);
  return 0;
}
