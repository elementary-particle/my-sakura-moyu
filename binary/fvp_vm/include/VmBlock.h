#ifndef _VM_BLOCK_H
#define _VM_BLOCK_H

#include "sys/cdefs.h"
#include "sys/queue.h"
#include "sys/tree.h"

#include "VmBitSet.h"
#include "VmInst.h"

class VmBlock {
public:
  enum SpanType {
    STMT,
    WHILE,
    IFTHEN,
    IFELSE,
  };

  uint32_t enterCp, leaveCp;
  VmInst *firstInst, *lastInst;
  int stackPtr;
  VmBlock *dstBlock0, *dstBlock1;

  SPLAY_ENTRY(VmBlock) treeLink;
  STAILQ_ENTRY(VmBlock) queueLink;

  SpanType spanType;
  int abnrmSrcNum;
  VmBlock *nextBlock, *aftrBlock;

  VmBlock();

  void getExtent() {
    VmInst *inst;
    for (inst = firstInst;
         inst && inst->leaveCp < leaveCp;
         inst = inst->next);
    inst->next = nullptr;
  }

  void printAsm(FILE *file) {
    for (VmInst *inst = firstInst; inst; inst = inst->next) {
      inst->printAsm(file);
    }
  }
  VmExpr *printCode(FILE *file, int indentLevel);
  ~VmBlock();
};

static inline void printIndent(FILE *file, int indentLevel) {
  int i;
  for (i = 0; i < indentLevel; i++) {
    fputc(' ', file); fputc(' ', file);
  }
}

static inline int blockCmp(VmBlock *block0, VmBlock *block1) {
  return (int)block0->enterCp - (int)block1->enterCp;
}

STAILQ_HEAD(VmBlockList, VmBlock);
SPLAY_HEAD(VmBlockTree, VmBlock);
SPLAY_PROTOTYPE(VmBlockTree, VmBlock, treeLink, blockCmp);

#endif // _VM_BLOCK_H
