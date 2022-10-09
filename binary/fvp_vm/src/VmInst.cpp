/*
 * FVP Vm Instructions
 * 2022/05
 */

#include "VmInst.h"

static const char *instName[0x28] = {
  "nop" , "entr", "jal" , "func", "jr"  , "jrt" , "j"   , "jz"  ,
  "lc0" , "lc1" , "ll"  , "lh"  , "lb"  , "lflt", "lx"  , "lr"  ,
  "lf"  , "lra" , "lfa" , "dup" , "lt"  , "sr"  , "sf"  , "sra" ,
  "sfa" , "neg" , "add" , "sub" , "mul" , "div" , "mod" , "bsel",
  "land", "lor" , "seq" , "sne" , "sgt" , "sge" , "slt" , "sle" ,
};

static const int instPrior[0x28] = {
  0,  0, -1, -1,  0,  0,  0,  0,
 -1, -1, -1, -1, -1, -1, -1, -1,
 -1, -1, -1,  0,  0,  0,  0, -1,
 -1,  2,  2,  2,  1,  1,  1,  3,
  5,  6,  4,  4,  4,  4,  4,  4,
};

static const char *instSymbol[0x28] = {
  "<>", "<>", "<>", "<>", "<>", "<>", "<>", "<>",
  "0b", "1b", ":=", ":=", ":=", ":=", ":=", ":=",
  ":=", "[]", "[]", ":=", ":=", ":=", ":=", "[]",
  "[]", "-" , "+" , "-" , "*" , "/" , "%" , "#" ,
  "&&", "||", "==", "!=", ">" , ">=", "<" , "<=",
};
/* JAL and FUNC insts are special. */
static const int instStackIn[0x28] = {
  0,  0,  0,  0,  0,  1,  0,  1,
  0,  0,  0,  0,  0,  0,  0,  0,
  0,  1,  1,  1,  0,  1,  1,  2,
  2,  1,  2,  2,  2,  2,  2,  2,
  2,  2,  2,  2,  2,  2,  2,  2,
};

static const int instStackOut[0x28] = {
  0,  0,  0,  0,  0,  0,  0,  0,
  1,  1,  1,  1,  1,  1,  1,  1,
  1,  1,  1,  2,  1,  0,  0,  0,
  0,  1,  1,  1,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,
};

/* byte order: native */
uint8_t *VmInst::fromByteCode(uint8_t *bufCp) {
  head = (Head)*bufCp;
  bufCp++;
  name = instName[head];
  switch (head) {
    case NOP : // 0
    case JR  : case JRT :
    case LC0 : case LC1 :
    case DUP : case LT  :
    case NEG : case ADD : case SUB :
    case MUL : case DIV : case MOD :
    case BSEL: case LAND: case LOR :
    case SEQ : case SNE : case SGT :
    case SGE : case SLT : case SLE :
      break;
    case ENTR: // 16 frameInfo
      args.argFrameInfo.frameArgSize = vmUInt8(bufCp);
      args.argFrameInfo.frameValSize = vmUInt8(bufCp);
      break;
    case LB  : // 8 int/frameAddr
    case LF  : case LFA :
    case SF  : case SFA :
      args.argInt = vmInt8(bufCp);
      break;
    case LH  : // 16 int
      args.argInt = vmInt16(bufCp);
      break;
    case FUNC: // 16 addr
    case LR  : case LRA :
    case SR  : case SRA :
      args.argUnsigned = vmUInt16(bufCp);
      break;
    case LL  : // 32 int
      args.argInt = vmInt32(bufCp);
      break;
    case JAL : // 32 addr
    case J   : case JC  :
      args.argUnsigned = vmUInt32(bufCp);
      break;
    case LFLT:
      args.argFloat = vmFloat(bufCp);
      break;
    case LX  : // text
      vmText(args.argText, bufCp);
      break;
    default:
      /* invalid opcode */
      break;
  }
  stackIn = instStackIn[head];
  stackOut = instStackOut[head];
  stackOff = stackOut - stackIn;
  return bufCp;
}

void VmInst::printAsm(FILE *file) {
  fprintf(file, "%-8s", name);
  switch (head) {
    case NOP : // 0
    case JR  : case JRT :
    case LC0 : case LC1 :
    case DUP : case LT  :
    case NEG : case ADD : case SUB :
    case MUL : case DIV : case MOD :
    case BSEL: case LAND: case LOR :
    case SEQ : case SNE : case SGT :
    case SGE : case SLT : case SLE :
      /* INT   == C1 */
      /* FLOAT == C1 */
      /* TEXT  == C1 */
      /* TADR  == C1 */
      /* ARR   == C1 */
      break;
    case ENTR: // 16 frameInfo
      fprintf(file, "%-4d%-4d", args.argFrameInfo.frameArgSize, args.argFrameInfo.frameValSize);
      break;
    case LB  : // 8 int/frameAddr
    case LF  : case LFA :
    case SF  : case SFA :
    case LH  : // 16 int
    case LL  : // 32 int
      fprintf(file, "%-14d", args.argInt);
      break;
    case LR  : case LRA :
    case SR  : case SRA :
      fprintf(file, "%04x", args.argUnsigned);
      break;
    case JAL : // 32 addr
    case J   : case JC  : // dst block
      fprintf(file, "%08x", args.argUnsigned);
      break;
    case FUNC: // 16 addr
      fprintf(file, "%04x ; %d, %s", args.argUnsigned, -stackOff, comment);
      break;
    case LFLT: // 32 float
      fprintf(file, "%-8.3f ; %s", args.argFloat, args.argText.buf);
      break;
    case LX  : // text
      fprintf(file, "%-3d ; %s", args.argText.len, args.argText.buf);
      break;
    default:
      /* invalid opcode */
      break;
  }
  fprintf(file, "\n");
}

VmInst::~VmInst() {
  if (head == LX  ) {
    delete[] args.argText.buf;
  }
}

VmExpr::VmExpr(VmInst *initInst, VmExpr **&sp, VmExpr *&ret) {
  int i;
  inst = initInst;
  sp -= inst->stackIn;
  if (inst->head == VmInst::JAL || inst->head == VmInst::FUNC) {
    type = MULTI;
    u.arrOperands = new VmExpr *[inst->stackIn];
    for (i = 0; i < inst->stackIn; i++) {
      u.arrOperands[i] = sp[i];
    };
    ret = this;
  } else if(inst->head == VmInst::LT  ) {
    type = NOOP;
    sp[0] = ret;
    ret = nullptr;
  } else if(inst->head == VmInst::DUP ) {
    /* should count reference here */
    type = NOOP;
    sp[1] = sp[0];
  } else {
    switch (inst->head) {
    case VmInst::SRA : case VmInst::SFA :
    case VmInst::SR  : case VmInst::SF  :
      type = STMT;
      break;
    default:
      type = EXPR;
      prior = instPrior[inst->head];
      break;
    }
    if (inst->stackIn == 2) {
      u.binOperands.arg1 = sp[1];
      u.binOperands.arg0 = sp[0];
    } else if(inst->stackIn == 1) {
      u.binOperands.arg0 = sp[0];
      u.binOperands.arg1 = nullptr;
    } else {
      u.binOperands.arg0 = nullptr;
      u.binOperands.arg1 = nullptr;
    }
    if (inst->stackOut) {
      sp[0] = this;
    }
  }
  sp += inst->stackOut;
}

static inline void printWrap(FILE *file, VmExpr *self, VmExpr *expr) {
  if (self->prior <= expr->prior) {
    fprintf(file, "(");
    expr->printCode(file);
    fprintf(file, ")");
  } else {
    expr->printCode(file);
  }
}

void VmExpr::printVar(FILE *file) {
  bool isArrayIndex;
  isArrayIndex = false;
  switch (inst->head) {
    case VmInst::LRA : case VmInst::SRA :
      isArrayIndex = true;
    case VmInst::LR  : case VmInst::SR  :
      fprintf(file, "G%u", inst->args.argUnsigned);
      if (isArrayIndex) {
        goto tag_printArrayIndex;
      }
      break;
    case VmInst::LFA : case VmInst::SFA :
      isArrayIndex = true;
    case VmInst::LF  : case VmInst::SF  :
      if (inst->args.argInt < 0) {
        fprintf(file, "A%d", -inst->args.argInt - 2);
      } else {
        fprintf(file, "V%d", inst->args.argInt);
      }
      if (isArrayIndex) {
        goto tag_printArrayIndex;
      }
      break;
    tag_printArrayIndex:
      fprintf(file, "[");
      u.binOperands.arg0->printCode(file);
      fprintf(file, "]");
      break;
    default:
      break;
  }
}

void VmExpr::printCode(FILE *file) {
  int saveArg;
  saveArg = 0;
  switch (type) {
  case STACK:
    fprintf(file, "S%d", u.off);
    break;
  case MULTI:
    if (inst->head == VmInst::JAL ) {
      fprintf(file, "sub_%08x", inst->args.argUnsigned);
    } else {
      fprintf(file, "%s", inst->comment);
    }
    fprintf(file, "(");
    if (inst->stackIn > 0) {
      int i;
      u.arrOperands[0]->printCode(file);
      for (i = 1; i < inst->stackIn; i++) {
        fprintf(file, ", ");
        u.arrOperands[i]->printCode(file);
      }
    }
    fprintf(file, ")");
    break;
  case EXPR:
  case STMT:
    switch (inst->head) {
    case VmInst::LC0 : case VmInst::LC1 :
      fprintf(file, instSymbol[inst->head]);
      break;
    case VmInst::LL  : case VmInst::LH  : case VmInst::LB  :
      fprintf(file, "%d", inst->args.argInt);
      break;
    case VmInst::LFLT:
      fprintf(file, "%f", inst->args.argFloat);
      break;
    case VmInst::LX:
      fprintf(file, "\"%s\"", inst->args.argText.buf);
      break;
    case VmInst::SRA : case VmInst::SFA :
      saveArg = 2;
      goto tag_regSaveLoad;
    case VmInst::SR  : case VmInst::SF  :
      saveArg = 1;
    case VmInst::LRA : case VmInst::LR  :
    case VmInst::LFA : case VmInst::LF  :
tag_regSaveLoad:
      printVar(file);
      if (saveArg) {
        fprintf(file, " := ");
        if (saveArg == 1) {
          u.binOperands.arg0->printCode(file);
        } else {
          u.binOperands.arg1->printCode(file);
        }
        fprintf(file, ";\n");
      }
      break;
    case VmInst::NEG :
      fprintf(file, instSymbol[inst->head]);
      printWrap(file, this, u.binOperands.arg0);
      break;
    case VmInst::ADD : case VmInst::SUB :
    case VmInst::MUL : case VmInst::DIV : case VmInst::MOD :
    case VmInst::BSEL: case VmInst::LAND: case VmInst::LOR :
    case VmInst::SEQ : case VmInst::SNE :
    case VmInst::SGT : case VmInst::SGE :
    case VmInst::SLT : case VmInst::SLE :
      printWrap(file, this, u.binOperands.arg0);
      fprintf(file, " ");
      fputs(instSymbol[inst->head], file);
      fprintf(file, " ");
      printWrap(file, this, u.binOperands.arg1);
      break;
    default:
      break;
    }
    break;
  default:
    break;
  }
}

VmExpr::VmExpr(int stackPtr) {
  type = STACK;
  inst = nullptr;
  u.off = stackPtr;
}

VmExpr::~VmExpr() {
  if (type == MULTI) {
    int i;
    for (i = 0; i < inst->stackIn; i++) {
      delete u.arrOperands[i];
    }
    delete[] u.arrOperands;
  } else if (type == EXPR || type == STMT) {
    if (u.binOperands.arg0) {
      delete u.binOperands.arg0;
    }
    if (u.binOperands.arg1) {
      delete u.binOperands.arg1;
    }
  }
}
