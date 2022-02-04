#include "hashtable.h"

namespace PikaChess {
HashItem::HashItem(const HashItem &rhs)
    : m_zobrist { rhs.m_zobrist }, m_move { rhs.m_move },
      m_score { rhs.m_score }, m_depth { rhs.m_depth },
      m_hashFlag { rhs.m_hashFlag } { }

qint16 HashTable::probeHash(quint8 distance, quint64 zobrist,
                            qint16 alpha, qint16 beta, qint8 depth, Move &hashMove) {
  // 杀棋的标志，如果杀棋了就不用满足深度条件
  bool isMate { false };

  // 提取置换表项，上锁
  HashItem &hashItemRef = this->m_hashTable[zobrist & HASH_MASK];
  // 上锁并复制置换表项，注意到这里不直接使用上面的引用，因为下面要对分数进行修改
  hashItemRef.m_hashItemLock.lockForRead();
  HashItem hashItem = hashItemRef;
  hashItemRef.m_hashItemLock.unlock();

  // 校验高位zobrist是否对应得上
  if (zobrist not_eq hashItem.m_zobrist) {
    hashMove = INVALID_MOVE;
    return LOSS_SCORE;
  }

  // 将走法保存下来
  hashMove = hashItem.m_move;
  if (hashItem.m_score > WIN_SCORE) {
    // 有可能会导致搜索的不稳定性，因为这个长将的分数可能来自于另外一条不同的搜索分支
    if (hashItem.m_score < BAN_SCORE_MATE) return LOSS_SCORE;
    // 真的赢了
    isMate = true;
    // 给分数添加最短层数信息
    hashItem.m_score -= distance;
  } else if (hashItem.m_score < LOST_SCORE) {
    // 理由同上
    if (hashItem.m_score > BAN_SCORE_LOSS) return LOSS_SCORE;
    // 真的输了
    isMate = true;
    // 给分数添加最短层数信息
    hashItem.m_score += distance;
  }
  // 判断该走法是否满足深度条件，即结果比当前的搜索层数同层或者更深，如果赢了就不用满足深度条件
  if (hashItem.m_depth >= depth or isMate) {
    if (hashItem.m_hashFlag == HASH_BETA) {
      /*
       * 如果是beta走法，说明在同层或高层中搜索该局面走该走法时发生了beta截断
       * 那么查看一下在当前的beta下走该走法是否也可以发生截断，既score >= beta
       * 即是否超出alpha-beta的边界(alpha, beta)(here)
       * 如果摸到或超过当前beta边界即可截断
       * 如果没有超过beta，那么不能直接返回score
       * 因为既然同层或高层发生了beta截断，那么就有可能没有搜索完该局面下的所有走法，分数不一定具备参考性
       * 但是这个走法能直接返回，因为有助于更快速的缩小当前的alpha-beta范围
       */
      if (hashItem.m_score >= beta) return hashItem.m_score;
      else return LOSS_SCORE;
    } else if (hashItem.m_hashFlag == HASH_ALPHA) {
      /*
       * 如果是alpha走法，说明在同层或者上层的搜索中遍历了局面中的所有走法
       * 并且得到的最好的走法是alpha走法，分数无法超过那层的alpha
       * 那么查看一下在当前的alpha下是否也无法超过当前的alpha，既score <= alpha
       * 即是否超出alpha-beta的边界(here)(alpha, beta)
       * 如果摸到或小于当前alpha即可截断
       * 如果大于alpha，那么不能直接返回score
       * 因为如果它在alpha-beta范围内，那么它就不是alpha走法，值得搜索一下
       * 返回该走法有助于更快速的缩小当前的alpha-beta范围
       * 如果它甚至超过了beta，那么这个走法就可以发生beta截断
       */
      if (hashItem.m_score <= alpha) return hashItem.m_score;
      else return LOSS_SCORE;
    }
    /*
     * 在同层或者上层的搜索中遍历了局面中的所有走法，找到了pv走法，所以就是这个分数了！直接返回即可。
     */
    return hashItem.m_score;
  }
  // 不满足深度条件并且不是杀棋
  return LOSS_SCORE;
}

void HashTable::recordHash(quint8 distance, quint64 zobrist,
                           quint8 hashFlag, qint32 score, qint8 depth, const Move &move) {
  // 先上锁
  HashItem &hashItem { this->getHashItem(zobrist) };
  QWriteLocker writeLocker { &hashItem.m_hashItemLock };

  // 查看置换表中的项是否比当前项更加准确
  if (hashItem.m_depth > depth) return;

  // 不然就保存置换表标志和深度
  hashItem.m_hashFlag = hashFlag;
  hashItem.m_depth = depth;
  if (score > WIN_SCORE) {
    // 可能导致搜索的不稳定性，并且没有最佳着法，立刻退出
    if (not move.isVaild() and score <= BAN_SCORE_MATE) return;
    // 否则就记录分数，消除分数的最短层数信息
    hashItem.m_score = score + distance;
  } else if (score < LOST_SCORE) {
    // 同上
    if (not move.isVaild() and score >= BAN_SCORE_LOSS) return;
    hashItem.m_score = score - distance;
  }
  // 不是杀棋时直接记录分数
  else hashItem.m_score = score;

  // 记录走法
  hashItem.m_move = move;

  // 记录Zobrist
  hashItem.m_zobrist = zobrist;
}

HashItem &HashTable::getHashItem(quint64 zobrist) {
  return this->m_hashTable[zobrist & HASH_MASK];
}

void HashTable::reset() {
  for (auto &hashItem : this->m_hashTable) {
    hashItem.m_hashFlag = HASH_ALPHA;
    hashItem.m_depth = 0;
    hashItem.m_move = INVALID_MOVE;
    hashItem.m_score = 0;
    hashItem.m_zobrist = 0;
  }
}
}