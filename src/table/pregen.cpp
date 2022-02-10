#include "pregen.h"

namespace PikaChess {
PreGen PRE_GEN;

/** 位棋盘掩码 */
extern __m128i BITBOARD_MASK[90];
/** 位棋盘反掩码 */
extern __m128i BITBOARD_NOT_MASK[90];

PreGen::PreGen() {
  // 位棋盘掩码初始化
  for (quint8 index { 0 }; index < 90; ++index) {
    BITBOARD_MASK[index] = __m128i(__uint128_t(1) << index);
    BITBOARD_NOT_MASK[index] = ~BITBOARD_MASK[index];
  }

  // 默认走法初始化
  INVALID_MOVE.setMove(EMPTY, EMPTY, 0, 0);

  // 生成占用位
  genRookOccupancy();
  genKnightOccupancy();
  genCannonOccupancy();
  genBishopOccupancy();
  genAttackByKnightOccupancy();

  // 计算PEXT移位
  genShiftRook();
  genShiftKnight();
  genShiftCannon();
  genShiftBishop();
  genShiftAttackByKnight();

  // 生成走法
  genRook();
  genKnight();
  genCannon();
  genRedPawn();
  genBlackPawn();
  genBishop();
  genAdvisor();
  genKing();
  genAttackByKnight();
  genAttackByRedPawn();
  genAttackByBlackPawn();

  // 生成Zobrist值;
  genZobristValues();
}

void PreGen::genRookOccupancy() {
#pragma omp parallel for
  for (quint8 index = 0; index < 90; ++index) {
    qint8 rank = index / 9;
    qint8 file = index % 9;
    Bitboard bitboard { };
    // 上下左右
    for (qint8 up = rank - 1; up > 0; --up) bitboard.setBit(up * 9 + file);
    for (qint8 down = rank + 1; down < 9; ++down) bitboard.setBit(down * 9 + file);
    for (qint8 left = file - 1; left > 0; --left) bitboard.setBit(rank * 9 + left);
    for (qint8 right = file + 1; right < 8; ++right) bitboard.setBit(rank * 9 + right);
    this->m_rookOccupancy[index][0] = bitboard.m_bitboard[0];
    this->m_rookOccupancy[index][1] = bitboard.m_bitboard[1];
  }
}

void PreGen::genKnightOccupancy() {
#pragma omp parallel for
  for (quint8 index = 0; index < 90; ++index) {
    qint8 rank = index / 9;
    qint8 file = index % 9;
    Bitboard bitboard { };
    // 马腿上下左右
    if (rank > 0) bitboard.setBit((rank - 1) * 9 + file);
    if (rank < 9) bitboard.setBit((rank + 1) * 9 + file);
    if (file > 0) bitboard.setBit(rank * 9 + file - 1);
    if (file < 8) bitboard.setBit(rank * 9 + file + 1);
    this->m_knightOccupancy[index][0] = bitboard.m_bitboard[0];
    this->m_knightOccupancy[index][1] = bitboard.m_bitboard[1];
  }
}

void PreGen::genCannonOccupancy() {
#pragma omp parallel for
  for (quint8 index = 0; index < 90; ++index) {
    qint8 rank = index / 9;
    qint8 file = index % 9;
    Bitboard bitboard { };
    // 上下左右
    for (qint8 up = rank - 1; up >= 0; --up) bitboard.setBit(up * 9 + file);
    for (qint8 down = rank + 1; down <= 9; ++down) bitboard.setBit(down * 9 + file);
    for (qint8 left = file - 1; left >= 0; --left) bitboard.setBit(rank * 9 + left);
    for (qint8 right = file + 1; right <= 8; ++right) bitboard.setBit(rank * 9 + right);
    this->m_cannonOccupancy[index][0] = bitboard.m_bitboard[0];
    this->m_cannonOccupancy[index][1] = bitboard.m_bitboard[1];
  }
}

void PreGen::genBishopOccupancy() {
#pragma omp parallel for
  for (quint8 index = 0; index < 90; ++index) {
    qint8 rank = index / 9;
    qint8 file = index % 9;
    Bitboard bitboard { };
    // 可以向上走的条件
    if (index == 63 or index == 67 or index == 71 or index == 83 or index == 87 or
        index == 18 or index == 22 or index == 26 or index == 38 or index == 42) {
      if (file > 0) bitboard.setBit((rank - 1) * 9 + file - 1);
      if (file < 8) bitboard.setBit((rank - 1) * 9 + file + 1);
    }
    // 可以往下走的条件
    if (index == 47 or index == 51 or index == 63 or index == 67 or index == 71 or
        index == 2 or index == 6 or index == 18 or index == 22 or index == 26) {
      if (file > 0) bitboard.setBit((rank + 1) * 9 + file - 1);
      if (file < 8) bitboard.setBit((rank + 1) * 9 + file + 1);
    }
    this->m_bishopOccupancy[index][0] = bitboard.m_bitboard[0];
    this->m_bishopOccupancy[index][1] = bitboard.m_bitboard[1];
  }
}

void PreGen::genAttackByKnightOccupancy() {
#pragma omp parallel for
  for (quint8 index = 0; index < 90; ++index) {
    qint8 rank = index / 9;
    qint8 file = index % 9;
    Bitboard bitboard { };
    // 可以向上走的条件
    if (rank > 0) {
      if (file > 0) bitboard.setBit((rank - 1) * 9 + file - 1);
      if (file < 8) bitboard.setBit((rank - 1) * 9 + file + 1);
    }
    // 可以往下走的条件
    if (rank < 9) {
      if (file > 0) bitboard.setBit((rank + 1) * 9 + file - 1);
      if (file < 8) bitboard.setBit((rank + 1) * 9 + file + 1);
    }
    this->m_attackByKnightOccupancy[index][0] = bitboard.m_bitboard[0];
    this->m_attackByKnightOccupancy[index][1] = bitboard.m_bitboard[1];
  }
}

Bitboard PreGen::getEnumOccupancy(const quint64 &occ0, const quint64 &occ1, quint32 count)
{
  Bitboard occupancy { };
  occupancy.m_bitboard[0] = occ0;
  occupancy.m_bitboard[1] = occ1;

  Bitboard bitboard;
  for (quint8 i { 0 }; i < 90; ++i) {
    if (occupancy[i]) {
      if (count & 1) bitboard.setBit(i);
      count >>= 1;
    }
  }

  return bitboard;
}

void PreGen::genShiftRook() {
#pragma omp parallel for
  for (int i = 0; i < 90; ++i) {
    // 计算高位的位移
    this->m_rookShift[i] = _mm_popcnt_u64(this->m_rookOccupancy[i][1]);
  }
}

void PreGen::genShiftKnight() {
#pragma omp parallel for
  for (int i = 0; i < 90; ++i) {
    // 计算高位的位移
    this->m_knightShift[i] = _mm_popcnt_u64(this->m_knightOccupancy[i][1]);
  }
}

void PreGen::genShiftCannon() {
#pragma omp parallel for
  for (int i = 0; i < 90; ++i) {
    // 计算高位的位移
    this->m_cannonShift[i] = _mm_popcnt_u64(this->m_cannonOccupancy[i][1]);
  }
}

void PreGen::genShiftBishop() {
#pragma omp parallel for
  for (int i = 0; i < 90; ++i) {
    // 计算高位的位移
    this->m_bishopShift[i] = _mm_popcnt_u64(this->m_bishopOccupancy[i][1]);
  }
}

void PreGen::genShiftAttackByKnight() {
#pragma omp parallel for
  for (int i = 0; i < 90; ++i) {
    // 计算高位的位移
    this->m_attackByKnightShift[i] = _mm_popcnt_u64(
        this->m_attackByKnightOccupancy[i][1]);
  }
}

Bitboard PreGen::getPreRookAttack(qint8 pos, const Bitboard &occupancy) {
  qint8 rank = pos / 9;
  qint8 file = pos % 9;
  Bitboard bitboard;
  // 向上走的情况
  qint8 up = rank - 1;
  while (up > 0 and not occupancy[up * 9 + file]) {
    bitboard.setBit(up * 9 + file);
    --up;
  }
  if (up >= 0) bitboard.setBit(up * 9 + file);
  // 向下走的情况
  qint8 down = rank + 1;
  while (down < 9 and not occupancy[down * 9 + file]) {
    bitboard.setBit(down * 9 + file);
    ++down;
  }
  if (down <= 9) bitboard.setBit(down * 9 + file);
  // 向左走的情况
  qint8 left = file - 1;
  while (left > 0 and not occupancy[rank * 9 + left]) {
    bitboard.setBit(rank * 9 + left);
    --left;
  }
  if (left >= 0) bitboard.setBit(rank * 9 + left);
  // 向右走的情况
  qint8 right = file + 1;
  while (right < 8 and not occupancy[rank * 9 + right]) {
    bitboard.setBit(rank * 9 + right);
    ++right;
  }
  if (right <= 8) bitboard.setBit(rank * 9 + right);
  return bitboard;
}

Bitboard PreGen::getPreKnightAttack(qint8 pos, const Bitboard &occupancy) {
  qint8 rank = pos / 9;
  qint8 file = pos % 9;
  Bitboard bitboard;
  // 马腿上下左右
  if (rank > 1 and not occupancy[(rank - 1) * 9 + file]) {
    if (file > 0) bitboard.setBit((rank - 2) * 9 + file - 1);
    if (file < 8) bitboard.setBit((rank - 2) * 9 + file + 1);
  }
  if (rank < 8 and not occupancy[(rank + 1) * 9 + file]) {
    if (file > 0) bitboard.setBit((rank + 2) * 9 + file - 1);
    if (file < 8) bitboard.setBit((rank + 2) * 9 + file + 1);
  }
  if (file > 1 and not occupancy[rank * 9 + file - 1]) {
    if (rank > 0) bitboard.setBit((rank - 1) * 9 + file - 2);
    if (rank < 9) bitboard.setBit((rank + 1) * 9 + file - 2);
  }
  if (file < 7 and not occupancy[rank * 9 + file + 1]) {
    if (rank > 0) bitboard.setBit((rank - 1) * 9 + file + 2);
    if (rank < 9) bitboard.setBit((rank + 1) * 9 + file + 2);
  }
  return bitboard;
}

Bitboard PreGen::getPreCannonAttack(qint8 pos, const Bitboard &occupancy) {
  qint8 rank = pos / 9;
  qint8 file = pos % 9;
  Bitboard bitboard;
  // 向上走的情况
  qint8 up = rank - 1;
  while (up >= 0 and not occupancy[up * 9 + file]) {
    bitboard.setBit(up * 9 + file);
    --up;
  }
  --up;
  while (up >= 0 and not occupancy[up * 9 + file]) --up;
  if (up >= 0) bitboard.setBit(up * 9 + file);
  // 向下走的情况
  qint8 down = rank + 1;
  while (down <= 9 and not occupancy[down * 9 + file]) {
    bitboard.setBit(down * 9 + file);
    ++down;
  }
  ++down;
  while (down <= 9 and not occupancy[down * 9 + file]) ++down;
  if (down <= 9) bitboard.setBit(down * 9 + file);
  // 向左走的情况
  qint8 left = file - 1;
  while (left >= 0 and not occupancy[rank * 9 + left]) {
    bitboard.setBit(rank * 9 + left);
    --left;
  }
  --left;
  while (left >= 0 and not occupancy[rank * 9 + left]) --left;
  if (left >= 0) bitboard.setBit(rank * 9 + left);
  // 向右走的情况
  qint8 right = file + 1;
  while (right <= 8 and not occupancy[rank * 9 + right]) {
    bitboard.setBit(rank * 9 + right);
    ++right;
  }
  ++right;
  while (right <= 8 and not occupancy[rank * 9 + right]) ++right;
  if (right <= 8) bitboard.setBit(rank * 9 + right);
  return bitboard;
}

Bitboard PreGen::getPreBishopAttack(qint8 pos, const Bitboard &occupancy) {
  Bitboard bitboard;
  // 左上
  if (pos == 22 or pos == 26 or pos == 38 or pos == 42 or
      pos == 67 or pos == 71 or pos == 83 or pos == 87) {
    if (not occupancy[pos - 10]) bitboard.setBit(pos - 20);
  }
  // 右上
  if (pos == 18 or pos == 22 or pos == 38 or pos == 42 or
      pos == 63 or pos == 67 or pos == 83 or pos == 87) {
    if (not occupancy[pos - 8]) bitboard.setBit(pos - 16);
  }
  // 左下
  if (pos == 2 or pos == 6 or pos == 22 or pos == 26 or
      pos == 47 or pos == 51 or pos == 67 or pos == 71) {
    if (not occupancy[pos + 8]) bitboard.setBit(pos + 16);
  }
  // 右下
  if (pos == 2 or pos == 6 or pos == 18 or pos == 22 or
      pos == 47 or pos == 51 or pos == 63 or pos == 67) {
    if (not occupancy[pos + 10]) bitboard.setBit(pos + 20);
  }
  return bitboard;
}

Bitboard PreGen::getPreAttackByKnight(qint8 pos, const Bitboard &occupancy) {
  qint8 rank = pos / 9;
  qint8 file = pos % 9;
  Bitboard bitboard;
  // 马腿斜线四方
  if (rank > 0 and not occupancy[(rank - 1) * 9 + file - 1]) {
    bitboard.setBit((rank - 1) * 9 + file - 2);
    if (rank > 1) bitboard.setBit((rank - 2) * 9 + file - 1);
  }
  if (rank > 0 and not occupancy[(rank - 1) * 9 + file + 1]) {
    bitboard.setBit((rank - 1) * 9 + file + 2);
    if (rank > 1) bitboard.setBit((rank - 2) * 9 + file + 1);
  }
  if (rank < 9 and not occupancy[(rank + 1) * 9 + file - 1]) {
    bitboard.setBit((rank + 1) * 9 + file - 2);
    if (rank < 8) bitboard.setBit((rank + 2) * 9 + file - 1);
  }
  if (rank < 9 and not occupancy[(rank + 1) * 9 + file + 1]) {
    bitboard.setBit((rank + 1) * 9 + file + 2);
    if (rank < 8) bitboard.setBit((rank + 2) * 9 + file + 1);
  }
  return bitboard;
}

void PreGen::genRook() {
#pragma omp parallel for
  for (quint8 index = 0; index < 90; ++index) {
    for (quint32 i { 0 }; i < 32768; ++i) {
      Bitboard occupancy { getEnumOccupancy(this->m_rookOccupancy[index][0],
                                          this->m_rookOccupancy[index][1], i) };

      quint64 pextIndex = occupancy.getPextIndex(this->m_rookOccupancy[index][0],
                                                 this->m_rookOccupancy[index][1],
                                                 this->m_rookShift[index]);

      this->m_rookAttack[index][pextIndex] = getPreRookAttack(index, occupancy);
    }
  }
}

void PreGen::genKnight() {
#pragma omp parallel for
  for (quint8 index = 0; index < 90; ++index) {
    for (quint32 i { 0 }; i < 16; ++i) {
      Bitboard occupancy { getEnumOccupancy(this->m_knightOccupancy[index][0],
                                          this->m_knightOccupancy[index][1], i) };

      quint64 pextIndex = occupancy.getPextIndex(this->m_knightOccupancy[index][0],
                                                 this->m_knightOccupancy[index][1],
                                                 this->m_knightShift[index]);

      this->m_knightAttack[index][pextIndex] = getPreKnightAttack(index, occupancy);
    }
  }
}

void PreGen::genCannon() {
#pragma omp parallel for
  for (quint8 index = 0; index < 90; ++index) {
    for (quint32 i { 0 }; i < 131072; ++i) {
      Bitboard occupancy { getEnumOccupancy(this->m_cannonOccupancy[index][0],
                                          this->m_cannonOccupancy[index][1], i) };

      quint64 pextIndex = occupancy.getPextIndex(this->m_cannonOccupancy[index][0],
                                                 this->m_cannonOccupancy[index][1],
                                                 this->m_cannonShift[index]);

      this->m_cannonAttack[index][pextIndex] = getPreCannonAttack(index, occupancy);
    }
  }
}

void PreGen::genBishop() {
  std::vector<int> poss { 2, 6, 18, 22, 26, 38, 42, 47, 51, 63, 67, 71, 83, 87 };
#pragma omp parallel for
  for (quint8 index : poss) {
    for (quint32 i { 0 }; i < 16; ++i) {
      Bitboard occupancy { getEnumOccupancy(this->m_bishopOccupancy[index][0],
                                          this->m_bishopOccupancy[index][1], i) };

      quint64 pextIndex = occupancy.getPextIndex(this->m_bishopOccupancy[index][0],
                                                 this->m_bishopOccupancy[index][1],
                                                 this->m_bishopShift[index]);

      this->m_bishopAttack[index][pextIndex] = getPreBishopAttack(index, occupancy);
    }
  }
}

void PreGen::genRedPawn() {
#pragma omp parallel for
  for (quint8 index = 0; index <= 62; ++index) {
    if (index == 46 or index == 55 or index == 48 or index == 57 or index == 50 or index == 59
        or index == 52 or index == 61) continue;
    qint8 rank = index / 9;
    qint8 file = index % 9;
    Bitboard &bitboard { this->m_redPawnAttack[index] };
    // 前面的一个格子
    if (rank > 0) bitboard.setBit((rank - 1) * 9 + file);
    // 过河后左右格子也要看
    if (rank < 5) {
      if (file > 0) bitboard.setBit(rank * 9 + file - 1);
      if (file < 8) bitboard.setBit(rank * 9 + file + 1);
    }
  }
}

void PreGen::genBlackPawn() {
#pragma omp parallel for
  for (quint8 index = 27; index < 90; ++index) {
    if (index == 28 or index == 37 or index == 30 or index == 39 or index == 32 or index == 41
        or index == 34 or index == 43) continue;
    qint8 rank = index / 9;
    qint8 file = index % 9;
    Bitboard &bitboard { this->m_blackPawnAttack[index] };
    // 前面的一个格子
    if (rank < 9) bitboard.setBit((rank + 1) * 9 + file);
    // 过河后左右格子也要看
    if (rank > 4) {
      if (file > 0) bitboard.setBit(rank * 9 + file - 1);
      if (file < 8) bitboard.setBit(rank * 9 + file + 1);
    }
  }
}

void PreGen::genAdvisor() {
#pragma omp parallel for
  for (quint8 index = 0; index < 90; ++index) {
    qint8 rank = index / 9;
    qint8 file = index % 9;
    Bitboard &bitboard { this->m_advisorAttack[index] };
    // 可以向左上走的条件
    if (index == 13 or index == 23 or index == 76 or index == 86) {
      bitboard.setBit((rank - 1) * 9 + file - 1);
    }
    // 可以向右上走的条件
    if (index == 13 or index == 21 or index == 76 or index == 84) {
      bitboard.setBit((rank - 1) * 9 + file + 1);
    }
    // 可以往左下走的条件
    if (index == 5 or index == 13 or index == 68 or index == 76) {
      bitboard.setBit((rank + 1) * 9 + file - 1);
    }
    // 可以往右下走的条件
    if (index == 3 or index == 13 or index == 66 or index == 76) {
      bitboard.setBit((rank + 1) * 9 + file + 1);
    }
  }
}

void PreGen::genKing() {
#pragma omp parallel for
  for (quint8 index = 0; index < 90; ++index) {
    qint8 rank = index / 9;
    qint8 file = index % 9;
    Bitboard &bitboard { this->m_kingAttack[index] };
    // 可以往上走一个格子的情况
    if (index == 12 or index == 13 or index == 14 or
        index == 21 or index == 22 or index == 23 or
        index == 75 or index == 76 or index == 77 or
        index == 84 or index == 85 or index == 86) {
      bitboard.setBit((rank - 1) * 9 + file);
    }
    // 可以往下走一个格子的情况
    if (index == 3 or index == 4 or index == 5 or
        index == 12 or index == 13 or index == 14 or
        index == 66 or index == 67 or index == 68 or
        index == 75 or index == 76 or index == 77) {
      bitboard.setBit((rank + 1) * 9 + file);
    }
    // 可以往左走一个格子的情况
    if (index == 4 or index == 5 or index == 13 or
        index == 14 or index == 22 or index == 23 or
        index == 67 or index == 68 or index == 76 or
        index == 77 or index == 85 or index == 86) {
      bitboard.setBit(rank * 9 + file - 1);
    }
    // 可以往右走一个格子的情况
    if (index == 3 or index == 4 or index == 12 or
        index == 13 or index == 21 or index == 22 or
        index == 66 or index == 67 or index == 75 or
        index == 76 or index == 84 or index == 85) {
      bitboard.setBit(rank * 9 + file + 1);
    }
  }
}

void PreGen::genAttackByKnight() {
#pragma omp parallel for
  for (quint8 index = 0; index < 90; ++index) {
    for (quint32 i { 0 }; i < 16; ++i) {
      Bitboard occupancy { getEnumOccupancy(this->m_attackByKnightOccupancy[index][0],
                                          this->m_attackByKnightOccupancy[index][1], i) };

      quint64 pextIndex = occupancy.getPextIndex(this->m_attackByKnightOccupancy[index][0],
                                                 this->m_attackByKnightOccupancy[index][1],
                                                 this->m_attackByKnightShift[index]);

      this->m_attackByKnight[index][pextIndex] = getPreAttackByKnight(index, occupancy);
    }
  }
}

void PreGen::genAttackByRedPawn() {
#pragma omp parallel for
  for (quint8 index = 0; index <= 62; ++index) {
    qint8 rank = index / 9;
    qint8 file = index % 9;
    Bitboard &bitboard { this->m_attackByRedPawn[index] };
    // 后面的一个格子
    if (index <= 35 or index == 45 or index == 36 or
        index == 47 or index == 38 or
        index == 49 or index == 40 or
        index == 51 or index == 42 or
        index == 53 or index == 44) {
      bitboard.setBit((rank + 1) * 9 + file);
    }
    // 左右格子
    if (index <= 44) {
      if (file > 0) bitboard.setBit(rank * 9 + file - 1);
      if (file < 8) bitboard.setBit(rank * 9 + file + 1);
    }
  }
}

void PreGen::genAttackByBlackPawn() {
#pragma omp parallel for
  for (quint8 index = 27; index < 90; ++index) {
    qint8 rank = index / 9;
    qint8 file = index % 9;
    Bitboard &bitboard { this->m_attackByBlackPawn[index] };
    // 前面的一个格子
    if (index >= 54 or index == 45 or index == 36 or
        index == 47 or index == 38 or
        index == 49 or index == 40 or
        index == 51 or index == 42 or
        index == 53 or index == 44) {
      bitboard.setBit((rank - 1) * 9 + file);
    }
    // 左右格子
    if (index >= 45) {
      if (file > 0) bitboard.setBit(rank * 9 + file - 1);
      if (file < 8) bitboard.setBit(rank * 9 + file + 1);
    }
  }
}

void PreGen::genZobristValues() {
  // 随机数生成引擎
  std::mt19937 engine(time(NULL));

  // 均匀分布
  std::uniform_int_distribution<quint64> uniform;

  // 生成每一个位置的Zobrist值
  for (quint8 index { 0 }; index < 90; ++index) {
    // 生成这个位置上每一个棋子的Zobrist值
    for (quint8 chess { RED_ROOK }; chess <= BLACK_KING; ++chess) {
      this->m_zobrists[index][chess] = uniform(engine);
    }
  }

     // 生成选边的Zobrist值
  this->m_sideZobrist = uniform(engine);
}

Bitboard PreGen::getAttack(quint8 chessType, quint8 index, const Bitboard &occupancy) {
  // 保证case n(n连续)，留给编译器使用函数指针数组来优化
  switch (chessType) {
  case RED_ROOK: return getRookAttack(index, occupancy);
  case RED_KNIGHT: return getKnightAttack(index, occupancy);
  case RED_CANNON: return getCannonAttack(index, occupancy);
  case RED_BISHOP: return getBishopAttack(index, occupancy);
  case RED_PAWN: return getRedPawnAttack(index);
  case RED_ADVISOR: return getAdvisorAttack(index);
  case RED_KING: return getKingAttack(index);
  case BLACK_ROOK: return getRookAttack(index, occupancy);
  case BLACK_KNIGHT: return getKnightAttack(index, occupancy);
  case BLACK_CANNON: return getCannonAttack(index, occupancy);
  case BLACK_BISHOP: return getBishopAttack(index, occupancy);
  case BLACK_PAWN: return getBlackPawnAttack(index);
  case BLACK_ADVISOR: return getAdvisorAttack(index);
  case BLACK_KING: return getKingAttack(index);
  }
  return { };
}

Bitboard PreGen::getRookAttack(quint8 index, const Bitboard &occupancy) const {
  quint64 pextIndex { occupancy.getPextIndex(this->m_rookOccupancy[index][0],
                                           this->m_rookOccupancy[index][1],
                                           this->m_rookShift[index]) };
  return this->m_rookAttack[index][pextIndex];
}

Bitboard PreGen::getKnightAttack(quint8 index, const Bitboard &occupancy) const {
  quint64 pextIndex { occupancy.getPextIndex(this->m_knightOccupancy[index][0],
                                           this->m_knightOccupancy[index][1],
                                           this->m_knightShift[index]) };
  return this->m_knightAttack[index][pextIndex];
}

Bitboard PreGen::getCannonAttack(quint8 index, const Bitboard &occupancy) const {
  quint64 pextIndex { occupancy.getPextIndex(this->m_cannonOccupancy[index][0],
                                           this->m_cannonOccupancy[index][1],
                                           this->m_cannonShift[index]) };
  return this->m_cannonAttack[index][pextIndex];
}

Bitboard PreGen::getBishopAttack(quint8 index, const Bitboard &occupancy) const {
  quint64 pextIndex { occupancy.getPextIndex(this->m_bishopOccupancy[index][0],
                                           this->m_bishopOccupancy[index][1],
                                           this->m_bishopShift[index]) };
  return this->m_bishopAttack[index][pextIndex];
}

Bitboard PreGen::getRedPawnAttack(quint8 index) const { return this->m_redPawnAttack[index]; }

Bitboard PreGen::getBlackPawnAttack(quint8 index) const { return this->m_blackPawnAttack[index]; }

Bitboard PreGen::getAdvisorAttack(quint8 index) const { return this->m_advisorAttack[index]; }

Bitboard PreGen::getKingAttack(quint8 index) const { return this->m_kingAttack[index]; }

Bitboard PreGen::getAttackByKnight(quint8 index, const Bitboard &occupancy) const {
  quint64 pextIndex { occupancy.getPextIndex(this->m_attackByKnightOccupancy[index][0],
                                           this->m_attackByKnightOccupancy[index][1],
                                           this->m_attackByKnightShift[index]) };
  return this->m_attackByKnight[index][pextIndex];
}

Bitboard PreGen::getAttackByPawn(quint8 side, quint8 index) const {
  if (RED == side) return this->m_attackByBlackPawn[index];
  else return this->m_attackByRedPawn[index];
}

quint64 PreGen::getZobrist(quint8 chess, quint8 index) { return this->m_zobrists[index][chess]; }

quint64 PreGen::getSideZobrist() { return this->m_sideZobrist; }
}
