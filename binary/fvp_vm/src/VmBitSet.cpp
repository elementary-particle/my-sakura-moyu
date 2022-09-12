#include "VmBitSet.h"

class BitSetNoSuchBit {};
class BitSetSizeMismatch {};

void VmBitSet::set(size_t bit) {
  size_t i;
  if (bit >= size) {
    throw BitSetNoSuchBit();
  }
  for (i = 0; bit >= NUM_BITS; i++, bit -= NUM_BITS);
  bitNums[i] |= ((UnitInt)1 << bit);
}

void VmBitSet::clear() {
  size_t i;
  for (i = 0; i < count; i++) {
    bitNums[i] = 0;
  }
}

size_t VmBitSet::min() {
  size_t bit, i;
  for (bit = 0, i = count; i < count; bit += NUM_BITS, i++) {
    if (bitNums[i]) {
      return bit + __builtin_ctz(bitNums[i]);
    }
  }
  return size;
}

size_t VmBitSet::max() {
  size_t bit, i;
  for (bit = count * NUM_BITS - 1, i = count; i > 0; ) {
    i--;
    if (bitNums[i]) {
      return bit - __builtin_clz(bitNums[i]);
    }
    bit -= NUM_BITS;
  }
  return size;
}

void VmBitSet::updateAnd(const VmBitSet &bitSet1) {
  size_t i;
  if (size != bitSet1.size) {
    throw BitSetSizeMismatch();
  }
  for (i = 0; i < count; i++) {
    bitNums[i] &= bitSet1.bitNums[i];
  }
}

