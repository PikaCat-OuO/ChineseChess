#pragma once
#include "bitboard.h"
#include "move.h"

namespace PikaChess {
class PreGen final {
public:
  PreGen();

  /** 预计算数据生成 */
  void genRookOccupancy();
  void genKnightOccupancy();
  void genCannonOccupancy();
  void genBishopOccupancy();
  void genAttackByKnightOccupancy();
  void genCenterOccupancy();
  Bitboard getEnumOccupancy(const quint64 &occ0, const quint64 &occ1, quint32 count);

  void genShiftRook();
  void genShiftKnight();
  void genShiftCannon();
  void genShiftBishop();
  void genShiftAttackByKnight();
  void genShiftCenter();

  Bitboard getPreRookAttack(qint8 pos, const Bitboard &occupancy);
  Bitboard getPreKnightAttack(qint8 pos, const Bitboard &occupancy);
  Bitboard getPreCannonAttack(qint8 pos, const Bitboard &occupancy);
  Bitboard getPreBishopAttack(qint8 pos, const Bitboard &occupancy);
  Bitboard getPreAttackByKnight(qint8 pos, const Bitboard &occupancy);
  void genRook();
  void genKnight();
  void genCannon();
  void genBishop();
  void genRedPawn();
  void genBlackPawn();
  void genAdvisor();
  void genKing();
  void genAttackByKnight();
  void genAttackByRedPawn();
  void genAttackByBlackPawn();
  void genCenter();
  void genSide();

  void genZobristValues();

  /** 获取棋子的攻击位 */
  Bitboard getAttack(quint8 chessType, quint8 index, const Bitboard &occupancy);
  /** 获取攻击位 */
  Bitboard getRookAttack(quint8 index, const Bitboard &occupancy) const;
  Bitboard getKnightAttack(quint8 index, const Bitboard &occupancy) const;
  Bitboard getCannonAttack(quint8 index, const Bitboard &occupancy) const;
  Bitboard getBishopAttack(quint8 index, const Bitboard &occupancy) const;
  Bitboard getRedPawnAttack(quint8 index) const;
  Bitboard getBlackPawnAttack(quint8 index) const;
  Bitboard getAdvisorAttack(quint8 index) const;
  Bitboard getKingAttack(quint8 index) const;

  /** 获取被马攻击的位置 */
  Bitboard getAttackByKnight(quint8 index, const Bitboard &occupancy) const;
  /** 获取被兵攻击的位置 */
  Bitboard getAttackByPawn(quint8 side, quint8 index) const;

  /** 获取双方的炮打窝心的位置 */
  Bitboard getCenter(bool red, const Bitboard &occupancy) const;

  /** 获取选边对应的位置 */
  Bitboard getRedSide() const;
  Bitboard getBlackSide() const;

  /** 获取某个棋子在某个位置上的Zobrist值 */
  quint64 getZobrist(quint8 chess, quint8 index);

  /** 获取选边的Zobrist值 */
  quint64 getSideZobrist();

private:
  /** 所有滑动棋子的占用位，用于走法生成 */
  quint64 m_rookOccupancy[90][2];
  quint64 m_knightOccupancy[90][2];
  quint64 m_cannonOccupancy[90][2];
  quint64 m_bishopOccupancy[90][2];
  quint64 m_attackByKnightOccupancy[90][2];
  /** 炮打窝心的占用位 */
  quint64 m_centerOccupancy[2][2];

  /** 所有滑动棋子的PEXT移位，用于走法生成 */
  quint8 m_rookShift[90];
  quint8 m_knightShift[90];
  quint8 m_cannonShift[90];
  quint8 m_bishopShift[90];
  /** 被马攻击的PEXT移位 */
  quint8 m_attackByKnightShift[90];
  /** 炮打窝心的PEXT移位 */
  quint8 m_centerShift[2];

  /** 所有棋子的攻击位，用于走法生成 */
  Bitboard m_rookAttack[90][1 << 15];
  Bitboard m_knightAttack[90][1 << 4];
  Bitboard m_cannonAttack[90][1 << 17];
  Bitboard m_bishopAttack[90][1 << 4];
  Bitboard m_redPawnAttack[90];
  Bitboard m_blackPawnAttack[90];
  Bitboard m_advisorAttack[90];
  Bitboard m_kingAttack[90];
  /** 被马攻击的位置 */
  Bitboard m_attackByKnight[90][1 << 4];
  /** 被兵攻击的位置 */
  Bitboard m_attackByRedPawn[90];
  Bitboard m_attackByBlackPawn[90];
  /** 用于判断炮打窝心的位置 */
  Bitboard m_center[2][1 << 8];

  /** 双方的那边 */
  Bitboard m_redSide;
  Bitboard m_blackSide;

  /** 棋盘上每个位置，每个棋子的Zobrist值 */
  quint64 m_zobrists[90][14];
  /** 选边的zobrist值 */
  quint64 m_sideZobrist;
};
/** 全局的预计算数据 */
extern PreGen PRE_GEN;
}
