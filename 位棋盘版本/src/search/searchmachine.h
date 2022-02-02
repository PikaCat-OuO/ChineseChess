#pragma once
#include "chessboard.h"

namespace PikaChess {
class SearchMachine final {
public:
  /**
   * @brief 搜索状态机的构造函数
   * @param chessboard 当前的棋盘
   * @param hashMove 当前的哈希表走法
   * @param killerMove1 杀手走法1
   * @param killerMove2 杀手走法2
   */
  SearchMachine(const Chessboard &chessboard, const Move &hashMove,
                Move &killerMove1, Move &killerMove2);

  /** 返回下一个走法 */
  Move getNextMove();

protected:
  /** 当前的棋盘 */
  const Chessboard &m_chessboard;

  /** 状态机当前的状态 */
  quint8 m_phase { PHASE_HASH };

  /** 现在正在遍历第几个走法 */
  quint8 m_nowMove { 0 };
  /** 总共有几个走法 */
  quint8 m_totalMoves;
  /** 所有的走法 */
  ValuedMove m_moveList[111];

  /** 置换表走法， */
  const Move &m_hashMove;
  /** 两个杀手走法 */
  Move &m_killerMove1, &m_killerMove2;
};
}
