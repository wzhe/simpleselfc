#include "defs.h"
#include "data.h"
#include "decl.h"
#include "cg.h"

#define NUMFREEREGS 4
#define FIRSTPARAMRE  9
static int freereg[NUMFREEREGS];

static char *reglist[] =
  {"%r10", "%r11", "%r12", "%r13", "%r9", "%r8", "%rcx", "%rdx", "%rsi", "%rdi" };
static char *breglist[] =
  {"%r10b", "%r11b", "%r12b", "%r13b", "%r9b", "%r8b", "%cl", "%dl", "%sil", "%dil" };
static char *dreglist[] =
  {"%r10d", "%r11d", "%r12d", "%r13d", "%r9d", "%r8d", "%ecx", "%edx", "%esi", "%edi" };

static int psize[] = {
		      0,  // P_NONE,
		      0,  // P_VOID,
		      1,  // P_CHAR,
		      4,  // P_INT,
		      8,  // P_LONG,
		      8,  // P_CHARPTR
		      8,  // P_INTPTR
		      8,  // P_LONGPTR
};

// Position of next local variable relative to stack base pointer.
// We store the offset as positive to make aligning the stack pointer easier
static int localOffset;
static int stackOffset;

// Flag to say which section were are outputing in to
enum { no_seg, text_seg, data_seg} currSeg = no_seg;

static int alloc_register(void) {
  int i;
  for (i = 0; i < NUMFREEREGS; i++) {
    if (freereg[i]) {
      freereg[i] = 0;
      return (i);
    }
  }
  fatal("Out of registers!\n");
  return (NOREG);
}

static void free_register(int reg) {
  if (freereg[reg] != 0) {
    fatald("Error trying to free register %d!\n", reg);
  }
  freereg[reg] = 1;
}

// Get the position of the next local variable.
// Use the isparam flag to allocate a parameter (not yet XXX).
static int cggetlocaloffset(int type) {
  // Decrement the offset by a minimum of 4 bytes
  // and allocate on the stack
  localOffset += (cgprimsize(type) > 4) ? cgprimsize(type) : 4;
  return (-localOffset);
}

int cgprimsize(int type) {
  if (type < P_NONE || type > P_LONGPTR)
    fatald("Bad type in cgprimsize()", type);
  return (psize[type]);
}

void freeall_registers(void)
{
  freereg[0] = 1;
  freereg[1] = 1;
  freereg[2] = 1;
  freereg[3] = 1;
}

// Print out the assembly preamble
void cgpreamble(){
  freeall_registers();
}

// Print out the assembly postamble
void cgpostamble() {
  fputs(
	"\tmovl\t$0, %eax\n"
	"\tpopq\t%rbp\n"
	"\tret\n",
	Outfile);
}


static void cgtextseg() {
  if (currSeg != text_seg) {
    fprintf(Outfile, "\t.text\n");
    currSeg = text_seg;
  }
}
static void cgdataseg() {
  if (currSeg != data_seg) {
    fprintf(Outfile, "\t.data\n");
    currSeg = data_seg;
  }
}

void cglabel(int l) {
  fprintf(Outfile, "L%d:\n", l);
}

void cgjump(int l) {
  fprintf(Outfile, "\tjmp\tL%d\n", l);
}
// Print out a function preamble
void cgfuncpreamble(int id) {
  char *name = Symtable[id].name;
  int i;
  int paramOffset = 16;        // Any pushed param start at this stack offset.
  int paramReg = FIRSTPARAMRE; // Index to the first param register in above reg list

  // Output in the text segment, reset local offset
  cgtextseg();
  localOffset = 0;

  // Output the function start, save the %rsp and rbp
  fprintf(Outfile,
	  "\t.globl\t%s\n"
	  "\t.type\t%s, @function\n"
	  "%s:\n"
	  "\tpushq\t%%rbp\n"
	  "\tmovq\t%%rsp, %%rbp\n",
	  name, name, name);

  // Copy any in-register parameters to the stack
  // Stop after no more than six parameter registers
  for (i = NSYMBOLS - 1; i > Locls; i--) {
    if (Symtable[i].clas != C_PARAM) break;
    if (i < NSYMBOLS - 6) break;
    Symtable[i].posn = cggetlocaloffset(Symtable[i].type);
    cgstorlocal(paramReg--, i);
  }
  // For the remainder, if they are a parameter then they are
  // already on the stack. If only a local, make a stack position.
  for (; i > Locls; i--) {
    if (Symtable[i].clas == C_PARAM) {
      Symtable[i].posn = paramOffset;
      paramOffset += 8;
    } else {
      Symtable[i].posn = cggetlocaloffset(Symtable[i].type);
    }
  }

  // Align the stack pointer to be a multiple of 16
  // less than its previous value
  stackOffset = (localOffset + 15) & ~15;
  fprintf(Outfile, "\taddq\t$%d, %%rsp\n", -stackOffset);
}

void cgfuncpostamble(int id) {
  cglabel(Symtable[id].endlabel);
  fprintf(Outfile, "\taddq\t$%d, %%rsp\n", stackOffset);
  fputs("\tpopq\t%rbp\n" "\tret\n", Outfile);
}

// Load an integer literal value into a register
int cgloadint(int value) {
  // Get a new register
  int r = alloc_register();

  // Print out the code to initialise it
  fprintf(Outfile, "\tmovq\t$%d, %s\n", value, reglist[r]);
  return (r);
}

// Generate a global symbol
void cgglobsym(int id) {
  int typesize;
  int i;

  if (Symtable[id].stype == S_FUNCTION)
    return;

  // Get the size of the type
  typesize = cgprimsize(Symtable[id].type);

  // Generate the golbl identify and the label
  cgdataseg();
  fprintf(Outfile, "\t.globl\t%s\n", Symtable[id].name);
  fprintf(Outfile, "%s:\n", Symtable[id].name);

  // Genera the space
  for (i = 0; i < Symtable[id].size; i++) {
    switch(typesize) {
    case 1: fprintf(Outfile, "\t.byte\t0\n"); break;
    case 4: fprintf(Outfile, "\t.long\t0\n"); break;
    case 8: fprintf(Outfile, "\t.quad\t0\n"); break;
    default: fatald("Unknown typesize in cgglobsym", typesize);
    }
  } 
}

int cgstorglob(int r, int id) {
  switch(Symtable[id].type){
  case P_CHAR:
    fprintf(Outfile, "\tmovb\t%s, %s(%%rip)\n", breglist[r], Symtable[id].name);
    break;
  case P_INT:
    fprintf(Outfile, "\tmovl\t%s, %s(%%rip)\n", dreglist[r], Symtable[id].name);
    break;
  case P_LONG:
  case P_CHARPTR:
  case P_INTPTR:
  case P_LONGPTR:
    fprintf(Outfile, "\tmovq\t%s, %s(%%rip)\n", reglist[r], Symtable[id].name);
    break;
  default:
    fatald("Bad type in cgstorglob", Symtable[id].type);
  }
  return (r);
}
int cgstorlocal(int r, int id) {
  switch(Symtable[id].type){
  case P_CHAR:
    fprintf(Outfile, "\tmovb\t%s, %d(%%rbp)\n", breglist[r], Symtable[id].posn);
    break;
  case P_INT:
    fprintf(Outfile, "\tmovl\t%s, %d(%%rbp)\n", dreglist[r], Symtable[id].posn);
    break;
  case P_LONG:
  case P_CHARPTR:
  case P_INTPTR:
  case P_LONGPTR:
    fprintf(Outfile, "\tmovq\t%s, %d(%%rbp)\n", reglist[r], Symtable[id].posn);
    break;
  default:
    fatald("Bad type in cgstorlocal", Symtable[id].type);
  }
  return (r);
}
// Call a function with the given symbol id
// Pop off any arguments pushed on the stack
// Return the register with result
int cgcall(int id, int numargs) {

  int outr = alloc_register();

  // Call the function
  fprintf(Outfile, "\tcall\t%s\n", Symtable[id].name);
  // Remove any arguments pushed on the stack
  if (numargs > 6) 
    fprintf(Outfile, "\taddq\t$%d, %%rsp\n", 8*(numargs-6));
  
  fprintf(Outfile, "\tmovq\t%%rax, %s\n", reglist[outr]);

  return (outr);
}
void cgreturn(int r, int id){
  switch(Symtable[id].type){
  case P_CHAR:
    fprintf(Outfile, "\tmovzbl\t%s, %%eax\n", breglist[r]);
    break;
  case P_INT:
    fprintf(Outfile, "\tmovl\t%s, %%eax\n", dreglist[r]);
    break;
  case P_LONG:
    fprintf(Outfile, "\tmovq\t%s, %%eax\n", reglist[r]);
    break;
  default:
    fatald("Bad type in cgreturn:", Symtable[id].type);
  }
  cgjump(Symtable[id].endlabel);
}

// Widen the value in the register
// from the old to the new type,
// and return a registre with this new value
int cgwiden(int r, int oldtype, int newtype) {
  // Nothing to do
  (void)oldtype;
  (void)newtype;
  return (r);
}

static void checkreg(int r) {
  if (r < 0 || r > NUMFREEREGS) fatald("register out range. reg", r);
}

int cgadd(int r1, int r2) {
  checkreg(r1);
  checkreg(r2);
  fprintf(Outfile, "\taddq\t%s, %s\n", reglist[r1], reglist[r2]);
  free_register(r1);
  return (r2);
}

int cgsub(int r1, int r2) {
  fprintf(Outfile, "\tsubq\t%s, %s\n", reglist[r2], reglist[r1]);
  free_register(r2);
  return (r1);
}

int cgmul(int r1, int r2) {
  fprintf(Outfile, "\timulq\t%s, %s\n", reglist[r1], reglist[r2]);
  free_register(r1);
  return (r2);
}

// Divide the first register by the second and
// return the number of the register with the result
int cgdiv(int r1, int r2){
  fprintf(Outfile, "\tmovq\t%s, %%rax\n", reglist[r1]);
  fprintf(Outfile, "\tcqo\n");
  fprintf(Outfile, "\tidivq\t%s\n",reglist[r2]);
  fprintf(Outfile, "\tmovq\t%%rax,%s\n",reglist[r1]);
  free_register(r2);
  return (r1);
}

// Compare two registers
static int cgcompare(int r1, int r2, char *how) {
  fprintf(Outfile, "\tcmpq\t%s, %s\n", reglist[r2], reglist[r1]);
  fprintf(Outfile, "\t%s\t%s\n", how,breglist[r2]);
  fprintf(Outfile, "\tandq\t$255,%s\n", reglist[r2]);
  free_register(r1);
  return (r2);
}

int cgequal(int r1, int r2) {
  return (cgcompare(r1, r2, "sete"));
}
int cgnotequal(int r1, int r2) {
  return (cgcompare(r1, r2, "setne"));
}
int cglessthan(int r1, int r2) {
  return (cgcompare(r1, r2, "setl"));
}
int cggreaterthan(int r1, int r2) {
  return (cgcompare(r1, r2, "setg"));
}
int cglessequal(int r1, int r2) {
  return (cgcompare(r1, r2, "setle"));
}
int cggreaterequal(int r1, int r2) {
  return (cgcompare(r1, r2, "setge"));
}

void cgprintint(int r) {
  fprintf(Outfile, "\tmovq\t%s, %%rdi\n", reglist[r]);
  fprintf(Outfile, "\tcall\tprintint\n");
  free_register(r);
}

// List of comparison instructions
// in AST order: A_EQ,
static char *cmplist[] = {
			  "sete", "setne", "setl", "setg", "setle", "setge"
};
static char *invcmplist[] = {
			     "jne", "je", "jge", "jle", "jg", "jl"
};

// Compare two registers and set if true
int cgcompare_and_set(int ASTop, int r1, int r2) {
  // Check
  if (ASTop < A_EQ || ASTop > A_GE) {
    fatald("Bad ASTop in cgcompare_and_set", ASTop);
  }

  fprintf(Outfile, "\tcmpq\t%s, %s\n", reglist[r2], reglist[r1]);
  fprintf(Outfile, "\t%s\t%s\n", cmplist[ASTop - A_EQ], breglist[r2]);
  fprintf(Outfile, "\tmovzbq\t%s, %s\n", breglist[r2], reglist[r2]);
  free_register(r1);
  return (r2);
}

int cgcompare_and_jump(int ASTop, int r1, int r2, int label) {
  // Check
  if (ASTop < A_EQ || ASTop > A_GE) {
    fatald("Bad ASTop in cgcompare_and_jump", ASTop);
  }

  fprintf(Outfile, "\tcmpq\t%s, %s\n", reglist[r2], reglist[r1]);
  fprintf(Outfile, "\t%s\tL%d\n", invcmplist[ASTop - A_EQ], label);
  freeall_registers();
  return (NOREG);
}

// Generaate code to load the addres of a global
// identifier into a variable. Return a new register
int cgaddress(int id) {
  int r = alloc_register();

  if (Symtable[id].clas == C_GLOBAL) {
    fprintf(Outfile, "\tleaq\t%s(%%rip), %s\n", Symtable[id].name, reglist[r]);
  }
  else if (Symtable[id].clas == C_LOCAL) {
    fprintf(Outfile, "\tleaq\t%d(%%rbp), %s\n", Symtable[id].posn, reglist[r]);
  } else {
    fatals("Now not support addres param:", Symtable[id].name);
  }

  return (r);
}


// Dereference a pointer to get the value it
// pointing at into the same register
int cgderef(int r, int type){
  switch(type) {
  case P_CHARPTR:
    fprintf(Outfile, "\tmovzbq\t(%s), %s\n", reglist[r], reglist[r]);
    break;
  case P_INTPTR:
  case P_LONGPTR:
    fprintf(Outfile, "\tmovq\t(%s), %s\n", reglist[r], reglist[r]);
    break;
  }

  return (r);
}

//Shift a register left by a constant
int cgshlconst(int r, int val) {
  fprintf(Outfile, "\tsalq\t$%d, %s\n", val, reglist[r]);
  return (r);
}

// Store through a dereferenced pointer
int cgstorderef(int r1, int r2, int type) {
  switch (type) {
  case P_CHAR:
    fprintf(Outfile, "\tmovb\t%s, (%s)\n", breglist[r1], reglist[r2]);
    break;
  case P_INT:
    fprintf(Outfile, "\tmovq\t%s, (%s)\n", reglist[r1], reglist[r2]);
    break;
  case P_LONG:
    fprintf(Outfile, "\tmovq\t%s, (%s)\n", reglist[r1], reglist[r2]);
    break;
  default:
    fatald("Can't cgstorderef on type:", type);
  }

  return (r1);

}

void cgglobstr(int label,char *strvalue) {
  char *cptr;
  cglabel(label);
  for (cptr = strvalue; *cptr; cptr++) {
    fprintf(Outfile, "\t.byte\t%d\n", *cptr);
  }
  fprintf(Outfile, "\t.byte\t0\n");
}

int cgloadglobstr(int label) {
  int r = alloc_register();
  fprintf(Outfile, "\tleaq\tL%d(%%rip), %s\n",label, reglist[r]);
  return (r);
}

int cgand(int r1, int r2) {
  fprintf(Outfile, "\tandq\t%s, %s\n", reglist[r1], reglist[r2]);
  free_register(r1); return (r2);
}

int cgor(int r1, int r2) {
  fprintf(Outfile, "\torq\t%s, %s\n", reglist[r1], reglist[r2]);
  free_register(r1); return (r2);
}

int cgxor(int r1, int r2) {
  fprintf(Outfile, "\txorq\t%s, %s\n", reglist[r1], reglist[r2]);
  free_register(r1); return (r2);
}

// Negate a register's value
int cgnegate(int r) {
  fprintf(Outfile, "\tnegq\t%s\n", reglist[r]); return (r);
}

// Invert a register's value
int cginvert(int r) {
  fprintf(Outfile, "\tnotq\t%s\n", reglist[r]); return (r);
}

int cgshl(int r1, int r2) {
  fprintf(Outfile, "\tmovb\t%s, %%cl\n", breglist[r2]);
  fprintf(Outfile, "\tshlq\t%%cl, %s\n", reglist[r1]);
  free_register(r2); return (r1);
}

int cgshr(int r1, int r2) {
  fprintf(Outfile, "\tmovb\t%s, %%cl\n", breglist[r2]);
  fprintf(Outfile, "\tshrq\t%%cl, %s\n", reglist[r1]);
  free_register(r2); return (r1);
}
// Logically negate a register's value
int cglognot(int r) {
  fprintf(Outfile, "\ttest\t%s, %s\n", reglist[r], reglist[r]);
  fprintf(Outfile, "\tsete\t%s\n", breglist[r]);
  fprintf(Outfile, "\tmovzbq\t%s, %s\n", breglist[r], reglist[r]);
  return (r);
}
// Convert an integer value to a boolean value. Jump if
// it's an IF or WHILE operation
int cgboolean(int r, int op, int label) {
  fprintf(Outfile, "\ttest\t%s, %s\n", reglist[r], reglist[r]);
  if (op == A_IF || op == A_WHILE)
    fprintf(Outfile, "\tje\tL%d\n", label);
  else {
    fprintf(Outfile, "\tsetnz\t%s\n", breglist[r]);
    fprintf(Outfile, "\tmovzbq\t%s, %s\n", breglist[r], reglist[r]);
  }
  return (r);
}

// Load a value from a variable into a register.
// Return the number of the register. If the
// operation is pre- or post-increment/decrement,
// also perform this action.
int cgloadglob(int id, int op) {
  // Get a new register
  int r = alloc_register();

  // Print out the code to initialise it
  switch (Symtable[id].type) {
  case P_CHAR:
    if (op == A_PREINC)
      fprintf(Outfile, "\tincb\t%s(%%rip)\n", Symtable[id].name);
    if (op == A_PREDEC)
      fprintf(Outfile, "\tdecb\t%s(%%rip)\n", Symtable[id].name);
    fprintf(Outfile, "\tmovzbq\t%s(%%rip), %s\n", Symtable[id].name, reglist[r]);
    if (op == A_POSTINC)
      fprintf(Outfile, "\tincb\t%s(%%rip)\n", Symtable[id].name);
    if (op == A_POSTDEC)
      fprintf(Outfile, "\tdecb\t%s(%%rip)\n", Symtable[id].name);
    break;
  case P_INT:
    if (op == A_PREINC)
      fprintf(Outfile, "\tincl\t%s(%%rip)\n", Symtable[id].name);
    if (op == A_PREDEC)
      fprintf(Outfile, "\tdecl\t%s(%%rip)\n", Symtable[id].name);
    fprintf(Outfile, "\tmovslq\t%s(%%rip), %s\n", Symtable[id].name, reglist[r]);
    if (op == A_POSTINC)
      fprintf(Outfile, "\tincl\t%s(%%rip)\n", Symtable[id].name);
    if (op == A_POSTDEC)
      fprintf(Outfile, "\tdecl\t%s(%%rip)\n", Symtable[id].name);
    break;
  case P_LONG:
  case P_CHARPTR:
  case P_INTPTR:
  case P_LONGPTR:
    if (op == A_PREINC)
      fprintf(Outfile, "\tincq\t%s(%%rip)\n", Symtable[id].name);
    if (op == A_PREDEC)
      fprintf(Outfile, "\tdecq\t%s(%%rip)\n", Symtable[id].name);
    fprintf(Outfile, "\tmovq\t%s(%%rip), %s\n", Symtable[id].name, reglist[r]);
    if (op == A_POSTINC)
      fprintf(Outfile, "\tincq\t%s(%%rip)\n", Symtable[id].name);
    if (op == A_POSTDEC)
      fprintf(Outfile, "\tdecq\t%s(%%rip)\n", Symtable[id].name);
    break;
  default:
    fatald("Bad type in cgloadglob:", Symtable[id].type);
  }
  return (r);
}

int cgloadlocal(int id, int op) {
  // Get a new register
  int r = alloc_register();

  // Print out the code to initialise it
  switch (Symtable[id].type) {
  case P_CHAR:
    if (op == A_PREINC)
      fprintf(Outfile, "\tincb\t%d(%%rbp)\n", Symtable[id].posn);
    if (op == A_PREDEC)
      fprintf(Outfile, "\tdecb\t%d(%%rbp)\n", Symtable[id].posn);
    fprintf(Outfile, "\tmovzbq\t%d(%%rbp), %s\n", Symtable[id].posn, reglist[r]);
    if (op == A_POSTINC)
      fprintf(Outfile, "\tincb\t%d(%%rbp)\n", Symtable[id].posn);
    if (op == A_POSTDEC)
      fprintf(Outfile, "\tdecb\t%d(%%rbp)\n", Symtable[id].posn);
    break;
  case P_INT:
    if (op == A_PREINC)
      fprintf(Outfile, "\tincl\t%d(%%rbp)\n", Symtable[id].posn);
    if (op == A_PREDEC)
      fprintf(Outfile, "\tdecl\t%d(%%rbp)\n", Symtable[id].posn);
    fprintf(Outfile, "\tmovslq\t%d(%%rbp), %s\n", Symtable[id].posn, reglist[r]);
    if (op == A_POSTINC)
      fprintf(Outfile, "\tincl\t%d(%%rbp)\n", Symtable[id].posn);
    if (op == A_POSTDEC)
      fprintf(Outfile, "\tdecl\t%d(%%rbp)\n", Symtable[id].posn);
    break;
  case P_LONG:
  case P_CHARPTR:
  case P_INTPTR:
  case P_LONGPTR:
    if (op == A_PREINC)
      fprintf(Outfile, "\tincq\t%d(%%rbp)\n", Symtable[id].posn);
    if (op == A_PREDEC)
      fprintf(Outfile, "\tdecq\t%d(%%rbp)\n", Symtable[id].posn);
    fprintf(Outfile, "\tmovq\t%d(%%rbp), %s\n", Symtable[id].posn, reglist[r]);
    if (op == A_POSTINC)
      fprintf(Outfile, "\tincq\t%d(%%rbp)\n", Symtable[id].posn);
    if (op == A_POSTDEC)
      fprintf(Outfile, "\tdecq\t%d(%%rbp)\n", Symtable[id].posn);
    break;
  default:
    fatald("Bad type in cgloadlocal:", Symtable[id].type);
  }
  return (r);
}

// Given a registre with an argument value,
// copy this argument into the argposn'th parameter
// in preparation for a future function call.
// Note that argposn is 1,2,3,4,..., never zero.
void cgcopyarg(int r, int argposn) {
  // If this is above the sixth argument, simply push the
  // register on the stack. We rely on being called with
  // successive arguments in the correct order for x86-64
  if (argposn > 6) {
    fprintf(Outfile, "\tpushq\t%s\n", reglist[r]);
  } else {
    // Otherwise, copy the value into one of the six registers
    // used to hold parameter values
    // Note : not zeor base, need +1
    fprintf(Outfile, "\tmovq\t%s, %s\n", reglist[r],
	    reglist[FIRSTPARAMRE - argposn + 1]);
  }
}


