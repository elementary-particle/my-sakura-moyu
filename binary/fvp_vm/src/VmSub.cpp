#include "VmSub.h"

#include <cstdio>

#include "VmEnv.h"

VmInst *VmSub::getInst(VmInst *prevInst)  {
  VmEntry<VmInst> *inst;

  inst = new VmEntry<VmInst>(env->getCp());
  env->getInst(inst, prevInst);
  instMap.add(inst);
  return inst;
}

VmBlock *VmSub::peekBlock(uint32_t blockCp, int stackPtr) {
  VmBlock *block, *findBlock;

  block = new VmBlock;
  block->enterCp = blockCp;
  findBlock = SPLAY_INSERT(VmBlockTree, &blockTree, block);

  if (findBlock) {
    delete block;
    block = findBlock;
    if (stackPtr != block->stackPtr) {
      fprintf(stderr, "Block %08x: inconsistent sp: %d, %d.\n", blockCp, stackPtr, block->stackPtr);
    }
  } else {
    block->enterCp = blockCp;
    block->stackPtr = stackPtr;
    STAILQ_INSERT_TAIL(&blockQueue, block, queueLink);
  }
  return block;
}

void VmSub::getBlocks() {
  VmBlock *block, *nextBlock;

  firstBlock = peekBlock(entryCp, 0);
  STAILQ_FOREACH(block, &blockQueue, queueLink) {
    VmBlock *prevBlock;
    VmInst *inst, *prevInst;
    int stackPtr;

    prevBlock = SPLAY_PREV(VmBlockTree, &blockTree, block);
    if (prevBlock && prevBlock->leaveCp > block->enterCp) {
      block->firstInst = instMap.get(block->enterCp);

      block->leaveCp = prevBlock->leaveCp;
      block->lastInst = prevBlock->lastInst;
      block->dstBlock0 = prevBlock->dstBlock0;
      block->dstBlock1 = prevBlock->dstBlock1;

      prevBlock->leaveCp = block->enterCp;
      prevBlock->lastInst = nullptr;
      prevBlock->dstBlock0 = block;
      prevBlock->dstBlock1 = nullptr;
      continue;
    }
    nextBlock = SPLAY_NEXT(VmBlockTree, &blockTree, block);

    prevInst = nullptr;
    env->setCp(block->enterCp);
    stackPtr = block->stackPtr;

    inst = getInst(nullptr);
    block->firstInst = inst;

    for ( ; ; ) {
      uint32_t dstCp;

      inst->leaveCp = env->getCp();
      stackPtr += inst->stackOff;
      switch (inst->head) {
      case VmInst::J   :
        dstCp = inst->args.argUnsigned;
        block->dstBlock0 = peekBlock(dstCp, stackPtr);
        block->dstBlock1 = nullptr;
        goto tag_blockEnd;
      case VmInst::JC  :
        dstCp = inst->args.argUnsigned;
        block->dstBlock0 = peekBlock(inst->leaveCp, stackPtr);
        block->dstBlock1 = peekBlock(dstCp, stackPtr);
        goto tag_blockEnd;
      case VmInst::JR  : case VmInst::JRT :
        block->dstBlock0 = nullptr;
        block->dstBlock1 = nullptr;
        if (stackPtr != 0) {
          fprintf(stderr, "Sub %08x: invalid sp on return: %d.\n", entryCp - 3, stackPtr);
        }
        goto tag_blockEnd;
      default:
        break;
      }
      if (nextBlock && inst->leaveCp == nextBlock->enterCp) {
        break;
      }
      prevInst = inst;
      inst = getInst(prevInst);
      prevInst->next = inst;
    }

    block->dstBlock0 = nextBlock;
    block->dstBlock1 = nullptr;
    block->lastInst = nullptr;
    block->leaveCp = inst->leaveCp;
    continue;

tag_blockEnd:
    block->lastInst = inst;
    block->leaveCp = inst->leaveCp;
  }

  for (block = SPLAY_MIN(VmBlockTree, &blockTree); block; block = nextBlock) {
    nextBlock = SPLAY_NEXT(VmBlockTree, &blockTree, block);
    block->nextBlock = nextBlock;
    block->getExtent();
  }
}

VmBlock *VmSub::getSpan(VmBlock *enterBlock, VmBlock *leaveBlock) {
  VmBlock *curBlock;

  for (curBlock = enterBlock;
       curBlock && (!leaveBlock || blockCmp(curBlock, leaveBlock) < 0); ) {
    if (!curBlock->lastInst) {
      curBlock->spanType = VmBlock::STMT;
      curBlock = curBlock->dstBlock0;
    } else if (curBlock->lastInst->head == VmInst::JC  ) {
      VmBlock *lastBlock;

      lastBlock = getSpan(curBlock->dstBlock0, curBlock->dstBlock1);
      if (lastBlock == curBlock->dstBlock1) {
        curBlock->spanType = VmBlock::IFTHEN;
        curBlock->nextBlock = curBlock->dstBlock1;
        curBlock = curBlock->nextBlock;
      } else if (lastBlock == curBlock) {
        curBlock->spanType = VmBlock::WHILE;
        curBlock->nextBlock = curBlock->dstBlock1;
        curBlock = curBlock->nextBlock;
      } else {
        VmBlock *nextBlock;

        nextBlock = getSpan(curBlock->dstBlock1, lastBlock);
        if (nextBlock == lastBlock && lastBlock == curBlock->dstBlock1->aftrBlock) {
          curBlock->spanType = VmBlock::IFELSE;
          curBlock->nextBlock = lastBlock;
          curBlock = curBlock->nextBlock;
        } else {
          curBlock->spanType = VmBlock::IFTHEN;
          if (lastBlock && nextBlock != lastBlock) {
            lastBlock->abnrmSrcNum++;
          }
          curBlock->nextBlock = curBlock->dstBlock1;
          enterBlock->aftrBlock = curBlock->dstBlock1->aftrBlock;
          return nextBlock;
        }
      }
    } else if (curBlock->lastInst->head == VmInst::J   ) {
      curBlock->spanType = VmBlock::STMT;
      enterBlock->aftrBlock = curBlock->nextBlock;
      return curBlock->dstBlock0;
    } else if (curBlock->lastInst->head == VmInst::JR   ||
               curBlock->lastInst->head == VmInst::JRT ) {
      enterBlock->aftrBlock = curBlock->nextBlock;
      return nullptr;
    }
  }
  enterBlock->aftrBlock = curBlock;
  return curBlock;
}

void VmSub::printSpan(FILE *file, VmBlock *whileBlock, VmBlock *enterBlock, VmBlock *leaveBlock, int indentLevel) {
  VmBlock *curBlock;

  for (curBlock = enterBlock;
       curBlock && (!leaveBlock || blockCmp(curBlock, leaveBlock) < 0);
       curBlock = curBlock->nextBlock) {
    VmExpr *expr;

    if (curBlock->abnrmSrcNum) {
      fprintf(file, "LABEL_%08x:\n", curBlock->enterCp);
    }

    expr = curBlock->printCode(file, indentLevel);

    if (curBlock->spanType == VmBlock::WHILE) {
      printIndent(file, indentLevel);
      fprintf(file, "WHILE(");
      expr->printCode(file);
      fprintf(file, "):\n");
      delete expr;

      printSpan(file, curBlock, curBlock->dstBlock0, curBlock->nextBlock, indentLevel + 1);

      if (curBlock->dstBlock0->aftrBlock != curBlock->dstBlock1) {
        fprintf(file, "[FLOAT]\n");
        printSpan(file, nullptr, curBlock->dstBlock0->aftrBlock, curBlock->dstBlock1, 1);
      }
    } else if (curBlock->spanType == VmBlock::IFTHEN) {
      printIndent(file, indentLevel);
      fprintf(file, "IF(");
      expr->printCode(file);
      fprintf(file, "):\n");
      delete expr;

      printSpan(file, whileBlock, curBlock->dstBlock0, curBlock->nextBlock, indentLevel + 1);

      if (curBlock->dstBlock0->aftrBlock != curBlock->dstBlock1) {
        fprintf(file, "[FLOAT]\n");
        printSpan(file, nullptr, curBlock->dstBlock0->aftrBlock, curBlock->dstBlock1, 1);
      }
    } else if (curBlock->spanType == VmBlock::IFELSE) {
      printIndent(file, indentLevel);
      fprintf(file, "IF(");
      expr->printCode(file);
      fprintf(file, "):\n");
      delete expr;

      printSpan(file, whileBlock, curBlock->dstBlock0, curBlock->nextBlock, indentLevel + 1);

      printIndent(file, indentLevel);
      fprintf(file, "ELSE:\n");

      printSpan(file, whileBlock, curBlock->dstBlock1, curBlock->nextBlock, indentLevel + 1);

      if (curBlock->dstBlock0->aftrBlock != curBlock->dstBlock1) {
        fprintf(file, "[FLOAT]\n");
        printSpan(file, nullptr, curBlock->dstBlock0->aftrBlock, curBlock->dstBlock1, 1);
      }
    }

    if (curBlock->lastInst && curBlock->lastInst->head == VmInst::J   ) {
      if (whileBlock && curBlock->dstBlock0 == whileBlock->nextBlock) {
        printIndent(file, indentLevel);
        fprintf(file, "BREAK;\n");
      } else if (whileBlock && curBlock->dstBlock0 == whileBlock) {
        printIndent(file, indentLevel);
        fprintf(file, "CONTINUE;\n");
      } else if (curBlock->dstBlock0 != leaveBlock){
        printIndent(file, indentLevel);
        fprintf(file, "GOTO LABEL_%08x;\n", curBlock->dstBlock0->enterCp);
      }
      break;
    }

    if (curBlock->lastInst && curBlock->lastInst->head == VmInst::JR  ) {
      printIndent(file, indentLevel);
      fprintf(file, "RETURN;\n");
      break;
    }

    if (curBlock->lastInst && curBlock->lastInst->head == VmInst::JRT ) {
      printIndent(file, indentLevel);
      fprintf(file, "RETURN (");
      expr->printCode(file);
      fprintf(file, ");\n");
      break;
    }
  }
}

int subBefore(const void *sub0, const void *sub1) {
  return (int)((*(const VmEntry<VmSub> **)sub0)->entryCp) - (int)((*(const VmEntry<VmSub> **)sub1)->entryCp);
}
