#include "bitboard.h"

namespace PikaChess {
/** 位棋盘掩码 */
__m128i BITBOARD_MASK[90];
/** 位棋盘反掩码 */
__m128i BITBOARD_NOT_MASK[90];

Bitboard::Bitboard() :m_bitboard { _mm_setzero_si128() } { }

Bitboard::Bitboard(const __m128i &rhs) :m_bitboard { rhs } { }

Bitboard::operator bool() const {
  return not _mm_testz_si128(this->m_bitboard, this->m_bitboard);
}

Bitboard operator~(const Bitboard &bitboard) { return ~bitboard.m_bitboard; }

Bitboard operator<<(const Bitboard &bitboard, const qint32 &count) {
  return __m128i(__uint128_t(bitboard.m_bitboard) << count);
}

Bitboard operator>>(const Bitboard &bitboard, const qint32 &count) {
  return __m128i(__uint128_t(bitboard.m_bitboard) >> count);
}

Bitboard operator&(const Bitboard &lhs, const Bitboard &rhs) {
  return lhs.m_bitboard & rhs.m_bitboard;
}

Bitboard operator|(const Bitboard &lhs, const Bitboard &rhs) {
  return lhs.m_bitboard | rhs.m_bitboard;
}

Bitboard operator^(const Bitboard &lhs, const Bitboard &rhs) {
  return lhs.m_bitboard ^ rhs.m_bitboard;
}

bool operator==(const Bitboard &lhs, const Bitboard &rhs) {
  auto xorResult { lhs.m_bitboard ^ rhs.m_bitboard };
  return _mm_testz_si128(xorResult, xorResult);
}

void Bitboard::operator&=(const Bitboard &rhs) { this->m_bitboard &= rhs.m_bitboard; }

void Bitboard::operator|=(const Bitboard &rhs) { this->m_bitboard |= rhs.m_bitboard; }

void Bitboard::operator^=(const Bitboard &rhs) { this->m_bitboard ^= rhs.m_bitboard; }

void Bitboard::operator<<=(const quint8 &count) { *this = *this << count; }

void Bitboard::operator>>=(const quint8 &count) { *this = *this >> count; }

quint64 Bitboard::getPextIndex(const quint64 occ0, const quint64 occ1, const quint8 shift) const
{
  return _pext_u64(this->m_bitboard[0], occ0) << shift |
         _pext_u64(this->m_bitboard[1], occ1);
}

bool Bitboard::operator[](quint8 index) const {
  return not _mm_testz_si128(this->m_bitboard, BITBOARD_MASK[index]);
}

void Bitboard::setBit(quint8 index) { *this |= BITBOARD_MASK[index]; }

void Bitboard::clearBit(quint8 index) { *this &= BITBOARD_NOT_MASK[index]; }

void Bitboard::clearAllBits() { this->m_bitboard = _mm_setzero_si128(); }

uint8_t Bitboard::getLastBitIndex() const {
  if (this->m_bitboard[0]) return __tzcnt_u64(this->m_bitboard[0]);
  else return 64 + __tzcnt_u64(this->m_bitboard[1]);
}

uint8_t Bitboard::countBits() const {
  return _mm_popcnt_u64(this->m_bitboard[0]) + _mm_popcnt_u64(this->m_bitboard[1]);
}

void Bitboard::print() {
  qDebug("位棋盘的表示:");
  for (qsizetype rank { 0 }; rank < 10; ++rank) {
    // 打印每一个位
    for (qsizetype file { 0 }; file < 9; ++file) std::cout << (*this)[rank * 9 + file] << "  ";
    // 打印换行符
    std::cout << std::endl;
  }
  // 打印位棋盘的十六进制表示
  if (this->m_bitboard[1] & 0xFFFFFFFFC0000000) throw "位越界";
  qDebug("十六进制表示: 0x%016llx%016llx", this->m_bitboard[1], this->m_bitboard[0]);
}
}
