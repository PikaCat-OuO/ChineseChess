#pragma once
#include "killertable.h"
#include "hashtable.h"
#include "searchmachine.h"
#include "searchquiescencemachine.h"
#include "chessboard.h"

namespace PikaChess {
class SearchInstance {
public:
  SearchInstance(const Chessboard &chessboard, HashTable &hashTable);

  /** 根节点的搜索 */
  void searchRoot(const qint8 depth);

  /** 完全局面搜索 */
  qint16 searchFull(qint16 alpha, const qint16 beta, const qint8 depth, const bool nullOk = true);

  /** 静态局面搜索 */
  qint16 searchQuiescence(qint16 alpha, const qint16 beta);

  /** 获得最好的分数 */
  qint16 bestScore() const;

  /** 获得最好的走法 */
  Move bestMove() const;

  /** 当前局面有效的走法数 */
  quint8 legalMove() const;

protected:
  /** 走一步 */
  bool makeMove(Move &move);

  /** 撤销一步 */
  void unMakeMove();

  /** 走一步空步 */
  void makeNullMove();

  /** 撤销一步空步 */
  void unMakeNullMove();

  /** 查找置换表 */
  qint16 probeHash(qint16 alpha, qint16 beta, qint8 depth, Move &hashMove);

  /** 记录到置换表 */
  void recordHash(quint8 hashFlag, qint16 score, qint8 depth, const Move &move);

  /** 设置最好的走法 */
  void setBestMove(const Move &move, qint8 depth);

private:
  /** 距离根节点的距离 */
  quint8 m_distance { 0 };

  /** 当前局面下最好的分数 */
  qint16 m_bestScore;

  /** 当前局面下最好的走法 */
  Move m_bestMove { INVALID_MOVE };

  /** 局面的内部迭代加深走法 */
  Move m_iidMove { INVALID_MOVE };

  /** 当前局面的有效走法数 */
  quint8 m_legalMove;

  /** 每个搜索实例都有自己的棋盘 */
  Chessboard m_chessboard;

  /** 杀手走法表 */
  KillerTable m_killerTable;

  /** 置换表的引用 */
  HashTable &m_hashTable;
};
}
