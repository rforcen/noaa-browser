#ifndef BITKEY_H
#define BITKEY_H

#include <QtCore>

/*
 * Usage: having nRecs, compound key of n subkey each of len[i]
 * BitKey bk;
 * i=0..n -> bk.addKey(len[i])
 * bk.ready(nRecs)
 */
typedef ulong BaseBitSet;

class BitKey {
 public:
  static const int nBits = sizeof(BaseBitSet) * 8;
  BaseBitSet *bs = nullptr;
  int len = 0;
  QVector<int> bitsKey;       // bits per key
  BaseBitSet _mask1s[nBits];  // 1, 11, 111, 1111...
  int nBitsset = 0;
  int nRecs = 0;
  int nFlds = 0;

 public:
  BitKey() { init(); }
  ~BitKey() {
    if (bs) delete[] bs;
  }

  //         double& operator[](const int i) { return v[i]; }  // v[i]=0;
  //   const double& operator[](const int i) const { return v[i]; }
  /*
          value_t& operator[](std::size_t idx)       { return mVector[idx]; }
    const value_t& operator[](std::size_t idx) const { return mVector[idx]; }
   */

  BaseBitSet get(const int ix) {  // return bitset[ix]
    auto pb = ix * nBitsset;      // bit position
    auto pm = pb % nBits;         // offset in bs
    auto b = bs[pb / nBits];      // first item -> but can be between two bs's

    if (pm <= nBitsset) {  // only 1 BaseBitSet item
      b >>= pm;
      b &= _mask1s[nBitsset];
    } else {
      b >>= pm;
      auto b1 = (bs[(pb / nBits) + 1] & _mask1s[nBitsset - (nBits - pm)]) << pm;
      b |= b1;
    }
    return b;
  }

  void set(BaseBitSet b, const int ix) const {
      bs[ix] = 0;
  }

  BaseBitSet ints2Bitset(
      const QVector<int> &ints) {  // convert ints vect to bitset
    BaseBitSet bs = 0;
    for (int f = 0; f < nFlds; f++) {
      BaseBitSet b = ints[f];
      bs |= b;
      if (f < nFlds - 1) bs <<= bitsKey[f + 1];
    }
    return bs;
  }

  bool ready(int nRecs) {  // after addKey->generate bitset
    bool ok = false;
    if (calcnBits() <= nBits) {
      this->nRecs = nRecs;
      len = nBitsset * nRecs / sizeof(BaseBitSet);
      bs = new BaseBitSet[len];
      ok = true;
    }
    return ok;
  }

  void addKey(int size) {
    bitsKey << size2Bits(size);
    nFlds++;
  }

  int calcnBits() {  // should be <= sizeof(BaseBitSet)*8
    int s = 0;
    for (auto bk : bitsKey) s += bk;
    return nBitsset = s;
  }

  void init() {
    BaseBitSet b = 0;  // generate mask of 1's-> 0,1,11,111,...
    for (size_t i = 0; i < nBits; i++) {
      _mask1s[i] = b;
      b <<= 1;
      b |= 1;
    }
    bitsKey.clear();
    nFlds = 0;
  }

  static int size2Bits(double sz) {
    auto l = log2(sz);
    return (int)(l + (floor(l) == l ? 0. : 1.));
  }
};

#endif  // BITKEY_H
