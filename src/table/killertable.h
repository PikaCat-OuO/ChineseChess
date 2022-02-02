#pragma once
#include "move.h"

namespace PikaChess {
class KillerTable {
public:
  /**
   * @brief 获得杀手走法
   * @param distance 距离根节点的距离
   * @param i 获取哪一个杀手走法
   * @return 一个杀手走法
   */
  Move &getKiller(const quint8 distance, const quint8 i);

  /**
   * @brief 更新杀手走法
   * @param distance 距离根节点的距离
   * @param move 一个杀手走法
   */
  void updateKiller(const quint8 distance, const Move &move);

private:
  /** 杀手走法表 */
  Move m_killerMoves[256][2] { };
};
}
