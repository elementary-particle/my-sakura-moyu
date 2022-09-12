#ifndef _VM_ENV_H
#define _VM_ENV_H

#include <cstdint>
#include <cstdlib>
#include <cstdio>

#include "VmBin.h"
#include "VmAddrMap.h"
#include "VmSub.h"

struct VmFunc {
  int argSize;
  VmText name;
};

int subBefore(const void *sub0, const void *sub1);

class VmEnv
{
public:
  uint8_t *buf, *bufCp;
  uint32_t mainSubCp;
  unsigned regSize1, regSize2;
  VmText title;

  unsigned funcsSize;
  VmFunc *funcs;
  unsigned funcThreadStart;

  VmSub *peekSub(uint32_t subCp);
  VmAddrMap<VmSub> subMap;

  uint32_t getCp() {
    return bufCp - buf;
  }

  void setCp(uint32_t cp) {
    bufCp = buf + cp;
  }

  VmEnv(uint8_t *initBuf);
  void getInst(VmInst *inst, VmInst *prevInst);
  void getSubs();
  void printAsm(FILE *file) {
    VmEntry<VmSub> *sub;

    STAILQ_FOREACH(sub, &subMap.itemList, listLink) {
      fprintf(file, "\n.sub %08x\n", sub->addr);
      sub->printAsm(file);
    }
  }
  void printFlow(FILE *file) {
    VmEntry<VmSub> *sub, **sortedSubs;
    size_t i;

    sortedSubs = new VmEntry<VmSub> *[subMap.size()];
    i = 0;
    STAILQ_FOREACH(sub, &subMap.itemList, listLink) {
      sortedSubs[i] = sub;
      i++;
    }
    qsort(sortedSubs, subMap.size(), sizeof(VmSub *), subBefore);
    for (i = 0; i < subMap.size(); i++) {
      fprintf(file, "\nsub_%08x:\n", sortedSubs[i]->addr);
      sortedSubs[i]->printFlow(file);
    }
    delete[] sortedSubs;
  }
  ~VmEnv();
};

#endif // _VM_ENV_H
