#include "searchinstance.h"

namespace PikaChess {
SearchInstance::SearchInstance(const Chessboard &chessboard, HashTable &hashTable)
    : m_chessboard { chessboard }, m_hashTable { hashTable } { }

void SearchInstance::searchRoot(const qint8 depth) {
  // 当前走法
  Move move;
  // 搜索有限状态机
  SearchMachine search { this->m_chessboard, this->m_bestMove,
                       this->m_killerTable.getKiller(this->m_distance, 0),
                       this->m_killerTable.getKiller(this->m_distance, 1)};

  qint16 bestScore { LOSS_SCORE };
  // LMR的计数器
  quint8 moveSearched { 0 };
  this->m_legalMove = 0;
  // 是否被对方将军
  bool notInCheck { not this->m_chessboard.getLastMove().isChecked() };

  // 遍历所有走法
  while ((move = search.getNextMove()).isVaild()) {
    // 如果被将军了就不搜索这一步
    if (makeMove(move)) {
      // 不然就获得评分并更新最好的分数
      qint16 tryScore;
      const HistoryMove &lastMove { this->m_chessboard.getLastMove() };
      // 将军延伸，如果将军了对方就多搜几步
      qint8 newDepth = lastMove.isChecked() ? depth : depth - 1;
      // PVS
      if (moveSearched == 0) {
        tryScore = -searchFull(LOSS_SCORE, MATE_SCORE, newDepth, NO_NULL);
      } else {
        // LMR，在层数大于等于3时使用，要求没有被将军，没有将军别人，该步不是吃子步
        if (depth >= 3 and notInCheck and newDepth not_eq depth and not lastMove.isCapture()) {
          qint8 reduce = 1;
          // 靠后的走法采用更加激进的裁剪策略
          if (moveSearched >= 6) reduce = depth / 3;
          tryScore = -searchFull(-bestScore - 1, -bestScore, newDepth - reduce);
        }
        // 其余情况保证搜索正常进行
        else tryScore = bestScore + 1;
        if (tryScore > bestScore) {
          tryScore = -searchFull(-bestScore - 1, -bestScore, newDepth);
          if (tryScore > bestScore) {
            tryScore = -searchFull(LOSS_SCORE, -bestScore, newDepth, NO_NULL);
          }
        }
      }
      // 撤销走棋
      unMakeMove();
      // 如果没有被杀棋，就认为这一步是合理的走法
      if (tryScore > BAN_SCORE_LOSS) ++this->m_legalMove;
      if (tryScore > bestScore) {
        // 找到最佳走法
        bestScore = tryScore;
        this->m_bestMove = move;
      }
      // 搜索了一步棋
      ++moveSearched;
    }
  }
  // 记录到置换表
  recordHash(HASH_PV, bestScore, depth, this->m_bestMove);
  // 如果不是吃子着法，就保存到历史表和杀手着法表
  if (not this->m_bestMove.isCapture()) setBestMove(this->m_bestMove, depth);
  this->m_bestScore = bestScore;
}

qint16 SearchInstance::searchFull(qint16 alpha, const qint16 beta,
                                  const qint8 depth, const bool nullOk) {
  // 清空内部迭代加深走法
  this->m_iidMove = INVALID_MOVE;

  // 达到深度就返回静态评价，由于空着裁剪，深度可能小于-1
  if (depth <= 0) return searchQuiescence(alpha, beta);

  // 杀棋步数裁剪
  qint16 tryScore = LOSS_SCORE + this->m_distance;
  if (tryScore >= beta) return tryScore;

  // 先检查重复局面，获得重复局面标志
  auto repeatScore { this->m_chessboard.getRepeatScore(this->m_distance) };
  // 如果有重复的情况，直接返回分数
  if (repeatScore.has_value()) return repeatScore.value();

  // 获得静态评分
  qint16 staticEval { this->m_chessboard.score() };

  // 不是PV节点
  bool notPVNode { beta - alpha <= 1 };

  bool notInCheck { not this->m_chessboard.getLastMove().isChecked() };

  // 静态评分裁剪
  if (depth < 3 and notPVNode and notInCheck and beta - 1 > BAN_SCORE_LOSS) {
    // 裁剪的边界
    int evalMargin = 40 * depth;

    // 如果放弃一定的分值还是超出边界就返回
    if (staticEval - evalMargin >= beta) return staticEval - evalMargin;
  }

  // 当前走法
  Move move;
  // 尝试置换表裁剪，并得到置换表走法
  tryScore = probeHash(alpha, beta, depth, move);
  // 置换表裁剪成功
  if (tryScore > LOSS_SCORE) return tryScore;

  /* 进行空步裁剪，不能连着走两步空步，被将军时不能走空步
     残局走空步，需要进行检验，不然会有特别大的风险
     根节点的Beta值是"MATE_SCORE"，所以不可能发生空步裁剪 */
  if (nullOk and notInCheck) {
    // 走一步空步
    makeNullMove();
    // 获得评分，深度减掉空着裁剪推荐的两层，然后本身走了一步空步，还要再减掉一层
    tryScore = -searchFull(-beta, 1 - beta, depth - 3, NO_NULL);
    // 撤销空步
    unMakeNullMove();
    // 如果足够好就可以发生截断，残局阶段要注意进行校验
    if (tryScore >= beta and (this->m_chessboard.isNotEndgame() or
                              searchFull(beta - 1, beta, depth - 2, NO_NULL) >= beta)) {
      return tryScore;
    }
  }

  // 剃刀裁剪
  if (notPVNode and notInCheck and depth <= 3) {
    // 给静态评价加上第一个边界
    tryScore = staticEval + 40;

    // 如果超出边界
    if (tryScore < beta) {
      // 第一层直接返回评分和静态搜索的最大值
      if (depth == 1) return std::max(tryScore, searchQuiescence(alpha, beta));

      // 其余情况加上第二个边界
      tryScore += 60;

      // 如果还是超出边界
      if (tryScore < beta and depth <= 2) {
        // 获得静态评分
        qint16 newScore { searchQuiescence(alpha, beta) };

        // 如果静态评分也超出边界，返回评分和静态搜索的最大值
        if (newScore < beta) return std::max(tryScore, newScore);
      }
    }
  }

  // 内部迭代加深启发，只在PV节点上使用
  if (beta - alpha > 1 and depth > 2 and move == INVALID_MOVE) {
    tryScore = searchFull(alpha, beta, depth >> 1, NO_NULL);
    if (tryScore <= alpha) tryScore = searchFull(LOSS_SCORE, beta, depth >> 1, NO_NULL);
    move = this->m_iidMove;
  }

  // 搜索有限状态机
  SearchMachine search { this->m_chessboard, move,
                       this->m_killerTable.getKiller(this->m_distance, 0),
                       this->m_killerTable.getKiller(this->m_distance, 1) };

  // 最佳走法的标志
  quint8 bestMoveHashFlag { HASH_ALPHA };
  qint16 bestScore { LOSS_SCORE };
  Move bestMove { };

  // LMR的计数器
  quint8 moveSearched { 0 };
  // 遍历所有走法
  while ((move = search.getNextMove()).isVaild()) {
    // 如果被将军了就不搜索这一步
    if (makeMove(move)) {
      // 不然就获得评分并更新最好的分数
      const HistoryMove &lastMove { this->m_chessboard.getLastMove() };
      // 将军延伸，如果将军了对方就多搜几步
      qint8 newDepth = lastMove.isChecked() ? depth : depth - 1;
      // PVS
      if (moveSearched == 0) tryScore = -searchFull(-beta, -alpha, newDepth);
      else {
        // LMR，在层数大于等于3时使用，要求没有被将军，没有将军别人，该步不是吃子步
        if (depth >= 3 and notInCheck and newDepth not_eq depth and not lastMove.isCapture()) {
          qint8 reduce = 1;
          if (moveSearched >= 6) reduce = depth / 3;
          tryScore = -searchFull(-alpha - 1, -alpha, newDepth - reduce);
        }
        // 其余情况保证搜索正常进行
        else tryScore = alpha + 1;
        if (tryScore > alpha) {
          tryScore = -searchFull(-alpha - 1, -alpha, newDepth);
          if (tryScore > alpha and tryScore < beta) {
            tryScore = -searchFull(-beta, -alpha, newDepth);
          }
        }
      }
      // 撤销走棋
      unMakeMove();
      if (tryScore > bestScore) {
        // 找到最佳走法(但不能确定是Alpha、PV还是Beta走法)
        bestScore = tryScore;
        // 找到一个Beta走法
        if (tryScore >= beta) {
          // 更新走法标志
          bestMoveHashFlag = HASH_BETA;
          // Beta走法要保存到历史表
          bestMove = move;
          // Beta截断
          break;
        }
        // 找到一个PV走法
        if (tryScore > alpha) {
          // 更新走法标志
          bestMoveHashFlag = HASH_PV;
          // PV走法要保存到历史表
          bestMove = move;
          // 缩小Alpha-Beta边界
          alpha = tryScore;
        }
      }
      // 搜索了一步棋
      ++moveSearched;
    }
  }

  // 提供给内部迭代加深使用
  this->m_iidMove = bestMove;

  // 所有走法都搜索完了，把最佳走法(不能是Alpha走法)保存到历史表，返回最佳值。如果是杀棋，就根据杀棋步数给出评价
  if (bestScore == LOSS_SCORE) return LOSS_SCORE + this->m_distance;

  // 记录到置换表
  recordHash(bestMoveHashFlag, bestScore, depth, bestMove);
  // 如果不是Alpha走法，并且不是吃子走法，就将最佳走法保存到历史表、杀手表
  if (bestMove.isVaild() and not bestMove.isCapture()) setBestMove(bestMove, depth);

  return bestScore;
}

qint16 SearchInstance::searchQuiescence(qint16 alpha, const qint16 beta) {
  // 杀棋步数裁剪
  qint16 tryScore = LOSS_SCORE + this->m_distance;
  if (tryScore >= beta) return tryScore;

  // 先检查重复局面，获得重复局面标志
  auto repeatScore { this->m_chessboard.getRepeatScore(this->m_distance) };
  // 如果有重复的情况，直接返回分数
  if (repeatScore.has_value()) return repeatScore.value();

  qint16 bestScore { LOSS_SCORE };

  // 如果不被将军，先做局面评价，如果局面评价没有截断，再生成吃子走法
  bool notInCheck { not this->m_chessboard.getLastMove().isChecked() };
  if (notInCheck) {
    tryScore = this->m_chessboard.score();
    if (tryScore > bestScore) {
      bestScore = tryScore;
      // Beta截断
      if (tryScore >= beta) return tryScore;

      // 差值(delta)裁剪，残局阶段不开启，如果吃一个车都无法超过alpha就裁剪
      if (this->m_chessboard.isNotEndgame() and tryScore + 200 < alpha) return alpha;

      // 缩小Alpha-Beta边界
      if (tryScore > alpha) alpha = tryScore;
    }
  }

  // 静态搜索有限状态机
  SearchQuiescenceMachine search { this->m_chessboard, notInCheck };

  // 遍历所有走法
  Move move;
  while ((move = search.getNextMove()).isVaild()) {
    // 如果被将军了就不搜索这一步
    if (makeMove(move)) {
      // 不然就获得评分并更新最好的分数
      tryScore = -searchQuiescence(-beta, -alpha);

      // 撤销走棋
      unMakeMove();
      if (tryScore > bestScore) {
        // 找到最佳走法(但不能确定是Alpha、PV还是Beta走法)
        bestScore = tryScore;
        // 找到一个Beta走法，Beta截断
        if (tryScore >= beta) return tryScore;
        // 找到一个PV走法，缩小Alpha-Beta边界
        if (tryScore > alpha) alpha = tryScore;
      }
    }
  }

  // 所有走法都搜索完了，返回最佳值。如果是杀棋，就根据杀棋步数给出评价
  if (bestScore == LOSS_SCORE) return LOSS_SCORE + this->m_distance;

  // 否则就返回最佳值
  return bestScore;
}

qint16 SearchInstance::bestScore() const { return this->m_bestScore; }

Move SearchInstance::bestMove() const { return this->m_bestMove; }

quint8 SearchInstance::legalMove() const { return this->m_legalMove; }

bool SearchInstance::makeMove(Move &move) {
  bool result { this->m_chessboard.makeMove(move) };
  if (result) ++this->m_distance;
  return result;
}

void SearchInstance::unMakeMove() { this->m_chessboard.unMakeMove(); --this->m_distance; }

void SearchInstance::makeNullMove() { ++this->m_distance; this->m_chessboard.makeNullMove(); }

void SearchInstance::unMakeNullMove() {
  --this->m_distance;
  this->m_chessboard.unMakeNullMove();
}

qint16 SearchInstance::probeHash(qint16 alpha, qint16 beta, qint8 depth, Move &hashMove) {
  return this->m_hashTable.probeHash(this->m_distance, this->m_chessboard.zobrist(),
                                     alpha, beta, depth, hashMove);
}

void SearchInstance::recordHash(quint8 hashFlag, qint16 score, qint8 depth, const Move &move) {
  this->m_hashTable.recordHash(this->m_distance, this->m_chessboard.zobrist(),
                               hashFlag, score, depth, move);
}

void SearchInstance::setBestMove(const Move &move, qint8 depth) {
  this->m_killerTable.updateKiller(this->m_distance, move);
  this->m_chessboard.updateHistoryValue(move, depth);
}
}
