#pragma once
#include "chessboard.h"

namespace PikaChess {
class SearchQuiescenceMachine final {
public:
  /**
   * @brief 静态搜索状态机的构造函数
   * @param chessboard 当前的棋盘
   */
  SearchQuiescenceMachine(const Chessboard &chessboard);

  /** 返回下一个走法 */
  Move getNextMove();

private:
  /** 当前的棋盘 */
  const Chessboard &m_chessboard;

  /** 状态机当前的状态 */
  quint8 m_phase { PHASE_CAPTURE_GEN };

  /** 现在正在遍历第几个走法 */
  quint8 m_nowMove { 0 };
  /** 总共有几个走法 */
  quint8 m_totalMoves;
  /** 所有的走法 */
  ValuedMove m_moveList[111];
};
}
