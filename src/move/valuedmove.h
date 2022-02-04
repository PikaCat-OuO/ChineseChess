#pragma once
#include "move.h"

namespace PikaChess {
class ValuedMove : public Move {
public:
  /**
   * @brief 设置该步的分值
   * @param 该步的分值
   */
  void setScore(qint64 score);

  qint64 score() const;

  /** 比较函数，用于比较两个走法分值的大小 */
  friend bool operator<(const ValuedMove &lhs, const ValuedMove &rhs);

private:
  /** 该步的分值，用于走法排序 */
  qint64 m_score;
};
}
