#pragma once
#include "pregen.h"
#include "historymove.h"
#include "valuedmove.h"
#include "historytable.h"

namespace PikaChess {
class Chessboard final {
public:
  Chessboard();

  /** 将一个FEN串转化为棋盘 */
  void parseFen(const QString &fen);
  /** 将一个棋盘上的棋子转化为FEN串 */
  QString getFen() const;

  /**
   *  返回当前局面的吃子走法
   *  @param moveList 生成的走法存放的位置
   *  @return 生成的走法的个数
   */
  quint8 genCapMoves(ValuedMove *moveList) const;
  /**
   *  返回当前局面的不吃子走法
   *  @param moveList 生成的走法存放的位置
   *  @return 生成的走法的个数
   */
  quint8 genNonCapMoves(ValuedMove *moveList) const;

  /** 当前是否被将军 */
  bool isChecked() const;

  /**
   * @brief 判断一个位置是否被保护了
   * @param index 位置
   * @return 这个位置是否被保护了
   */
  bool isProtected(quint8 index) const;

  /**
   * @brief 判断一个走法是否合法
   * @param move
   * @return 这个走法是否合法
   */
  bool isLegalMove(Move &move) const;

  /** 判断当前是否属于残局 */
  bool isNotEndgame() const;

  /**
   * @brief 获得一个局面的重复情况
   * @param distance 距离根节点的深度
   * @return 如果局面重复，返回对应的评分，如果局面不重复，返回std::nullopt
   */
  std::optional<qint16> getRepeatScore(quint8 distance) const;

  /**
   * @brief 在棋盘上走一步
   * @param 一个走法
   * @return 这一步能不能走（如果被将军了就自动回退并返回假）
   */
  bool makeMove(Move &move);

  /** 还原刚才走的步 */
  void unMakeMove();

  /** 走一步空步 */
  void makeNullMove();

  /** 还原一步空步 */
  void unMakeNullMove();

  /**
   * @brief 更新一个走法的历史表值
   * @param move 需要更新历史分值的走法
   * @param depth 当前的深度
   */
  void updateHistoryValue(const Move &move, quint8 depth);

  /** 获得最后一个走法 */
  const HistoryMove &getLastMove() const;

  void setSide(quint8 newSide);

  quint64 zobrist() const;

  qint16 score() const;

  quint8 side() const;

protected:
  /**
   * @brief 撤销一个走法
   * @param move 一个走法
   */
  void undoMove(const Move &move);

private:
  /** 各种棋子的位棋盘 */
  Bitboard m_bitboards[14];
  /** 双方的占位位棋盘 */
  Bitboard m_redOccupancy;
  Bitboard m_blackOccupancy;
  Bitboard m_occupancy;

  /** 当前轮到哪一方走 */
  quint8 m_side;

  /** 当前局面的Zobrist值 */
  quint64 m_zobrist;

  /** 当前局面的红黑方得分 */
  qint16 m_redScore;
  qint16 m_blackScore;

  /** 走棋的历史记录 */
  HistoryMove m_historyMoves[256];
  /** 当前行棋的步数统计，留一个头部作为头部哨兵 */
  quint16 m_historyMovesCount { 1 } ;

  /** 历史分值表，用于给生成的非吃子走法打分 */
  HistoryTable m_historyTable;
};
}
