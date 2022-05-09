#pragma once
#include "move.h"
#include "bitboard.h"

namespace PikaChess {
class HistoryMove : public Move {
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
   * @brief 获取该步的flag
   * @return 该步的flag
   */
  quint16 getFlag() const;

  /**
   * @brief 设置该步是否为捉子步
   * @param 被捉的棋子
   */
  void setChase(quint16 chessFlag);

  /**
   * @brief 设置该步为将军步
   */
  void setChecked();

  /**
   * @brief 设置该步为空步
   */
  void setNullMove();

  /**
   * @brief 设置该步的Zobrist值
   * @param zobrist 走该步之前的Zobrist值
   */
  void setZobrist(quint64 zobrist);

  /** 设置一个走法 */
  void setMove(const Move &move);

private:
  /** 走该步之前的Zobrist值 */
  quint64 m_zobrist;

  /** 该步的标志信息，第16位表示是否是空步，第15位表示是否为将军，后面14位表示捉的棋子 */
  quint16 m_flag { 0x8000 };
};
}
