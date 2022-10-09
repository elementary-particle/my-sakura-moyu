#ifndef _VM_SUB_H
#define _VM_SUB_H

#include "VmAddrMap.h"
#include "VmInst.h"
#include "VmBlock.h"

class VmEnv;

class VmSub {
public:
  VmInst *entryInst;
  uint32_t entryCp;
  VmBlock *firstBlock;
  VmAddrMap<VmInst> instMap;
  VmBlockList blockQueue;
  VmBlockTree blockTree;
  VmEnv *env;

  VmSub(): instMap(10) {
    entryInst = nullptr;
    firstBlock = nullptr;
    STAILQ_INIT(&blockQueue);
    SPLAY_INIT(&blockTree);
    env = nullptr;
  }

  int getArgCount() {
    return (int)entryInst->args.argFrameInfo.frameArgSize;
  }

  VmInst *getInst(VmInst *prevInst);
  VmBlock *peekBlock(uint32_t blockCp, int stackPtr);
  void getBlocks();
  VmBlock *getSpan(VmBlock *enterBlock, VmBlock *leaveBlock);

  void printAsm(FILE *file) {
    VmBlock *block;

    entryInst->printAsm(file);
    SPLAY_FOREACH(block, VmBlockTree, &blockTree) {
      fprintf(file, "%08x:\n", block->enterCp);
      block->printAsm(file);
    }
  }

  void printSpan(FILE *file, VmBlock *whileBlock, VmBlock *enterBlock, VmBlock *leaveBlock, int indentLevel);

  void printFlow(FILE *file) {
    VmBlock *floatBlock;

    for (floatBlock = firstBlock;
         floatBlock;
         floatBlock = floatBlock->aftrBlock) {
      printSpan(file, nullptr, floatBlock, floatBlock->aftrBlock, 1);
    }
  }

  ~VmSub() {
    VmBlock *p, *q;
    STAILQ_FOREACH_SAFE(p, &blockQueue, queueLink, q) {
      delete p;
    }
    delete entryInst;
  }
};

#endif // _VM_SUB_H
