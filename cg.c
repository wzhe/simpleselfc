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
static int newlocaloffset(int type) {
  // Decrement the offset by a minimum of 4 bytes
  // and allocate on the stack
  localOffset += (cgprimsize(type) > 4) ? cgprimsize(type) : 4;
  return (-localOffset);
}

int cgprimsize(int type) {
  if (ptrtype(type)) return (8);
  switch(type) {
    case P_CHAR: return 1;
    case P_INT: return 4;
    case P_LONG: return 8;
    default:
      fatald("Unknow type in cgprimsize", type);
  }
  return (0);
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
  fprintf(Outfile,
          "switch:\n"
          "\tpushq\t%%rsi\n"
          "\tmovq\t%%rdx, %%rsi\n"
          "\tmovq\t%%rax, %%rbx\n"
          "\tcld\n"
          "\tlodsq\n"
          "\tmovq\t%%rax, %%rcx\n"
          "next:\n"
          "\tlodsq\n"
          "\tmovq\t%%rax, %%rdx\n"
          "\tlodsq\n"
          "\tcmpq\t%%rdx, %%rbx\n"
          "\tjnz\tno\n"
          "\tpopq\t%%rsi\n"
          "\tjmp\t*%%rax\n"
          "no:\n"
          "\tloop\tnext\n"
          "\tlodsq\n"
          "\tpopq\t%%rsi\n"
          "\tjmp\t*%%rax\n");
}

// Nothing to do
void cgpostamble() {
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
void cgfuncpreamble(struct symtable *n) {
  char *name = n->name;
  int cnt;
  struct symtable *parm;
  struct symtable *localvar;
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
  for (parm = n->member, cnt = 1; parm != NULL; parm = parm->next, cnt++) {
    if (cnt > 6) {
      parm->posn = paramOffset;
      paramOffset += 8;
    } else {
      parm->posn = newlocaloffset(parm->type);
      cgstorlocal(paramReg--, parm);
    }
  }
  // For the remainder, if they are a parameter then they are
  // already on the stack. If only a local, make a stack position.
  for (localvar = Localhead; localvar != NULL; localvar = localvar->next) {
    localvar->posn = newlocaloffset(localvar->type);
  }

  // Align the stack pointer to be a multiple of 16
  // less than its previous value
  stackOffset = (localOffset + 15) & ~15;
  fprintf(Outfile, "\taddq\t$%d, %%rsp\n", -stackOffset);
}

void cgfuncpostamble(struct symtable *n) {
  cglabel(n->endlabel);
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
void cgglobsym(struct symtable *n) {
  int size;
  int i;

  if (n == NULL) return;
  if (n->stype == S_FUNCTION)
    return;

  // Get the size of the type
  size = typesize(n->type, n->ctype);

  // Generate the golbl identify and the label
  cgdataseg();
  fprintf(Outfile, "\t.globl\t%s\n", n->name);
  fprintf(Outfile, "%s:\n", n->name);

  // Genera the space
  switch(size) {
    case 1: fprintf(Outfile, "\t.byte\t0\n"); break;
    case 4: fprintf(Outfile, "\t.long\t0\n"); break;
    case 8: fprintf(Outfile, "\t.quad\t0\n"); break;
    default:
      for (i = 0; i < size; i++) {
        fprintf(Outfile, "\t.byte\t0\n");
      }
  } 
}

int cgstorglob(int r, struct symtable *n) {
  int size = cgprimsize(n->type);
  switch(size){
    case 1:
      fprintf(Outfile, "\tmovb\t%s, %s(%%rip)\n", breglist[r], n->name);
      break;
    case 4:
      fprintf(Outfile, "\tmovl\t%s, %s(%%rip)\n", dreglist[r], n->name);
      break;
    case 8:
      fprintf(Outfile, "\tmovq\t%s, %s(%%rip)\n", reglist[r], n->name);
      break;
    default:
      fatald("Bad type in cgstorglob", n->type);
  }
  return (r);
}
int cgstorlocal(int r, struct symtable* n) {
  int size = cgprimsize(n->type);
  switch(size){
    case 1:
      fprintf(Outfile, "\tmovb\t%s, %d(%%rbp)\n", breglist[r], n->posn);
      break;
    case 4:
      fprintf(Outfile, "\tmovl\t%s, %d(%%rbp)\n", dreglist[r], n->posn);
      break;
    case 8:
      fprintf(Outfile, "\tmovq\t%s, %d(%%rbp)\n", reglist[r], n->posn);
      break;
    default:
      fatald("Bad type in cgstorlocal", n->type);
  }
  return (r);
}
// Call a function with the given symbol id
// Pop off any arguments pushed on the stack
// Return the register with result
int cgcall(struct symtable *n, int numargs) {

  int outr = alloc_register();

  // Call the function
  fprintf(Outfile, "\tcall\t%s\n", n->name);
  // Remove any arguments pushed on the stack
  if (numargs > 6) 
    fprintf(Outfile, "\taddq\t$%d, %%rsp\n", 8*(numargs-6));
  
  fprintf(Outfile, "\tmovq\t%%rax, %s\n", reglist[outr]);

  return (outr);
}
void cgreturn(int r, struct symtable *n){
  int size = cgprimsize(n->type);
  switch(size){
    case 1:
      fprintf(Outfile, "\tmovzbl\t%s, %%eax\n", breglist[r]);
      break;
    case 4:
      fprintf(Outfile, "\tmovl\t%s, %%eax\n", dreglist[r]);
      break;
    case 8:
      fprintf(Outfile, "\tmovq\t%s, %%eax\n", reglist[r]);
      break;
    default:
      fatald("Bad type in cgreturn:", n->type);
  }
  cgjump(n->endlabel);
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
int cgaddress(struct symtable *n) {
  int r = alloc_register();

  if (n->clas == C_GLOBAL) {
    fprintf(Outfile, "\tleaq\t%s(%%rip), %s\n", n->name, reglist[r]);
  }
  else if (n->clas == C_LOCAL) {
    fprintf(Outfile, "\tleaq\t%d(%%rbp), %s\n", n->posn, reglist[r]);
  } else {
    fatals("Now not support addres param:", n->name);
  }

  return (r);
}


// Dereference a pointer to get the value it
// pointing at into the same register
int cgderef(int r, int type){
  int newtype = value_at(type);
  int size = cgprimsize(newtype);
  switch(size) {
    case 1:
      fprintf(Outfile, "\tmovzbq\t(%s), %s\n", reglist[r], reglist[r]);
      break;
    case 4:
    case 8:
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
int cgloadglob(struct symtable *n, int op) {
  // Get a new register
  int r = alloc_register();

  int size = cgprimsize(n->type);
  // Print out the code to initialise it
  switch(size){
    case 1:
      if (op == A_PREINC)
        fprintf(Outfile, "\tincb\t%s(%%rip)\n", n->name);
      if (op == A_PREDEC)
        fprintf(Outfile, "\tdecb\t%s(%%rip)\n", n->name);
      fprintf(Outfile, "\tmovzbq\t%s(%%rip), %s\n", n->name, reglist[r]);
      if (op == A_POSTINC)
        fprintf(Outfile, "\tincb\t%s(%%rip)\n", n->name);
      if (op == A_POSTDEC)
        fprintf(Outfile, "\tdecb\t%s(%%rip)\n", n->name);
      break;
    case 4:
      if (op == A_PREINC)
        fprintf(Outfile, "\tincl\t%s(%%rip)\n", n->name);
      if (op == A_PREDEC)
        fprintf(Outfile, "\tdecl\t%s(%%rip)\n", n->name);
      fprintf(Outfile, "\tmovslq\t%s(%%rip), %s\n", n->name, reglist[r]);
      if (op == A_POSTINC)
        fprintf(Outfile, "\tincl\t%s(%%rip)\n", n->name);
      if (op == A_POSTDEC)
        fprintf(Outfile, "\tdecl\t%s(%%rip)\n", n->name);
      break;
    case 8:
      if (op == A_PREINC)
        fprintf(Outfile, "\tincq\t%s(%%rip)\n", n->name);
      if (op == A_PREDEC)
        fprintf(Outfile, "\tdecq\t%s(%%rip)\n", n->name);
      fprintf(Outfile, "\tmovq\t%s(%%rip), %s\n", n->name, reglist[r]);
      if (op == A_POSTINC)
        fprintf(Outfile, "\tincq\t%s(%%rip)\n", n->name);
      if (op == A_POSTDEC)
        fprintf(Outfile, "\tdecq\t%s(%%rip)\n", n->name);
      break;
    default:
      fatald("Bad type in cgloadglob:", n->type);
  }
  return (r);
}

int cgloadlocal(struct symtable *n, int op) {
  // Get a new register
  int r = alloc_register();

  int size = cgprimsize(n->type);
  // Print out the code to initialise it
  switch(size){
    case 1:
      if (op == A_PREINC)
        fprintf(Outfile, "\tincb\t%d(%%rbp)\n", n->posn);
      if (op == A_PREDEC)
        fprintf(Outfile, "\tdecb\t%d(%%rbp)\n", n->posn);
      fprintf(Outfile, "\tmovzbq\t%d(%%rbp), %s\n", n->posn, reglist[r]);
      if (op == A_POSTINC)
        fprintf(Outfile, "\tincb\t%d(%%rbp)\n", n->posn);
      if (op == A_POSTDEC)
        fprintf(Outfile, "\tdecb\t%d(%%rbp)\n", n->posn);
      break;
    case 4:
      if (op == A_PREINC)
        fprintf(Outfile, "\tincl\t%d(%%rbp)\n", n->posn);
      if (op == A_PREDEC)
        fprintf(Outfile, "\tdecl\t%d(%%rbp)\n", n->posn);
      fprintf(Outfile, "\tmovslq\t%d(%%rbp), %s\n", n->posn, reglist[r]);
      if (op == A_POSTINC)
        fprintf(Outfile, "\tincl\t%d(%%rbp)\n", n->posn);
      if (op == A_POSTDEC)
        fprintf(Outfile, "\tdecl\t%d(%%rbp)\n", n->posn);
      break;
    case 8:
      if (op == A_PREINC)
        fprintf(Outfile, "\tincq\t%d(%%rbp)\n", n->posn);
      if (op == A_PREDEC)
        fprintf(Outfile, "\tdecq\t%d(%%rbp)\n", n->posn);
      fprintf(Outfile, "\tmovq\t%d(%%rbp), %s\n", n->posn, reglist[r]);
      if (op == A_POSTINC)
        fprintf(Outfile, "\tincq\t%d(%%rbp)\n", n->posn);
      if (op == A_POSTDEC)
        fprintf(Outfile, "\tdecq\t%d(%%rbp)\n", n->posn);
      break;
    default:
      fatald("Bad type in cgloadlocal:", n->type);
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


// Given a scalar type, an existing memory offset
// (which hasn't been allocated to anything yet)
// and a direction ( 1 is up, -1 is down), calculate
// and return a suitably aligned memory offset
// for this scalar type. This could be the original offset
// or it could be above/below the original
int cgalign(int type, int offset, int direction) {
  int alignment;

  if (!ptrtype(type)) {
    switch (type) {
    case P_CHAR:
      return (offset);
    case P_INT:
    case P_LONG:
      break;
    default:
      fatald("Bad type in calc_aligned_offset", type);
    }
  }

  alignment = 4;
  offset = (offset + direction * (alignment - 1)) & ~(alignment-1);
  return (offset);
}

void cgswitch(int reg, int casecount, int toplabel,
              int *caselabel, int *caseval, int defaultlabel) {
  int i, label;

  // Get a label for the switch table
  label = genlabel();
  cglabel(label);

  if (casecount == 0) {
    caseval[0] = 0;
    caselabel[0]  = defaultlabel;
    casecount = 1;
  }

  //Generate the switch jump table
  fprintf(Outfile,
          "\t.quad\t%d\n", casecount);
  for (i = 0; i < casecount; i++)
    fprintf(Outfile, "\t.quad\t%d, L%d\n", caseval[i], caselabel[i]);
  fprintf(Outfile, "\t.quad\tL%d\n", defaultlabel);

  // Load the specific registers
  cglabel(toplabel);
  fprintf(Outfile, "\tmovq\t%s, %%rax\n", reglist[reg]);
  fprintf(Outfile, "\tleaq\tL%d(%%rip), %%rdx\n", label);
  fprintf(Outfile, "\tjmp\tswitch\n");
}
