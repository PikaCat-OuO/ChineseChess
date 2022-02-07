#include "chessengine.h"
#include "searchinstance.h"

namespace PikaChess {
ChessEngine::ChessEngine() { reset(); }

void ChessEngine::reset() {
  // 初始局面
  this->m_chessboard.parseFen("rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w");
}

void ChessEngine::search() {
  // 重置信息
  this->m_hashTable.reset();
  // 迭代加深，重置深度
  this->m_currentDepth = 1;

  // 多线程搜索，每个线程一个搜索实例，只有置换表是共享的
  QVector<SearchInstance> searchInstances;
  // 初始化线程局面
  for (quint8 threadID { 0 }; threadID < std::thread::hardware_concurrency() / 2; ++threadID) {
    searchInstances.emplaceBack(this->m_chessboard, this->m_hashTable);
  }

  // 时间控制
  auto startTimeStamp = clock();

  forever {
    // 多线程搜索
#pragma omp parallel for
    for (auto &searchInstance : searchInstances) {
      searchInstance.searchRoot(this->m_currentDepth);
    }

    // 随便取出一个最好的分数查看一下
    this->m_bestScore = searchInstances.front().bestScore();
    this->m_bestMove = searchInstances.front().bestMove();

    // 如果赢了或者输了或者产生了长将局面或者超过时间就停止搜索就不用再往下搜索了
    if (this->m_bestScore < LOST_SCORE or this->m_bestScore > WIN_SCORE or
        clock() - startTimeStamp > this->m_searchTime) {
      // 从所有的线程中选出分数最高的那一个走法来走
      for (const auto &searchInstance : searchInstances) {
        qint16 threadScore { searchInstance.bestScore() };
        if (this->m_bestScore < threadScore) {
          this->m_bestScore = threadScore;
          this->m_bestMove = searchInstance.bestMove();
        }
      }
      // 走棋
      this->m_chessboard.makeMove(this->m_bestMove);
      break;
    }
    // 增加层数
    ++this->m_currentDepth;
  }
}

quint8 ChessEngine::currentDepth() const { return this->m_currentDepth; }

qint16 ChessEngine::bestScore() const { return this->m_bestScore; }

Move ChessEngine::bestMove() const { return this->m_bestMove; }

void ChessEngine::setSearchTime(clock_t searchTime) { this->m_searchTime = searchTime; }

bool ChessEngine::makeMove(Move &move) {
  if (not this->m_chessboard.isLegalMove(move)) return false;
  else return this->m_chessboard.makeMove(move);
}

void ChessEngine::unMakeMove() { this->m_chessboard.unMakeMove(); }

std::optional<qint16> ChessEngine::getRepeatScore() {
  return this->m_chessboard.getRepeatScore(0);
}

void ChessEngine::setSide(quint8 side) { this->m_chessboard.setSide(side); }

quint8 ChessEngine::side() const { return this->m_chessboard.side(); }

QString ChessEngine::fen() const { return this->m_chessboard.getFen(); }
}
