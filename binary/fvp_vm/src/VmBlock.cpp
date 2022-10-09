#include "VmBlock.h"

VmBlock::VmBlock() {
  enterCp = leaveCp = 0;
  firstInst = lastInst = nullptr;
  stackPtr = 0;

  spanType = STMT;
  abnrmSrcNum = 0;
  nextBlock = aftrBlock = nullptr;
}

VmExpr *VmBlock::printCode(FILE *file, int indentLevel) {
  VmInst *p;
  VmExpr *stack[0x200], **sp, *ret;

  int i;
  for (i = 0, sp = stack; i < stackPtr; i++, sp++) {
    *sp = new VmExpr(i);
  }

  for (p = firstInst; p; p = p->next) {
    /* build expr tree. */
    VmExpr *expr;

    if (p->head == VmInst::J    || p->head == VmInst::JC   ||
        p->head == VmInst::JR   || p->head == VmInst::JRT ) {
      break;
    } else {
      expr = new VmExpr(p, sp, ret);
      if (expr->type == VmExpr::NOOP) {
        delete expr;
      } else if (expr->type == VmExpr::STMT) {
        printIndent(file, indentLevel);
        expr->printCode(file);
      } else if (expr->type == VmExpr::MULTI && (!p->next || p->next->head != VmInst::LT  )) {
        printIndent(file, indentLevel);
        expr->printCode(file);
        fprintf(file, ";\n");
        delete expr;
      }
    }
  }

  if (!p) {
    ret = nullptr;
  } else if (p->head == VmInst::JC   || p->head == VmInst::JRT ) {
    sp--;
    ret = *sp;
  } else {
    ret = nullptr;
  }

  for (i = 0; i < sp - stack; i++) {
    if ((*sp)->type != VmExpr::STACK) {
      printIndent(file, indentLevel);
      fprintf(file, "S%d := ", i);
      (*sp)->printCode(file);
      fprintf(file, ";\n");
    }
    delete stack[i];
  }
  return ret;
}

VmBlock::~VmBlock() {
}

SPLAY_GENERATE(VmBlockTree, VmBlock, treeLink, blockCmp);
