#include "searchmachine.h"

namespace PikaChess {
SearchMachine::SearchMachine(const Chessboard &chessboard, const Move &hashMove,
                             Move &killerMove1, Move &killerMove2)
    : m_chessboard { chessboard }, m_hashMove { hashMove },
      m_killerMove1 { killerMove1 }, m_killerMove2 { killerMove2 } { }

Move SearchMachine::getNextMove() {
  switch (this->m_phase) {
  case PHASE_HASH:
    // 指明下一个阶段
    this->m_phase = PHASE_CAPTURE_GEN;
    // 确保这一个置换表走法不是无效走法
    if (this->m_hashMove.isVaild()) return this->m_hashMove;
    // 否则就下一步
    [[fallthrough]];

  case PHASE_CAPTURE_GEN:
    // 指明下一个阶段
    this->m_phase = PHASE_CAPTURE;
    // 生成吃子走法，使用MVVLVA对其进行排序
    this->m_totalMoves = this->m_chessboard.genCapMoves(this->m_moveList);
    std::sort(std::begin(this->m_moveList), std::begin(this->m_moveList) + this->m_totalMoves);
    // 直接下一步
    [[fallthrough]];

  case PHASE_CAPTURE:
    // 遍历走法，逐个返回好的吃子走法，吃亏的吃子着法留到最后搜索
    while (this->m_nowMove < this->m_totalMoves) {
      const ValuedMove &move { this->m_moveList[this->m_nowMove++] };
      if (move.score() >= 0) return move;
      else { --this->m_nowMove; break;}
    }
    // 如果没有了就下一步
    [[fallthrough]];

  case PHASE_KILLER1:
    // 指明下一个阶段
    this->m_phase = PHASE_KILLER2;
    // 确保这一个杀手走法不是默认走法，不是置换表走法，并且要确认是否是合法的步
    if (this->m_killerMove1.isVaild() and
        this->m_killerMove1 not_eq this->m_hashMove and
        this->m_chessboard.isLegalMove(this->m_killerMove1)) {
      return this->m_killerMove1;
    }
    // 否则就下一步
    [[fallthrough]];

  case PHASE_KILLER2:
    // 指明下一个阶段
    this->m_phase = PHASE_NOT_CAPTURE_GEN;
    // 确保这一个杀手走法不是默认走法，不是置换表走法
    if (this->m_killerMove2.isVaild() and
        this->m_killerMove2 not_eq this->m_hashMove and
        this->m_chessboard.isLegalMove(this->m_killerMove2)) {
      return this->m_killerMove2;
    }
    // 否则就下一步
    [[fallthrough]];

  case PHASE_NOT_CAPTURE_GEN:
    // 指明下一个阶段
    this->m_phase = PHASE_REST;
    // 生成非吃子的走法并使用历史表对其进行排序
    this->m_totalMoves += this->m_chessboard.genNonCapMoves(
        this->m_moveList + this->m_totalMoves);
    std::sort(std::begin(this->m_moveList) + this->m_nowMove,
              std::begin(this->m_moveList) + this->m_totalMoves);
    // 直接下一步
    [[fallthrough]];

  case PHASE_REST:
    // 遍历走法，逐个检查并返回
    while (this->m_nowMove < this->m_totalMoves) {
      const Move &move { this->m_moveList[this->m_nowMove++] };
      if (move not_eq this->m_hashMove and
          move not_eq this->m_killerMove1 and
          move not_eq this->m_killerMove2) { return move; }
    }
    // 如果没有了就直接返回
    [[fallthrough]];

  default:
    return INVALID_MOVE;
  }
}
}
