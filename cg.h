// cg.c
void freeall_registers(void);
void cgpreamble();
void cgpostamble();
void cgfuncpreamble(struct symtables *n);
void cgfuncpostamble(struct symtables *n);

void cgglobsym(struct symtables *n);
int cgloadglob(struct symtables *n, int op);
int cgstorglob(int r, struct symtables *n);
int cgloadlocal(struct symtables *n, int op);
int cgstorlocal(int r, struct symtables *n);

int cgloadint(int value);
int cgadd(int r1, int r2);
int cgsub(int r1, int r2);
int cgmul(int r1, int r2);
int cgdiv(int r1, int r2);
int cgequal(int r1, int r2);
int cgnotequal(int r1, int r2);
int cglessthan(int r1, int r2);
int cggreaterthan(int r1, int r2);
int cglessequal(int r1, int r2);
int cggreaterequal(int r1, int r2);
int cgwiden(int r, int oldtype, int newtype);

void cglabel(int l);
void cgjump(int l);
int cgcompare_and_set(int ASTop, int r1, int r2);
int cgcompare_and_jump(int ASTop, int r1, int r2, int label);

int cgprimsize(int type);
void cgprintint(int r);
void cgreturn(int r,struct symtables *functionid);
int cgcall(struct symtables *n, int numargs);
int cgaddress(struct symtables *n);
int cgderef(int r, int type);
int cgshlconst(int r, int val);
int cgstorderef(int r1, int r2, int type);

void cgglobstr(int label,char *strvalue);
int cgloadglobstr(int label);

int cgand(int r1, int r2);
int cgor(int r1, int r2);
int cgxor(int r1, int r2);
int cgnegate(int r);
int cginvert(int r);
int cgshl(int r1, int r2);
int cgshr(int r1, int r2);
int cglognot(int r);
int cgboolean(int r, int op, int label);

void cgcopyarg(int r, int argposn);
