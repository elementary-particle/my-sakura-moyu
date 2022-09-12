#ifndef _VM_INST_H
#define _VM_INST_H

#include "VmBin.h"

#include <cstdio>

class VmBlock;
class VmInst {
public:
  enum Head {
    NOP  = 0x00, ENTR = 0x01, JAL  = 0x02, FUNC = 0x03,
    JR   = 0x04, JRT  = 0x05, J    = 0x06, JC   = 0x07,
    LC0  = 0x08, LC1  = 0x09,
    LL   = 0x0a, LH   = 0x0b, LB   = 0x0c,
    LFLT = 0x0d, LX   = 0x0e,
    LR   = 0x0f, LF   = 0x10, LRA  = 0x11, LFA  = 0x12,
    DUP  = 0x13, LT   = 0x14,
    SR   = 0x15, SF   = 0x16, SRA  = 0x17, SFA  = 0x18,
    NEG  = 0x19, ADD  = 0x1a, SUB  = 0x1b,
    MUL  = 0x1c, DIV  = 0x1d, MOD  = 0x1e,
    BSEL = 0x1f, LAND = 0x20, LOR  = 0x21,

    SEQ  = 0x22, SNE  = 0x23, SGT  = 0x24,
    SGE  = 0x25, SLT  = 0x26, SLE  = 0x27,
  };
  Head head;
  const char *name;
  char *comment;
  union {
    int32_t argInt;
    uint32_t argUnsigned;
    float argFloat;
    struct VmFrameInfo {
      uint8_t frameArgSize;
      uint8_t frameValSize;
    } argFrameInfo;
    VmText argText;
  } args;
  int stackIn, stackOut, stackOff;
  uint32_t leaveCp;
  VmInst *next;

  VmInst() {
    next = nullptr;
  }
  uint8_t *fromByteCode(uint8_t *cp);
  void printAsm(FILE *file);
  ~VmInst();
};

class VmExpr {
public:
  enum ExprType {
    STACK, MULTI, EXPR, STMT, NOOP
  };
  ExprType type;
  VmInst *inst;
  int prior;
  union {
    int off;
    VmExpr **arrOperands;
    struct {
      VmExpr *arg0, *arg1;
    } binOperands;
  } u;

  VmExpr(VmInst *initInst, VmExpr **&sp, VmExpr *&ret);
  VmExpr(int stackPtr);
  void printVar(FILE *file);
  void printCode(FILE *file);
  ~VmExpr();
};

#endif // _VM_INST_H
