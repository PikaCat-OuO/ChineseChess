#include "searchinstance.h"

namespace PikaChess {
/** 搜索的衰减层数 [第几层][第几个走法] */
quint16 REDUCTIONS[64][128];

/** 延迟走法裁剪的裁剪层数 [第几层] */
quint16 LMP_MOVE_COUNT[64];

SearchInstance::SearchInstance(const Chessboard &chessboard, HashTable &hashTable)
    : m_chessboard { chessboard }, m_hashTable { hashTable } { }

void SearchInstance::searchRoot(const qint8 depth) {
  // 当前走法
  Move move;
  // 搜索有限状态机
  SearchMachine search { this->m_chessboard, this->m_bestMove,
                       this->m_killerTable.getKiller(this->m_distance, 0),
                       this->m_killerTable.getKiller(this->m_distance, 1) };

  qint16 bestScore { LOSS_SCORE };
  // 搜索计数器
  quint8 moveCount { 0 };
  this->m_legalMove = 0;
  // 是否被对方将军
  bool notInCheck { not this->m_chessboard.getLastMove().isChecked() };

  // 遍历所有走法
  while ((move = search.getNextMove()).isVaild()) {
    // 如果被将军了就不搜索这一步
    if (makeMove(move)) {
      ++moveCount;
      // 不然就获得评分并更新最好的分数
      qint16 tryScore;
      const HistoryMove &lastMove { this->m_chessboard.getLastMove() };
      // 将军延伸，如果将军了对方就多搜几步
      qint8 newDepth = lastMove.isChecked() ? depth : depth - 1;
      // PVS
      if (moveCount == 1) {
        tryScore = -searchFull(LOSS_SCORE, MATE_SCORE, newDepth, NO_NULL);
      } else {
        // 对于延迟走法的处理，要求没有被将军，没有将军别人，该步不是吃子步
        if (depth >= 3 and notInCheck and newDepth not_eq depth and not lastMove.isCapture()) {
          tryScore = -searchFull(-bestScore - 1, -bestScore,
                                 newDepth - REDUCTIONS[depth][moveCount]);
        }
        // 如果不满足条件则不衰减层数
        else tryScore = -searchFull(-bestScore - 1, -bestScore, newDepth);
        if (tryScore > bestScore) {
          tryScore = -searchFull(LOSS_SCORE, -bestScore, newDepth, NO_NULL);
        }
      }
      // 撤销走棋
      unMakeMove();
      // 如果没有被杀棋，就认为这一步是合理的走法
      if (tryScore > LOST_SCORE) ++this->m_legalMove;
      if (tryScore > bestScore) {
        // 找到最佳走法
        bestScore = tryScore;
        this->m_bestMove = move;
      }
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

  // 当前走法
  Move move;
  // 尝试置换表裁剪，并得到置换表走法
  tryScore = probeHash(alpha, beta, depth, move);
  // 置换表裁剪成功
  if (tryScore > LOSS_SCORE) return tryScore;

  // 不被将军时可以进行一些裁剪
  bool notInCheck { not this->m_chessboard.getLastMove().isChecked() };
  if (notInCheck) {
    // 适用于非PV节点的前期裁剪
    if (beta - alpha <= 1) {
      // 获得局面的静态评分
      qint16 staticEval { this->m_chessboard.score() };

      // 无用裁剪，如果放弃一定的分值还是超出边界就返回
      if (depth < 9 and staticEval - 214 * (depth - 1) >= beta) return staticEval;

      /* 进行空步裁剪，不能连着走两步空步，被将军时不能走空步，层数较大时，需要进行检验
         根节点的Beta值是"MATE_SCORE"，所以不可能发生空步裁剪 */
      if (nullOk and staticEval >= beta) {
        // 走一步空步
        makeNullMove();
        // 获得评分，深度减掉空着裁剪推荐的两层，然后本身走了一步空步，还要再减掉一层
        tryScore = -searchFull(-beta, 1 - beta, depth - 3, NO_NULL);
        // 空步裁剪发现的胜利需要进一步验证
        if (tryScore >= WIN_SCORE) tryScore = beta;
        // 撤销空步
        unMakeNullMove();

        // 如果足够好就可以发生截断，层数较大时要注意进行校验
        if (tryScore >= beta and ((depth < 14 and abs(beta) < WIN_SCORE) or
                                  searchFull(beta - 1, beta, depth - 2, NO_NULL) >= beta)) {
          return tryScore;
        }
      }
    }

    // 适用于PV节点的内部迭代加深启发
    else if (depth > 2 and move == INVALID_MOVE) {
      tryScore = searchFull(alpha, beta, depth >> 1, NO_NULL);
      if (tryScore <= alpha) tryScore = searchFull(LOSS_SCORE, beta, depth >> 1, NO_NULL);
      move = this->m_iidMove;
    }
  }

  // 搜索有限状态机
  SearchMachine search { this->m_chessboard, move,
                       this->m_killerTable.getKiller(this->m_distance, 0),
                       this->m_killerTable.getKiller(this->m_distance, 1) };

  // 最佳走法的标志
  quint8 bestMoveHashFlag { HASH_ALPHA };
  qint16 bestScore { LOSS_SCORE };
  Move bestMove { INVALID_MOVE };

  // 搜索计数器
  quint8 moveCount { 0 };
  // 遍历所有走法
  while ((move = search.getNextMove()).isVaild()) {
    // 延迟走法裁剪
    if (notInCheck and not move.isCapture() and abs(bestScore) < WIN_SCORE and
        moveCount >= LMP_MOVE_COUNT[depth]) continue;

    // 如果被将军了就不搜索这一步
    if (makeMove(move)) {
      ++moveCount;      
      // 不然就获得评分并更新最好的分数
      const HistoryMove &lastMove { this->m_chessboard.getLastMove() };
      // 将军延伸，如果将军了对方就多搜几步
      qint8 newDepth = lastMove.isChecked() ? depth : depth - 1;

      // PVS，对于延迟走法的处理，要求没有被将军，没有将军别人，该步不是吃子步
      if (depth >= 3 and notInCheck and newDepth not_eq depth and not lastMove.isCapture()) {
        tryScore = -searchFull(-alpha - 1, -alpha,
                               newDepth - REDUCTIONS[depth][moveCount]);
      }
      // 如果不满足条件就不衰减层数
      else tryScore = -searchFull(-alpha - 1, -alpha, newDepth);
      if (tryScore > alpha and tryScore < beta) tryScore = -searchFull(-beta, -alpha, newDepth);

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

  // 差值裁剪的边界值
  qint16 deltaBase { LOSS_SCORE };

  // 如果不被将军，先做局面评价，如果局面评价没有截断，再生成吃子走法
  bool notInCheck { not this->m_chessboard.getLastMove().isChecked() };
  if (notInCheck) {
    bestScore = this->m_chessboard.score();
    // Beta截断
    if (bestScore >= beta) return bestScore;

    // 缩小Alpha-Beta边界
    if (bestScore > alpha) alpha = bestScore;

    // 调整差值裁剪的边界
    deltaBase = bestScore + 155;
  }

  // 静态搜索有限状态机
  SearchQuiescenceMachine search { this->m_chessboard, notInCheck };

  // 遍历所有走法
  Move move;
  while ((move = search.getNextMove()).isVaild()) {
    // 如果被将军了就不搜索这一步
    if (makeMove(move)) {
      // 首先进行差值裁剪，如果加上一定的值都不能超过alpha，就认为这个走法是无用的
      if (notInCheck and not this->m_chessboard.getLastMove().isChecked() and
          deltaBase + PIECE_VALUE[move.victim()] <= alpha) {
        tryScore = deltaBase + PIECE_VALUE[move.victim()];
      }
      // 否则就获得评分并更新最好的分数
      else tryScore = -searchQuiescence(-beta, -alpha);

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
