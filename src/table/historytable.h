#pragma once
#include "global.h"
#include "move.h"

namespace PikaChess {
class HistoryTable {
public:
  /**
   * @brief 返回一个走法的历史分值
   * @param move 一个走法
   * @return 该走法的历史分值
   */
  quint64 getValue(const Move &move) const;

  /**
   * @brief 更新一个走法的历史表值
   * @param move 需要更新历史分值的走法
   * @param depth 当前的深度
   */
  void updateValue(const Move &move, quint8 depth);

private:
  quint64 m_historyValues[14][90] { };
};
}
