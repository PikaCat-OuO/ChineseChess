#pragma once
#include <x86intrin.h>
#include "global.h"

namespace PikaChess {
/** 位棋盘类，包装__m128i */
class Bitboard final {
public:
  friend class PreGen;

  /** 默认构造函数，位棋盘全部置0 */
  Bitboard();

  /** 转换函数，在bool测试中使用 */
  operator bool() const;

  /** 按位取反 */
  friend Bitboard operator~(const Bitboard &bitboard);
  /** 按位左移，使用内置的__uint128_t获得最好的效果 */
  friend Bitboard operator<<(const Bitboard &bitboard, const qint32 &count);
  /** 按位右移，使用内置的__uint128_t获得最好的效果 */
  friend Bitboard operator>>(const Bitboard &bitboard, const qint32 &count);
  /** 按位与运算 */
  friend Bitboard operator&(const Bitboard &lhs, const Bitboard &rhs);
  /** 按位或运算 */
  friend Bitboard operator|(const Bitboard &lhs, const Bitboard &rhs);
  /** 按位异或 */
  friend Bitboard operator^(const Bitboard &lhs, const Bitboard &rhs);

  /** 比较两个位棋盘是否相等 */
  friend bool operator==(const Bitboard &lhs, const Bitboard &rhs);

  /** 按位与并保存 */
  void operator&=(const Bitboard &rhs);
  /** 按位或并保存 */
  void operator|=(const Bitboard &rhs);
  /** 按位异或并保存 */
  void operator^=(const Bitboard &rhs);
  /** 按位左移并保存 */
  void operator<<=(const quint8 &count);
  /** 按位右移并保存 */
  void operator>>=(const quint8 &count);


  /** 用于PEXT位棋盘 */
  quint64 getPextIndex(const quint64 occ0, const quint64 occ1, const quint8 shift) const;

  /** 获取某个位置上面的位 */
  bool operator[](quint8 index) const;
  /** 设置某个位置上面的位 */
  void setBit(quint8 index);
  /** 清除某个位置上面的位 */
  void clearBit(quint8 index);
  /** 清除位棋盘上的所有位 */
  void clearAllBits();
  /** 获得最后一个1的下标 */
  quint8 getLastBitIndex() const;
  /** 数一下现在位棋盘上有多少个1 */
  quint8 countBits() const;

  /** 打印位棋盘 */
  void print();

protected:
  /** 使用128位变量构造位棋盘 */
  Bitboard(const __m128i &rhs);

private:
  /** 位棋盘实际的实现，使用__m128i类型 */
  __m128i m_bitboard;
};
}
