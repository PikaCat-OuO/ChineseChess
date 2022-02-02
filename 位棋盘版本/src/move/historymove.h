#pragma once
#include "move.h"

namespace PikaChess {
class HistoryMove : public Move{
public:
  quint64 zobrist() const;

  /**
   * @brief 该步是否为空步
   */
  bool isNullMove() const;


  /**
   * @brief 该步是否为将军步
   */
  bool isChecked() const;

  /**
   * @brief 设置该步是否为将军步
   * @param checked 该步是否为将军步
   */
  void setCheck(bool checked);

  /**
   * @brief 设置该步的Zobrist值
   * @param zobrist 走该步之前的Zobrist值
   */
  void setZobrist(quint64 zobrist);

  /**
   * @brief 设置该步是否是空步
   * @param 是否为空步
   */
  void setNullMove(bool isNullMove);

  /** 设置一个走法 */
  void setMove(const Move &move);

private:
  /** 走该步之前的Zobrist值 */
  quint64 m_zobrist;

  /** 该步是否为将军步 */
  bool m_isChecked { false };

  /** 该步是否为空步 */
  bool m_isNullMove { false };
};
}
