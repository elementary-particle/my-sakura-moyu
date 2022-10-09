#ifndef _VM_BITSET_H
#define _VM_BITSET_H

#include <cstddef>
#include <cstring>

class VmBitSet {
  public:
    using UnitInt = long;
    static constexpr size_t NUM_BITS = (sizeof(UnitInt) * 8);

    size_t size, count;
    UnitInt *bitNums;
    VmBitSet(size_t initSize) {
      size_t i;

      size = initSize;
      count = (size - 1) / NUM_BITS + 1;
      bitNums = new UnitInt[count];
      for (i = 0; i < count; i++) {
        bitNums[i] = (UnitInt)-1;
      }
    }

    void set(size_t bit);
    void clear();
    size_t min();
    size_t max();
    void updateAnd(const VmBitSet &bitSet1);

    ~VmBitSet() {
      delete[] bitNums;
    }
};

#endif // _VM_BITSET_H
