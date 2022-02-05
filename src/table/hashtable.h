#pragma once
#include "move.h"

namespace PikaChess {
union HashItem {
  struct {
    /** 走该走法前对应的Zobrist，用于校验 */
    quint64 m_zobrist;
    /** 走法 */
    Move m_move;
    /** 该走法对应的搜索分数 */
    qint16 m_score;
    /** 记录该项时所处的深度 */
    qint8 m_depth;
    /** 该走法对应的类型（ALPHA，PV，BETA） */
    quint8 m_hashFlag;
  };
  __m128i m_data;

  /** 构造函数 */
  HashItem() = default;
  /** 复制构造 */
  HashItem(volatile const __m128i &data);
};

class HashTable {
public:
  /**
   * @brief 用于从置换表中获取一个走法
   * @param distance 离根节点的距离
   * @param zobrist 当前局面对应的Zobrist值
   * @param alpha
   * @param beta
   * @param depth 深度
   * @param hashMove 置换表走法返回时存放的位置
   * @return 对应走法的置换表分数
   */
  qint16 probeHash(quint8 distance, quint64 zobrist,
                   qint16 alpha, qint16 beta,
                   qint8 depth, Move &hashMove);

  /**
   * @brief 将一个走法保存到置换表中
   * @param distance 离根节点的距离
   * @param zobrist 当前局面对应的Zobrist值
   * @param hashFlag 该走法对应的类型（ALPHA，PV，BETA）
   * @param score 这个走法对应的搜索分数
   * @param depth 深度信息
   * @param move 走法
   */
  void recordHash(quint8 distance, quint64 zobrist,
                  quint8 hashFlag, qint16 score, qint8 depth, const Move &move);

  /** 重置置换表 */
  void reset();

private:
  __m128i __attribute__((aligned (16))) m_hashTable[HASH_SIZE];
};
}
