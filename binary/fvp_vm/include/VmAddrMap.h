#ifndef _VM_ADDR_MAP_H
#define _VM_ADDR_MAP_H

#include <cstddef>
#include <cstdint>

#include "sys/queue.h"

template <typename ItemType>
class VmEntry: public ItemType {
public:
  uint32_t addr;
  SLIST_ENTRY(VmEntry) mapLink;
  STAILQ_ENTRY(VmEntry) listLink;
  VmEntry(uint32_t initAddr) {
    addr = initAddr;
  }
};

/*
 * Linked Hash Map
 */
template <typename ItemType>
class VmAddrMap {
public:
  STAILQ_HEAD(Queue, VmEntry<ItemType>);
  SLIST_HEAD(List, VmEntry<ItemType>);
  Queue itemList;
private:
  size_t bucketSize, itemCount;
  List *bucket;

  size_t hash(uint32_t addr) {
    return ((addr ^ (addr >> 2) ^ (addr >> 5))) & (bucketSize - 1);
  }
public:
  VmAddrMap(size_t bucketScale) {
    size_t i;

    bucketSize = 1 << bucketScale;
    itemCount = 0;
    bucket = new List[bucketSize];
    for (i = 0; i < bucketSize; i++) {
      SLIST_INIT(&bucket[i]);
    }

    STAILQ_INIT(&itemList);
  }
  void add(VmEntry<ItemType> *object) {
    size_t hashValue;

    itemCount++;

    hashValue = hash(object->addr);
    SLIST_INSERT_HEAD(&bucket[hashValue], object, mapLink);
    STAILQ_INSERT_TAIL(&itemList, object, listLink);
  }

  size_t size() const {
    return itemCount;
  }

  VmEntry<ItemType> *get(uint32_t addr) {
    VmEntry<ItemType> *p;
    size_t hashValue;

    hashValue = hash(addr);

    SLIST_FOREACH(p, &bucket[hashValue], mapLink) {
      if (p->addr == addr) {
        break;
      }
    }
    return p;
  }
  ~VmAddrMap() {
    VmEntry<ItemType> *p, *q;
    STAILQ_FOREACH_SAFE(p, &itemList, listLink, q) {
      delete p;
    }
    delete[] bucket;
  }
};

#endif // _VM_ADDR_MAP_H
