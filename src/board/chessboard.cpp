#include "chessboard.h"

namespace PikaChess {
/** 用于解析fen的映射 */
QMap<QChar, quint8> FEN_MAP {
    { 'R', RED_ROOK }, { 'N', RED_KNIGHT }, { 'C', RED_CANNON }, { 'B', RED_BISHOP },
    { 'P', RED_PAWN }, { 'A', RED_ADVISOR }, { 'K', RED_KING },
    { 'r', BLACK_ROOK }, { 'n', BLACK_KNIGHT }, { 'c', BLACK_CANNON }, { 'b', BLACK_BISHOP },
    { 'p', BLACK_PAWN }, { 'a', BLACK_ADVISOR }, { 'k', BLACK_KING }
};

/** 用于生成fen的映射 */
QMap<quint8, QChar> FEN_REVERSE_MAP {
    { RED_ROOK, 'R' }, { RED_KNIGHT, 'N' }, { RED_CANNON, 'C' }, { RED_BISHOP, 'B' },
    { RED_PAWN, 'P' }, { RED_ADVISOR, 'A' }, { RED_KING, 'K' },
    { BLACK_ROOK, 'r' }, { BLACK_KNIGHT, 'n' }, { BLACK_CANNON, 'c' }, { BLACK_BISHOP, 'b' },
    { BLACK_PAWN, 'p' }, { BLACK_ADVISOR, 'a' }, { BLACK_KING, 'k' }
};

/** 判断两个位置是否同一行 */
bool SAME_RANK[90][90];

/** 判断是否需要检测被捉 */
quint8 CHASE_INFO[14][14];

void Chessboard::parseFen(const QString &fen) {
  // 先清空原有的棋盘信息
  for (auto &bitboard : this->m_bitboards) bitboard.clearAllBits();
  this->m_redOccupancy.clearAllBits();
  this->m_blackOccupancy.clearAllBits();
  this->m_occupancy.clearAllBits();
  this->m_piece = 0;
  memset(this->m_helperBoard, EMPTY, sizeof(this->m_helperBoard));

  // 分割为棋盘和选边两部分
  QStringList fenList { fen.split(' ') };

  // 当前位置的计数器
  quint8 count { 0 };
  // 遍历整个棋盘fen，将对应的内容填入位棋盘和辅助棋盘中
  foreach (const auto &ch, fenList[0]) {
    switch (ch.toLatin1()) {
    case '/': continue;
    default:
      if (ch.isNumber()) { count += (ch.toLatin1() - '0'); continue; }
      ++this->m_piece;
      this->m_bitboards[FEN_MAP[ch]].setBit(count);
      this->m_helperBoard[count] = FEN_MAP[ch];
      break;
    }
    if (ch.isLower()) this->m_blackOccupancy.setBit(count);
    else this->m_redOccupancy.setBit(count);
    this->m_occupancy.setBit(count);
    ++count;
  }

  // 决定选边
  if ("w" == fenList[1]) this->m_side = RED;
  else this->m_side = BLACK;

  // 重置步数计数器
  this->m_historyMovesCount = 1;

  // 刷新双方的初始累加器
  Accumulator &acc { this->getLastMove().m_acc };
  qint32 featureIndexes[33];

  // 刷新对方的累加器
  this->m_side ^= OPP_SIDE;
  acc.kingPos[this->m_side] = this->m_bitboards[KING + this->m_side].getLastBitIndex();
  this->getAllFeatures(featureIndexes);
  featureTransformer->refreshAccumulator(acc, this->m_side, featureIndexes);

  // 刷新自己的累加器
  this->m_side ^= OPP_SIDE;
  acc.kingPos[this->m_side] = this->m_bitboards[KING + this->m_side].getLastBitIndex();
  this->getAllFeatures(featureIndexes);
  featureTransformer->refreshAccumulator(acc, this->m_side, featureIndexes);
}

QString Chessboard::getFen() const {
  QString fen;

  // 空位置计数器
  quint8 count { 0 };
  // 遍历辅助棋盘，取出棋子放入fen中
  for (quint8 rank { 0 }; rank < 10; ++rank) {
    for (quint8 file { 0 }; file < 9; ++file){
      quint8 index = rank * 9 + file;
      if (this->m_helperBoard[index] not_eq EMPTY) {
        if (count) { fen += QString::number(count); count = 0; }
        fen += FEN_REVERSE_MAP[this->m_helperBoard[index]];
      } else ++count;
    }

    // 一行结束了，清空空位置计数器
    if (count) { fen += QString::number(count); count = 0; }

    // 不是最后一行就补上/
    if (9 not_eq rank) fen += '/';
  }

  // 补充选边信息
  fen += " ";
  fen += RED == this->m_side ? 'w' : 'b';

  return fen;
}

quint8 Chessboard::genCapMoves(ValuedMove *moveList) const {
  // 可用的位置
  Bitboard available { this->m_side == RED ? this->m_blackOccupancy : this->m_redOccupancy };

  // 生成的走法数
  quint8 total { 0 };

  // 遍历所有棋子，生成对应的走法
  Bitboard bitboard { this->m_side == RED ? this->m_redOccupancy : this->m_blackOccupancy };

  // 遍历位棋盘上的每一个位
  quint8 index;
  while ((index = bitboard.getLastBitIndex()) < 90) {
    bitboard.clearBit(index);
    quint8 chess { this->m_helperBoard[index] };

    // 获取这个位可以攻击到的位置
    Bitboard attack { PRE_GEN.getAttack(chess, index, this->m_occupancy) & available };

    // 遍历可以攻击的位置，生成对应的走法
    quint8 victimIndex;
    while ((victimIndex = attack.getLastBitIndex()) < 90) {
      attack.clearBit(victimIndex);

      ValuedMove &move { moveList[total++] };
      move.setMove(chess, this->m_helperBoard[victimIndex], index, victimIndex);
      // 计算棋子的MVVLVA
      qint8 score { MVVLVA[move.victim()] };
      // 如果棋子被保护了还要减去攻击者的分值
      if (isProtected(move.to(), this->m_side)) score -= MVVLVA[chess];
      move.setScore(score);
    }
  }

  return total;
}

quint8 Chessboard::genNonCapMoves(ValuedMove *moveList) const {
  // 可用的位置
  Bitboard available { ~this->m_occupancy };

  // 生成的走法数
  quint8 total { 0 };

  // 遍历所有棋子，生成对应的走法
  Bitboard bitboard { this->m_side == RED ? this->m_redOccupancy : this->m_blackOccupancy };

  // 遍历位棋盘上的每一个位
  quint8 index;
  while ((index = bitboard.getLastBitIndex()) < 90) {
    bitboard.clearBit(index);
    quint8 chess { this->m_helperBoard[index] };

    // 获取这个位可以攻击到的位置
    Bitboard attack { PRE_GEN.getAttack(chess, index, this->m_occupancy) & available };
    // 遍历可以攻击的位置，生成对应的走法
    quint8 attackIndex;
    while ((attackIndex = attack.getLastBitIndex()) < 90) {
      attack.clearBit(attackIndex);
      ValuedMove &move { moveList[total++] };
      move.setMove(chess, EMPTY, index, attackIndex);
      move.setScore(this->m_historyTable.getValue(move));
    }
  }

  return total;
}

void Chessboard::getAllFeatures(qint32 *featureIndexes) const {
  // 获取当前走子方将的位置
  quint8 kingPos { this->getLastMove().m_acc.kingPos[this->m_side] };

  // 遍历所有位置，提取特征
  Bitboard occupancy { this->m_occupancy };

  quint8 index;
  while ((index = occupancy.getLastBitIndex()) < 90) {
    occupancy.clearBit(index);
    *featureIndexes++ = FeatureIndex(this->m_side, index, this->m_helperBoard[index], kingPos);
  }

  // 结束标志
  *featureIndexes = -1;
}

bool Chessboard::isChecked() const {
  // 获取对方的选边
  quint8 oppSide = this->m_side ^ OPP_SIDE;

  // 首先获取棋盘上的将的位置
  quint8 index { this->m_bitboards[KING + this->m_side].getLastBitIndex() };

  return
      // 把将当作车来走，看能不能吃到对方的车或将（将帅对脸）
      ((PRE_GEN.getRookAttack(index, this->m_occupancy) &
       (this->m_bitboards[ROOK + oppSide] | this->m_bitboards[KING + oppSide])) or
      // 把将当作马来走，看能不能吃到对方的马
      (PRE_GEN.getAttackByKnight(index, this->m_occupancy) &
       this->m_bitboards[KNIGHT + oppSide]) or
      // 把将当作炮来走，看能不能吃到对方的炮
      (PRE_GEN.getCannonAttack(index, this->m_occupancy) &
       this->m_bitboards[CANNON + oppSide]) or
      // 把将当作兵来走，看能不能吃到对方的兵
      (PRE_GEN.getAttackByPawn(this->m_side, index) & this->m_bitboards[PAWN + oppSide]));
}

bool Chessboard::isProtected(quint8 index, quint8 side) const {
  // 获取对方的选边
  quint8 oppSide = side ^ OPP_SIDE;

  return
      // 看这个位置有没有被车保护
      (PRE_GEN.getRookAttack(index, this->m_occupancy) &
       this->m_bitboards[ROOK + oppSide]) or
      // 看这个位置有没有被炮保护
      (PRE_GEN.getCannonAttack(index, this->m_occupancy) &
       this->m_bitboards[CANNON + oppSide]) or
      // 看这个位置有没有被马保护
      (PRE_GEN.getAttackByKnight(index, this->m_occupancy) &
       this->m_bitboards[KNIGHT + oppSide]) or
      // 看这个位置有没有被兵保护
      (PRE_GEN.getAttackByPawn(side, index) & this->m_bitboards[PAWN + oppSide]) or
      // 看这个位置有没有被象保护
      (PRE_GEN.getBishopAttack(index, this->m_occupancy) &
       this->m_bitboards[BISHOP + oppSide]) or
      // 看这个位置有没有被士保护
      (PRE_GEN.getAdvisorAttack(index) & this->m_bitboards[ADVISOR + oppSide]) or
      // 看这个位置有没有被将保护
      (PRE_GEN.getKingAttack(index) & this->m_bitboards[KING + oppSide]);
}

bool Chessboard::isLegalMove(Move &move) const {
  // 看看这个地方是不是自己的棋子，检查所去之处是否有自己的棋子
  const Bitboard &occupancy { RED == this->m_side ?
                                this->m_redOccupancy : this->m_blackOccupancy };
  if (not occupancy[move.from()] or occupancy[move.to()]) return false;

  // 在符合的情况下首先检查该走法是否符合相关棋子的步法
  move.setChess(this->m_helperBoard[move.from()]);
  if (PRE_GEN.getAttack(move.chess(), move.from(), this->m_occupancy)[move.to()]) {
    // 修正吃子情况
    move.setVictim(this->m_helperBoard[move.to()]);
    return true;
  }

  return false;
}

std::optional<qint16> Chessboard::getRepeatScore(quint8 distance) const {
  /* mySide代表的是是否是调用本函数的那一方(下称"我方")
   * 因为一调用搜索就马上调用了本函数，我方没有走棋
   * 所以在检查重复走法时，历史走法表中最后一项保存的是对方的最后一步
   * 所以这个变量的初始值为假，代表这一步不是我方，因为走法从后往前遍历 */
  bool mySide { false };

  // 双方的长打标志
  quint16 myFlag { 0x7fff }, oppFlag { 0x7fff };

  // 指向历史走法表的最后一项，往前遍历
  const HistoryMove *move = &this->m_historyMoves[this->m_historyMovesCount - 1];
  /* 必须保证步法有效，也就是没有到头部哨兵或者空步裁剪处
   * 如果遇到空步裁剪就不往下检测了，因为空步无法算作有效步
   * 并且要求不是吃子步，因为吃子就打破长打了 */
  while (not move->isNullMove() and not move->isCapture()) {
    if (mySide) {
      // 如果是我方，更新我方长打信息
      myFlag &= move->getFlag();

      // 如果检测到局面与当前局面重复就返回对应的分数
      if (move->zobrist() == this->m_zobrist) {
        myFlag = (myFlag & 0x3fff) == 0 ? myFlag : 0x3fff;
        oppFlag = (oppFlag & 0x3fff) == 0 ? oppFlag : 0x3fff;

        // 我方长打返回负分，对方长打返回正分，双方长打返回0分
        if (myFlag > oppFlag) return BAN_SCORE_LOSS + distance;
        else if (myFlag < oppFlag) return BAN_SCORE_MATE - distance;
        else return 0;
      }
    }
    // 如果是对方，更新对方的将军信息
    else oppFlag &= move->getFlag();

    // 更新选边信息
    mySide = not mySide;

    // move指向前一个走法
    --move;
  }

  // 没有重复局面
  return std::nullopt;
}

bool Chessboard::makeMove(Move &move) {
  // 如果这步是吃子步，从被吃掉的棋子的位棋盘中移除对应的位
  if (move.isCapture()) {
    if (RED == this->m_side) this->m_blackOccupancy.clearBit(move.to());
    else this->m_redOccupancy.clearBit(move.to());
    this->m_bitboards[move.victim()].clearBit(move.to());
    // 存活的子少了一个
    --this->m_piece;
    // 注意，这里不用移除occupancy中move.to()位，因为攻击的棋子会移动过来
  }

  // 将move.to()的位置设置上，并从原来的位置移除对应的位
  else this->m_occupancy.setBit(move.to());
  this->m_occupancy.clearBit(move.from());
  if (RED == this->m_side) {
    this->m_redOccupancy.setBit(move.to());
    this->m_redOccupancy.clearBit(move.from());
  } else {
    this->m_blackOccupancy.setBit(move.to());
    this->m_blackOccupancy.clearBit(move.from());
  }
  this->m_bitboards[move.chess()].setBit(move.to());
  this->m_bitboards[move.chess()].clearBit(move.from());

  // 检查走完之后是否被将军了，如果被将军了就撤销这一步
  if (isChecked()) { undoMove(move); return false; }

  // 走动辅助棋盘
  this->m_helperBoard[move.from()] = EMPTY;
  this->m_helperBoard[move.to()] = move.chess();

  // 获取上一个累加器
  const Accumulator &lastAcc { this->getLastMove().m_acc };

  // 在历史走法表中记录这一个走法
  HistoryMove &historyMove { this->m_historyMoves[this->m_historyMovesCount++] };

  // 记录现在的走法
  historyMove.setMove(move);

  // 记录现在的Zobrist值
  historyMove.setZobrist(this->m_zobrist);

  // 计算新的Zobrist值
  this->m_zobrist ^= PRE_GEN.getSideZobrist();
  this->m_zobrist ^= PRE_GEN.getZobrist(move.chess(), move.from());
  this->m_zobrist ^= PRE_GEN.getZobrist(move.chess(), move.to());
  // 吃子步需要把被吃的子的zobrist去除
  if (move.isCapture()) this->m_zobrist ^= PRE_GEN.getZobrist(move.victim(), move.to());

  // 如果走动的是将，就刷新自己的累加器
  if (move.chess() == KING + this->m_side) {
    historyMove.m_acc.kingPos[this->m_side] = move.to();
    qint32 featureIndexes[33];
    this->getAllFeatures(featureIndexes);
    featureTransformer->refreshAccumulator(historyMove.m_acc, this->m_side, featureIndexes);
  }
  // 否则就更新自己的累加器
  else featureTransformer->updateAccumulator(lastAcc, historyMove.m_acc, this->m_side, move);

  // 换边
  this->m_side ^= OPP_SIDE;

  // 不要忘记另一边累加器的也要更新
  featureTransformer->updateAccumulator(lastAcc, historyMove.m_acc, this->m_side, move);

  // 补充对应的将军捉子信息
  if (isChecked()) historyMove.setChecked();
  else historyMove.setChase(this->getChase());

  // 走子成功
  return true;
}

void Chessboard::unMakeMove() {
  // 换边
  this->m_side ^= OPP_SIDE;

  // 从历史走法表中取出最后一个走法
  const HistoryMove &move { this->m_historyMoves[--this->m_historyMovesCount] };

  // 还原辅助棋盘
  this->m_helperBoard[move.to()] = move.victim();
  this->m_helperBoard[move.from()] = move.chess();

  // 还原原来的Zobrist值
  this->m_zobrist = move.zobrist();

  // 撤销这个走法
  undoMove(move);
}

void Chessboard::makeNullMove() {
  // 获取历史走法表项，并将自增走法历史表的大小
  const Accumulator &lastAcc { this->getLastMove().m_acc };
  HistoryMove &move = this->m_historyMoves[this->m_historyMovesCount++];
  // 设置空步信息
  move.setNullMove();
  // 复制上一个累加器的内容
  this->getLastMove().m_acc.copyFrom(lastAcc);
  // 换边
  this->m_side ^= OPP_SIDE;
  // 计算新的Zobrist值
  this->m_zobrist ^= PRE_GEN.getSideZobrist();
}

void Chessboard::unMakeNullMove() {
  // 自减历史走法表大小
  --this->m_historyMovesCount;
  // 换边
  this->m_side ^= OPP_SIDE;
  // 还原Zobrist
  this->m_zobrist ^= PRE_GEN.getSideZobrist();
}

void Chessboard::updateHistoryValue(const Move &move, quint8 depth) {
  this->m_historyTable.updateValue(move, depth);
}

HistoryMove &Chessboard::getLastMove() {
  return this->m_historyMoves[this->m_historyMovesCount - 1];
}

const HistoryMove &Chessboard::getLastMove() const {
  return this->m_historyMoves[this->m_historyMovesCount - 1];
}

void Chessboard::undoMove(const Move &move) {
  // 恢复原来的位
  this->m_bitboards[move.chess()].clearBit(move.to());
  this->m_bitboards[move.chess()].setBit(move.from());
  if (RED == this->m_side) {
    this->m_redOccupancy.clearBit(move.to());
    this->m_redOccupancy.setBit(move.from());
  } else {
    this->m_blackOccupancy.clearBit(move.to());
    this->m_blackOccupancy.setBit(move.from());
  }

  // 如果这步是吃子步，就设置回对应的位，注意，这里不用再次设置occupancy，因为之前没有清除这个位
  if (move.isCapture()) {
    if (RED == this->m_side) this->m_blackOccupancy.setBit(move.to());
    else this->m_redOccupancy.setBit(move.to());
    this->m_bitboards[move.victim()].setBit(move.to());
    // 恢复存活子
    ++this->m_piece;
    // 注意，如果是吃子步则不用清空to，因为这里原来有一个棋子
  }

  // 恢复原来的位
  else this->m_occupancy.clearBit(move.to());
  this->m_occupancy.setBit(move.from());
}

quint16 Chessboard::getChase() {
  quint8 side = this->m_side ^ OPP_SIDE;
  
  const HistoryMove &move { this->getLastMove() };

  quint16 flag { 0 };
  
  // 首先获取被抓的棋子
  Bitboard chase { RED == side ? this->m_blackOccupancy : this->m_redOccupancy };
  switch(move.chess() - side) {
  case ROOK:
    chase &= PRE_GEN.getRookChase(move.to(), SAME_RANK[move.from()][move.to()], m_occupancy);
    break;
  case CANNON:
    chase &= PRE_GEN.getCannonChase(move.to(), SAME_RANK[move.from()][move.to()], m_occupancy);
    break;
  case KNIGHT:
    chase &= PRE_GEN.getKnightAttack(move.to(), this->m_occupancy);
    break;
  default:
    // 其余棋子不予考虑
    return 0;
  }

  // 接下来查表决定是否是捉
  quint8 index;
  while ((index = chase.getLastBitIndex()) < 90) {
    chase.clearBit(index);
    quint8 victim { this->m_helperBoard[index] };
    switch (CHASE_INFO[move.chess()][victim]) {
    case 1: flag |= CHESS_FLAG[victim]; break;
    case 2: if (not this->isProtected(index, side)) flag |= CHESS_FLAG[victim]; break;
    case 3:
      if (PRE_GEN.getSide(side)[index] and not this->isProtected(index, side)) {
        flag |= CHESS_FLAG[victim];
      }
    default:
      break;
    }
  }

  return flag;
}

quint8 Chessboard::side() const { return this->m_side; }

void Chessboard::setSide(quint8 newSide) { this->m_side = newSide; }

quint64 Chessboard::zobrist() const { return this->m_zobrist; }
}
