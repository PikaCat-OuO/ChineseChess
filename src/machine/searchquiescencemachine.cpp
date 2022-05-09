#include "searchquiescencemachine.h"

namespace PikaChess {
SearchQuiescenceMachine::SearchQuiescenceMachine(const Chessboard &chessboard, bool notInCheck)
    : m_chessboard { chessboard }, m_notInCheck { notInCheck } { }

Move SearchQuiescenceMachine::getNextMove() {
  switch (this->m_phase) {
  case PHASE_CAPTURE_GEN:
    // 指明下一个阶段
    this->m_phase = PHASE_CAPTURE;
    // 生成吃子走法，使用MVVLVA对其进行排序
    this->m_totalMoves = this->m_chessboard.genCapMoves(this->m_moveList);
    std::sort(this->m_moveList, this->m_moveList + this->m_totalMoves);
    // 直接下一步
    [[fallthrough]];

  case PHASE_CAPTURE:
    /* 遍历走法，逐个返回吃子走法 */
    while (this->m_nowMove < this->m_totalMoves) {
      const ValuedMove &move { this->m_moveList[this->m_nowMove++] };
      if (move.score() >= 0) return move;
      else { --this->m_nowMove; break; }
    }
    /* 如果被将军就搜索所有走法，否则只搜索吃子走法 */
    if (this->m_notInCheck) return INVALID_MOVE;
    // 如果没有了就下一步
    [[fallthrough]];

  case PHASE_NOT_CAPTURE_GEN:
    // 指明下一个阶段
    this->m_phase = PHASE_REST;
    // 生成非吃子的走法并使用历史表对其进行排序
    this->m_totalMoves += this->m_chessboard.genNonCapMoves(
        this->m_moveList + this->m_totalMoves);
    std::sort(this->m_moveList + this->m_nowMove, this->m_moveList + this->m_totalMoves);
    // 直接下一步
    [[fallthrough]];

  case PHASE_REST:
    // 遍历走法，逐个返回
    while (this->m_nowMove < this->m_totalMoves) return this->m_moveList[this->m_nowMove++];
    // 如果没有了就直接返回
    [[fallthrough]];

  default: return INVALID_MOVE;
  }
}
}
