#pragma once
#include "searchinstance.h"

namespace PikaChess {
class ChessEngine {
public:
  ChessEngine();

  void reset();

  void search();

  /** 获得当前搜索的深度 */
  quint8 currentDepth() const;

  /** 获得当前局面的最好的分数 */
  qint16 bestScore() const;

  /** 获得当前局面的最好的走法 */
  Move bestMove() const;

  /** 设置搜索的时间 */
  void setSearchTime(clock_t searchTime);

  /** 走一步 */
  bool makeMove(Move &move);

  /** 撤销一步 */
  void unMakeMove();

  /** 获得重复局面得分 */
  std::optional<qint16> getRepeatScore();

  void setSide(quint8 side);

  quint8 side() const;

  QString fen() const;

private:
  /** 当前搜索的深度 */
  quint8 m_currentDepth;

  /** 当前局面下最好的分数 */
  qint16 m_bestScore;

  /** 当前局面下最好的走法 */
  Move m_bestMove;

  /** 设计搜索时间 */
  clock_t m_searchTime { 1500 };

  /** 当前的棋盘 */
  Chessboard m_chessboard;

  /** 置换表 */
  HashTable m_hashTable;
};
}
