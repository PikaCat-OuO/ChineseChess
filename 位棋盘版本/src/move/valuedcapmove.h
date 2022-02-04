#pragma once
#include "move.h"

namespace PikaChess {
class ValuedCapMove : public Move {
public:
  /**
   * @brief 设置该步的分值
   * @param 该步的分值
   */
  void setScore(qint8 score);

  qint8 score() const;

  /** 比较函数，用于比较两个走法分值的大小 */
  friend bool operator<(const ValuedCapMove &lhs, const ValuedCapMove &rhs);

private:
  /** 该步的分值，用于走法排序 */
  qint8 m_score;
};
}
