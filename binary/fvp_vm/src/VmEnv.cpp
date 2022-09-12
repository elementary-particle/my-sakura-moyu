#include "VmEnv.h"

#include <cstring>

VmSub *VmEnv::peekSub(uint32_t subCp) {
  VmEntry<VmSub> *sub;

  sub = subMap.get(subCp);
  if (!sub) {
    uint32_t oldCp;

    oldCp = getCp();

    sub = new VmEntry<VmSub>(subCp);
    setCp(subCp);
    /* ',' is a C++ break point */
    sub->entryInst = new VmInst;
    getInst(sub->entryInst, nullptr);
    sub->entryCp = getCp();
    sub->env = this;

    setCp(oldCp);
    subMap.add(sub);
  }
  return sub;
}

VmEnv::VmEnv(uint8_t *initBuf): subMap(12) {
  uint8_t *head;
  unsigned i;

  buf = initBuf;
  head = buf + vmUInt32(buf);
  mainSubCp = vmUInt32(head);
  regSize1 = vmUInt16(head);
  regSize2 = vmUInt16(head);
  /* unknown 2 bytes */
  head += 2;
  vmText(title, head);
  funcsSize = vmUInt16(head);
  funcs = new VmFunc[funcsSize];
  funcThreadStart = -1;
  for (i = 0; i < funcsSize; i++) {
    funcs[i].argSize = vmUInt8(head);
    vmText(funcs[i].name, head);
    if (!strcmp((const char *)funcs[i].name.buf, "ThreadStart")) {
      funcThreadStart = i;
    }
  }
  /*
  unsigned importsSize;
  importsSize = vmUInt16(head);
  for (i = 0; i < importsSize; i++) {
    vmUInt32(head); // funcAddr
    vmUInt8(head); // argSize
    vmText(, head); // funcName
  }
  */
  buf = initBuf;
}

void VmEnv::getInst(VmInst *inst, VmInst *prevInst) {
  uint32_t startCp;

  startCp = getCp();
  bufCp = inst->fromByteCode(bufCp);
  if (inst->head == VmInst::JAL ) {
    uint32_t subCp;
    int argCount;
    subCp = inst->args.argUnsigned;
    argCount = peekSub(subCp)->getArgCount();
    inst->stackIn = argCount;
    inst->stackOut = 0;
    inst->stackOff = -argCount;
  } else if (inst->head == VmInst::FUNC) {
    unsigned funcId;
    int argCount;
    funcId = inst->args.argUnsigned;
    argCount = funcs[funcId].argSize;
    inst->stackIn = argCount;
    inst->stackOut = 0;
    inst->stackOff = -argCount;
    inst->comment = (char *)funcs[funcId].name.buf;
    if (inst->head == VmInst::FUNC && inst->args.argUnsigned == funcThreadStart) {
      if (prevInst && prevInst->head == VmInst::LL  ) {
        peekSub(prevInst->args.argUnsigned);
      } else {
        fprintf(stderr, "Func %08x: invoking thread with unknown address.\n", startCp);
      }
    }
  }
}

void VmEnv::getSubs() {
  VmEntry<VmSub> *sub;

  peekSub(mainSubCp);
  STAILQ_FOREACH(sub, &subMap.itemList, listLink) {
    VmBlock *floatBlock;

    sub->getBlocks();
    for (floatBlock = sub->firstBlock;
         floatBlock;
         floatBlock = floatBlock->aftrBlock) {
      VmBlock *nextBlock;

      nextBlock = sub->getSpan(floatBlock, nullptr);
      if (nextBlock) {
        nextBlock->abnrmSrcNum++;
      }
    }
  }
}

VmEnv::~VmEnv() {
  unsigned i;

  for (i = 0; i < funcsSize; i++) {
    delete[] funcs[i].name.buf;
  }
  delete[] funcs;
}
