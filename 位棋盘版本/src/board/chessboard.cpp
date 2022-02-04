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

// 设置历史走法表的头部哨兵节点
Chessboard::Chessboard() { this->m_historyMoves[0].setNullMove(true); }

void Chessboard::parseFen(const QString &fen) {
  // 先清空原有的棋盘信息
  for (auto &bitboard : this->m_bitboards) bitboard.clearAllBits();
  this->m_redOccupancy.clearAllBits();
  this->m_blackOccupancy.clearAllBits();
  this->m_occupancy.clearAllBits();
  memset(this->m_helperBoard, EMPTY, sizeof(this->m_helperBoard));

  // 分数归0
  this->m_redScore = this->m_blackScore = 0;

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
      this->m_bitboards[FEN_MAP[ch]].setBit(count);
      this->m_helperBoard[count] = FEN_MAP[ch];
      break;
    }
    if (ch.isLower()) {
      this->m_blackScore += VALUE[FEN_MAP[ch]][count];
      this->m_blackOccupancy.setBit(count);
    }
    else {
      this->m_redScore += VALUE[FEN_MAP[ch]][count];
      this->m_redOccupancy.setBit(count);
    }
    this->m_occupancy.setBit(count);
    ++count;
  }

  // 决定选边
  if ("w" == fenList[1]) this->m_side = RED;
  else this->m_side = BLACK;

  // 重置步数计数器
  this->m_historyMovesCount = 1;
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

template <typename MoveType>
requires std::is_same_v<ValuedMove, MoveType> or std::is_same_v<ValuedCapMove, MoveType>
quint8 Chessboard::genCapMoves(MoveType *moveList) const {
  // 可用的位置
  Bitboard available { this->m_side == RED ? this->m_blackOccupancy : this->m_redOccupancy };

  // 生成的走法数
  quint8 total { 0 };

  // 遍历所有棋子，生成对应的走法
  for (quint8 chess = ROOK + this->m_side; chess <= KING + this->m_side; ++chess) {
    // 首先获得该棋子的位棋盘
    Bitboard bitboard { this->m_bitboards[chess] };

    // 遍历位棋盘上的每一个位
    quint8 index;
    while ((index = bitboard.getLastBitIndex()) < 90) {
      bitboard.clearBit(index);

      // 获取这个位可以攻击到的位置
      Bitboard attack { PRE_GEN.getAttack(chess, index, this->m_occupancy) & available };

      // 遍历可以攻击的位置，生成对应的走法
      quint8 victimIndex;
      while ((victimIndex = attack.getLastBitIndex()) < 90) {
        attack.clearBit(victimIndex);

        MoveType &move { moveList[total++] };
        move.setMove(chess, this->m_helperBoard[victimIndex], index, victimIndex);
        // 计算棋子的MVVLVA
        qint8 score { MVVLVA[move.victim()] };
        // 如果棋子被保护了还要减去攻击者的分值
        if (isProtected(move.to())) score -= MVVLVA[chess];
        move.setScore(score);
      }
    }
  }

  return total;
}

template quint8 Chessboard::genCapMoves(ValuedMove *moveList) const;
template quint8 Chessboard::genCapMoves(ValuedCapMove *moveList) const;

quint8 Chessboard::genNonCapMoves(ValuedMove *moveList) const {
  // 可用的位置
  Bitboard available { ~this->m_occupancy };

  // 生成的走法数
  quint8 total { 0 };

  // 遍历所有棋子，生成对应的走法
  for (quint8 chess = ROOK + this->m_side; chess <= KING + this->m_side; ++chess) {
    // 首先获得该棋子的位棋盘
    Bitboard bitboard { this->m_bitboards[chess] };

    // 遍历位棋盘上的每一个位
    quint8 index;
    while ((index = bitboard.getLastBitIndex()) < 90) {
      bitboard.clearBit(index);
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
  }

  return total;
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

bool Chessboard::isProtected(quint8 index) const {
  // 获取对方的选边
  quint8 oppSide = this->m_side ^ OPP_SIDE;

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
      (PRE_GEN.getAttackByPawn(this->m_side, index) & this->m_bitboards[PAWN + oppSide]) or
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

bool Chessboard::isNotEndgame() const {
  if (RED == this->m_side) return this->m_redScore > 400;
  else return this->m_blackScore > 400;
}

std::optional<qint16> Chessboard::getRepeatScore(quint8 distance) const {
  /* mySide代表的是是否是调用本函数的那一方(下称"我方")
   * 因为一调用搜索就马上调用了本函数，我方没有走棋
   * 所以在检查重复走法时，历史走法表中最后一项保存的是对方的最后一步
   * 所以这个变量的初始值为假，代表这一步不是我方，因为走法从后往前遍历 */
  bool mySide { false };

  // 我方是否将军，对方是否将军
  bool myCheck { true }, oppCheck { true };

  // 指向历史走法表的最后一项，往前遍历
  const HistoryMove *move = &this->m_historyMoves[this->m_historyMovesCount - 1];
  /* 必须保证步法有效，也就是没有到头部哨兵或者空步裁剪处
   * 如果遇到空步裁剪就不往下检测了，因为空步无法算作有效步
   * 并且要求不是吃子步，因为吃子就打破长将了 */
  while (not move->isNullMove() and not move->isCapture()) {
    if (mySide) {
      // 如果是我方，更新我方将军信息
      myCheck &= move->isChecked();

      // 如果检测到局面与当前局面重复就返回对应的分数
      if (move->zobrist() == this->m_zobrist) {
        // 我方长将返回负分，对方长将返回正分
        qint16 score = (myCheck ? BAN_SCORE_LOSS + distance : 0) +
                       (oppCheck ? BAN_SCORE_MATE - distance : 0);

        /* 如果双方都长将或者双方都没有长将但是有重复局面就返回和棋的分数
         * 但无论如何都要使得和棋对于第一层的那一方来说是不利的，是负分
         * distance & 1 的作用是确定现在在那一层
         * 说明evaluate的那一层和第一层是同一方
         * 同一方返回负值，不同方返回正值，这样正值上到第一层就会变成负值 */
        if (score == 0) return distance & 1 ? -DRAW_SCORE : DRAW_SCORE;
        // 有一方长将
        else return score;
      }
    }
    // 如果是对方，更新对方的将军信息
    else oppCheck &= move->isChecked();

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

  // 在历史走法表中记录这一个走法
  HistoryMove &historyMove { this->m_historyMoves[this->m_historyMovesCount++] };

  // 清除空步信息
  historyMove.setNullMove(false);

  // 记录现在的走法
  historyMove.setMove(move);

  // 记录现在的Zobrist值
  historyMove.setZobrist(this->m_zobrist);

  // 计算新的Zobrist值
  this->m_zobrist ^= PRE_GEN.getSideZobrist();
  this->m_zobrist ^= PRE_GEN.getZobrist(move.chess(), move.from());
  this->m_zobrist ^= PRE_GEN.getZobrist(move.chess(), move.to());

  if (move.isCapture()) {
    // 吃子步需要把被吃的子的zobrist去除
    this->m_zobrist ^= PRE_GEN.getZobrist(move.victim(), move.to());
    // 顺便计算吃子得分
    if (RED == this->m_side) this->m_blackScore -= VALUE[move.victim()][move.to()];
    else this->m_redScore -= VALUE[move.victim()][move.to()];
  }

  // 计算得分
  if (RED == this->m_side) {
    this->m_redScore -= VALUE[move.chess()][move.from()];
    this->m_redScore += VALUE[move.chess()][move.to()];
  } else {
    this->m_blackScore -= VALUE[move.chess()][move.from()];
    this->m_blackScore += VALUE[move.chess()][move.to()];
  }

  // 换边
  this->m_side ^= OPP_SIDE;

  // 补充对应的将军信息
  historyMove.setCheck(isChecked());

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

  // 还原原来的得分
  if (RED == this->m_side) {
    this->m_redScore -= VALUE[move.chess()][move.to()];
    this->m_redScore += VALUE[move.chess()][move.from()];
    if (move.isCapture()) this->m_blackScore += VALUE[move.victim()][move.to()];
  } else {
    this->m_blackScore -= VALUE[move.chess()][move.to()];
    this->m_blackScore += VALUE[move.chess()][move.from()];
    if (move.isCapture()) this->m_redScore += VALUE[move.victim()][move.to()];
  }

  // 撤销这个走法
  undoMove(move);
}

void Chessboard::makeNullMove() {
  // 获取历史走法表项，并将自增走法历史表的大小
  HistoryMove &move = this->m_historyMoves[this->m_historyMovesCount++];
  // 保存本步棋是否将军对方的信息
  move.setCheck(false);
  // 设置空步信息
  move.setNullMove(true);
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
    // 注意，如果是吃子步则不用清空to，因为这里原来有一个棋子
  }

  // 恢复原来的位
  else this->m_occupancy.clearBit(move.to());
  this->m_occupancy.setBit(move.from());
}

quint8 Chessboard::side() const { return this->m_side; }

void Chessboard::setSide(quint8 newSide) { this->m_side = newSide; }

quint64 Chessboard::zobrist() const { return this->m_zobrist; }

qint16 Chessboard::score() const {
  /*
    为何此处要额外加上一个ADVANCED_SCORE先行棋分？因为执行该函数时本身是轮到该层玩家走棋，
    但是因为各种原因只能搜索到这里了，该层玩家没有走棋而直接返回了这个局面下自己的评分!
    实际上这样对他的评价是不够正确的，所以加上一个补偿分数，代表下一步是该玩家先行，使得评价更公正一些!
  */
  if (RED == this->m_side) return this->m_redScore - this->m_blackScore + ADVANCED_SCORE;
  else return this->m_blackScore - this->m_redScore + ADVANCED_SCORE;
}
}
