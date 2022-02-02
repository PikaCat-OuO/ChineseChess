#include "searchquiescencemachine.h"

namespace PikaChess {
SearchQuiescenceMachine::SearchQuiescenceMachine(const Chessboard &chessboard)
    : m_chessboard { chessboard } { }

Move SearchQuiescenceMachine::getNextMove() {
  switch (this->m_phase) {
  case PHASE_CAPTURE_GEN:
    // 指明下一个阶段
    this->m_phase = PHASE_CAPTURE;
    // 生成吃子走法，使用MVVLVA对其进行排序
    this->m_totalMoves = this->m_chessboard.genCapMoves(this->m_moveList);
    std::sort(std::begin(this->m_moveList), std::begin(this->m_moveList) + this->m_totalMoves);
    // 直接下一步
    [[fallthrough]];

  case PHASE_CAPTURE:
    /* 遍历走法，逐个返回好的吃子走法，吃亏的吃子着法留到最后搜索 */
    while (this->m_nowMove < this->m_totalMoves) {
      const ValuedMove &move { this->m_moveList[this->m_nowMove++] };
      if (move.score() >= 0) return move;
      else { --this->m_nowMove; break; }
    }
    // 如果没有了就下一步
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
    // 遍历走法，逐个返回
    while (this->m_nowMove < this->m_totalMoves) return this->m_moveList[this->m_nowMove++];
    // 如果没有了就直接返回
    [[fallthrough]];

  default:
    return INVALID_MOVE;
  }
}
}