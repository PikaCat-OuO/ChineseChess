#include "chessboard.h"

namespace PikaChess {
/** 真实用于评分的价值表 */
/** 各个子力的价值表 */
qint16 VALUE[14][90];
/** 空头炮的罚分表 */
qint16 HOLLOW_THREAT_PENALTY[2][90];
/** 沉底炮的罚分表 */
qint16 BOTTOM_THREAT_PENALTY[90];
/** 缺士的罚分值 */
qint16 ADVISOR_LEAKAGE_PENALTY[2];
/** 先行棋的分数 */
qint16 ADVANCED_SCORE;
/** 炮镇窝心马的罚分表 */
constexpr qint16 CENTER_KNIGHT_PENALTY[2][90] {{
    0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0, 50,  0,  0,  0,  0,
    0,  0,  0,  0, 45,  0,  0,  0,  0,
    0,  0,  0,  0, 40,  0,  0,  0,  0,
    0,  0,  0,  0, 35,  0,  0,  0,  0,
    0,  0,  0,  0, 30,  0,  0,  0,  0,
    0,  0,  0,  0, 30,  0,  0,  0,  0,
    0,  0,  0,  0, 30,  0,  0,  0,  0
}, {
    0,  0,  0,  0, 30,  0,  0,  0,  0,
    0,  0,  0,  0, 30,  0,  0,  0,  0,
    0,  0,  0,  0, 30,  0,  0,  0,  0,
    0,  0,  0,  0, 35,  0,  0,  0,  0,
    0,  0,  0,  0, 40,  0,  0,  0,  0,
    0,  0,  0,  0, 45,  0,  0,  0,  0,
    0,  0,  0,  0, 50,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,
}};
/** 中炮的罚分表 */
qint16 CENTER_THREAT_PENALTY[2][90] {{
    0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0, 12,  0,  0,  0,  0,
    0,  0,  0,  0, 11,  0,  0,  0,  0,
    0,  0,  0,  0, 10,  0,  0,  0,  0,
    0,  0,  0,  0,  8,  0,  0,  0,  0,
    0,  0,  0,  0,  7,  0,  0,  0,  0,
    0,  0,  0,  0,  7,  0,  0,  0,  0,
    0,  0,  0,  0,  7,  0,  0,  0,  0
}, {
    0,  0,  0,  0,  7,  0,  0,  0,  0,
    0,  0,  0,  0,  7,  0,  0,  0,  0,
    0,  0,  0,  0,  7,  0,  0,  0,  0,
    0,  0,  0,  0,  8,  0,  0,  0,  0,
    0,  0,  0,  0, 10,  0,  0,  0,  0,
    0,  0,  0,  0, 11,  0,  0,  0,  0,
    0,  0,  0,  0, 12,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,
}};

qint16 Chessboard::score() const {
  return staticScore() + kingSafety();
}

qint16 Chessboard::staticScore() const {
  /*
    为何此处要额外加上一个ADVANCED_SCORE先行棋分？因为执行该函数时本身是轮到该层玩家走棋，
    但是因为各种原因只能搜索到这里了，该层玩家没有走棋而直接返回了这个局面下自己的评分!
    实际上这样对他的评价是不够正确的，所以加上一个补偿分数，代表下一步是该玩家先行，使得评价更公正一些!
  */
  if (RED == this->m_side) {
    return this->m_redScore - this->m_blackScore + ADVANCED_SCORE;
  } else return this->m_blackScore - this->m_redScore + ADVANCED_SCORE;
}

qint16 Chessboard::kingSafety() const {
  // 红黑双方的王安全分
  qint16 redSafety { kingSafety_helper(RED, 76, 84, 85, 86) };
  qint16 blackSafety { kingSafety_helper(BLACK, 13, 3, 4, 5) };

  // 根据选边返回王安全分
  if (RED == this->m_side) return redSafety - blackSafety;
  else return blackSafety - redSafety;
}

qint16 Chessboard::kingSafety_helper(quint8 side, quint8 center,
                                     quint8 left, quint8 middle, quint8 right) const {
  bool red { side == RED };
  quint8 oppSide = side ^ OPP_SIDE;

  // 获取士的形状
  quint8 shape { SHAPE_NONE };
  // 缺士
  if (2 not_eq this->m_bitboards[ADVISOR + side].countBits(red)) shape = SHAPE_LEAK;
  // 将不在中间
  else if (KING + side not_eq this->m_helperBoard[middle]) shape = SHAPE_HOLLOW;
  // 左右士
  else if (this->m_helperBoard[left] == ADVISOR + side and
           this->m_helperBoard[left] == this->m_helperBoard[right]) {
    shape = SHAPE_CENTER;
  // 确认花心有一个士，左或右有一个士
  } else if (this->m_helperBoard[center] == ADVISOR + side and
           (this->m_helperBoard[left] == ADVISOR + side or
            this->m_helperBoard[right] == ADVISOR + side)) shape = SHAPE_LR;

  // 根据士的形状来决定逻辑
  quint8 index;
  qint16 safety { 0 };
  switch(shape) {
  case SHAPE_LEAK:
    // 缺士怕双车
    if (2 == this->m_bitboards[ROOK + oppSide].countBits()) {
      safety -= ADVISOR_LEAKAGE_PENALTY[red];
    }
    break;

  case SHAPE_HOLLOW:
    // 有双士，但将占领花心
    if (KING + side == this->m_helperBoard[center]) safety -= 20;
    break;

  case SHAPE_LR:
    // 中士加左或右士，查看将的另一边有无沉底炮
    index = (PRE_GEN.getRookAttack(middle, this->m_occupancy) &
             this->m_bitboards[CANNON + oppSide]).getLastBitIndex();
    if (index < 90) safety -= BOTTOM_THREAT_PENALTY[index];
    // 计算中炮威胁
    index = (PRE_GEN.getCannonChase(center, false, this->m_occupancy) &
             this->m_bitboards[CANNON + oppSide]).getLastBitIndex();
    if (index < 90) {
      safety -= CENTER_THREAT_PENALTY[red][index];
      // 将门被对方控制，给予一定的罚分
      index = ADVISOR + side == this->m_helperBoard[right] ? left : right;
      if (this->isProtected(index, side) or
          PRE_GEN.getRookAttack(index, this->m_occupancy) & this->m_bitboards[KING + oppSide]) {
        safety -= 20;
      }
      // 如果车在底线保护将，给予更大的罚分
      index = (PRE_GEN.getRookAttack(middle, this->m_occupancy) &
               this->m_bitboards[ROOK + side]).getLastBitIndex();
      if (index < 90) safety -= 80;
    }
    break;

  case SHAPE_CENTER:
    // 两边士，查看有无空头炮
    index = (PRE_GEN.getRookAttack(middle, this->m_occupancy) &
             this->m_bitboards[CANNON + oppSide]).getLastBitIndex();
    if (index < 90) safety -= HOLLOW_THREAT_PENALTY[red][index];
    // 如果不存在空头炮，就计算是否存在炮镇窝心马
    else if (KNIGHT + side == this->m_helperBoard[center]) {
      index = (PRE_GEN.getCannonChase(center, false, this->m_occupancy) &
               this->m_bitboards[CANNON + oppSide]).getLastBitIndex();
      if (index < 90) safety -= CENTER_KNIGHT_PENALTY[red][index];
    }
    break;

  default: break;
  }

  return safety;
}

void Chessboard::preCalculateScores() {
  qint16 pawnAttacking[90], pawnAttackless[90];

  // 首先判断局势处于开中局还是残局阶段，方法是计算各种棋子的数量，按照车=6、马炮=3、其它=1相加。
  qint16 midgameValue = (this->m_bitboards[RED_ADVISOR] | this->m_bitboards[BLACK_ADVISOR] |
                         this->m_bitboards[RED_BISHOP] | this->m_bitboards[BLACK_BISHOP] |
                         this->m_bitboards[RED_PAWN] | this->m_bitboards[BLACK_PAWN])
                            .countBits();
  midgameValue += (this->m_bitboards[RED_KNIGHT] | this->m_bitboards[BLACK_KNIGHT] |
                   this->m_bitboards[RED_CANNON] | this->m_bitboards[BLACK_CANNON])
                      .countBits() * 3;
  midgameValue += (this->m_bitboards[RED_ROOK] | this->m_bitboards[BLACK_ROOK]).countBits() * 6;
  // 使用二次函数，子力很少时才认为接近残局
  midgameValue = (132 - midgameValue) * midgameValue / 66;
  ADVANCED_SCORE = (4 * midgameValue + 2) / 66;

  auto intLerp { [&] (const qint16 a, const qint16 b) {
    return (a * midgameValue + b * (66 - midgameValue)) / 66;
  } };

  for (quint8 index { 0 }; index < 90; ++index) {
    // 计算过渡性的分数，首先回填将的分数
    VALUE[RED_KING][index] = VALUE[BLACK_KING][89 - index] =
        intLerp(CONST_KING_MIDGAME[index], CONST_KING_ENDGAME[index]);
    // 然后回填马的分数
    VALUE[RED_KNIGHT][index] = VALUE[BLACK_KNIGHT][89 - index] =
        intLerp(CONST_KNIGHT_MIDGAME[index], CONST_KNIGHT_ENDGAME[index]);
    // 然后回填车的分数
    VALUE[RED_ROOK][index] = VALUE[BLACK_ROOK][89 - index] =
        intLerp(CONST_ROOK_MIDGAME[index], CONST_ROOK_ENDGAME[index]);
    // 然后回填炮的分数
    VALUE[RED_CANNON][index] = VALUE[BLACK_CANNON][89 - index] =
        intLerp(CONST_CANNON_MIDGAME[index], CONST_CANNON_ENDGAME[index]);
    // 最后计算兵的分数
    pawnAttacking[index] =
        intLerp(CONST_PAWN_ATTACKING_MIDGAME[index], CONST_PAWN_ATTACKING_ENDGAME[index]);
    pawnAttackless[index] =
        intLerp(CONST_PAWN_ATTACKLESS_MIDGAME[index], CONST_PAWN_ATTACKLESS_ENDGAME[index]);
  }

  // 计算空头炮罚分
  for (quint8 index { 0 }; index < 90; ++index) {
    HOLLOW_THREAT_PENALTY[1][index] = HOLLOW_THREAT_PENALTY[0][89 - index] =
        CONST_HOLLOW_THREAT_PENALTY[index] * (midgameValue + 66) / 132;
  }

  // 然后判断各方是否处于进攻状态，方法是计算各种过河棋子的数量，按照车马2炮兵1相加。
  qint16 redAttacks { 0 }, blackAttacks { 0 };
  redAttacks += 2 * ((this->m_bitboards[RED_ROOK] | this->m_bitboards[RED_KNIGHT]) &
                     PRE_GEN.getBlackSide()).countBits();
  redAttacks += ((this->m_bitboards[RED_CANNON] | this->m_bitboards[RED_PAWN]) &
                 PRE_GEN.getBlackSide()).countBits();
  blackAttacks += 2 * ((this->m_bitboards[BLACK_ROOK] | this->m_bitboards[BLACK_KNIGHT]) &
                       PRE_GEN.getRedSide()).countBits();
  blackAttacks += ((this->m_bitboards[BLACK_CANNON] | this->m_bitboards[BLACK_PAWN]) &
                   PRE_GEN.getRedSide()).countBits();

  // 如果本方轻子数比对方多，那么每多一个轻子(车算2个轻子)威胁值加2。威胁值最多不超过8。
  qint16 redLights = 2 * this->m_bitboards[RED_ROOK].countBits();
  redLights += (this->m_bitboards[RED_KNIGHT] | this->m_bitboards[RED_CANNON]).countBits();
  qint16 blackLights = 2 * this->m_bitboards[BLACK_ROOK].countBits();
  blackLights += (this->m_bitboards[BLACK_KNIGHT] | this->m_bitboards[BLACK_CANNON]).countBits();
  if (redLights > blackLights) redAttacks += (redLights - blackLights) * 2;
  else blackAttacks += (blackLights - redLights) * 2;
  redAttacks = std::min(redAttacks, qint16(8));
  blackAttacks = std::min(blackAttacks, qint16(8));

  // 填写红黑双方的缺士罚分
  ADVISOR_LEAKAGE_PENALTY[0] = 10 * redAttacks;
  ADVISOR_LEAKAGE_PENALTY[1] = 10 * blackAttacks;

  // 计算沉底炮的威胁值
  for (quint8 index : { 0, 1, 7, 8 }) {
    BOTTOM_THREAT_PENALTY[index] = CONST_BOTTOM_THREAT_PENALTY[index] * redAttacks / 8;
  }
  for (quint8 index : { 81, 82, 88, 89 }) {
    BOTTOM_THREAT_PENALTY[index] = CONST_BOTTOM_THREAT_PENALTY[index] * blackAttacks / 8;
  }

  auto redLerp { [&] (const qint16 a, const qint16 b) {
    return (a * redAttacks + b * (8 - redAttacks)) / 8;
  } };

  auto blackLerp { [&] (const qint16 a, const qint16 b) {
    return (a * blackAttacks + b * (8 - blackAttacks)) / 8;
  } };

  // 计算象士兵的分值
  for (quint8 index { 0 }; index < 90; ++index) {
    VALUE[RED_BISHOP][index] =
        blackLerp(CONST_BISHOP_THREATENED[index], CONST_BISHOP_THREATLESS[index]);
    VALUE[BLACK_BISHOP][89 - index] =
        redLerp(CONST_BISHOP_THREATENED[index], CONST_BISHOP_THREATLESS[index]);
    VALUE[RED_ADVISOR][index] =
        blackLerp(CONST_ADVISOR_THREATENED[index], CONST_ADVISOR_THREATLESS[index]);
    VALUE[BLACK_ADVISOR][89 - index] =
        redLerp(CONST_ADVISOR_THREATENED[index], CONST_ADVISOR_THREATLESS[index]);
    VALUE[RED_PAWN][index] = redLerp(pawnAttacking[index], pawnAttackless[index]);
    VALUE[BLACK_PAWN][89 - index] = blackLerp(pawnAttacking[index], pawnAttackless[index]);
  }

  // 调整不受威胁方少掉的士、象分值
  this->m_redScore = 10 * (8 - blackAttacks);
  this->m_blackScore = 10 * (8 - redAttacks);

  // 最后重新计算子力位置分
  quint8 index;
  Bitboard redOccupancy { this->m_redOccupancy };
  while ((index = redOccupancy.getLastBitIndex()) < 90) {
    this->m_redScore += VALUE[this->m_helperBoard[index]][index];
    redOccupancy.clearBit(index);
  }
  Bitboard blackOccupancy { this->m_blackOccupancy };
  while ((index = blackOccupancy.getLastBitIndex()) < 90) {
    this->m_blackScore += VALUE[this->m_helperBoard[index]][index];
    blackOccupancy.clearBit(index);
  }
}
}
