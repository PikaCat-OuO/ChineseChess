#include "chessengine.h"
#include "searchinstance.h"

namespace PikaChess {
ChessEngine::ChessEngine() {
  // 加载神经网络
  NNUEInit();
  // 初始化棋盘
  reset();
}

void ChessEngine::reset() {
  // 初始局面
  this->m_chessboard.parseFen("rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w");
  // 清理置换表
  this->m_hashTable.clear();
}

void ChessEngine::search() {
  // 重置信息
  this->m_hashTable.reset();
  // 迭代加深，重置深度
  this->m_currentDepth = 1;

  // 多线程搜索，每个线程一个搜索实例，只有置换表是共享的
  QVector<SearchInstance> searchInstances;
  // 初始化线程局面
  for (quint8 threadID { 0 };
       threadID < qMax(quint8(1), std::thread::hardware_concurrency() / 2);
       ++threadID) {
    searchInstances.emplaceBack(this->m_chessboard, this->m_hashTable);
  }

  // 时间控制
  auto startTimeStamp = clock();
  auto timeMan = QtConcurrent::run([&] {
    while (not searchInstances.front().isStopped() and
           clock() - startTimeStamp < this->m_searchTime) {
      std::this_thread::yield();
    }
    for (auto &searchInstance : searchInstances) {
      searchInstance.stopSearch();
    }
  });

  forever {
    // 多线程搜索
#pragma omp parallel for proc_bind(spread)
    for (auto &searchInstance : searchInstances) {
      searchInstance.searchRoot(this->m_currentDepth);
    }

    // 记录所有的线程中最高分数与对应的走法
    if (not searchInstances.front().isStopped()) {
      this->m_bestScore = searchInstances.front().bestScore();
      this->m_bestMove = searchInstances.front().bestMove();
      for (const auto &searchInstance : searchInstances) {
        qint16 threadScore { searchInstance.bestScore() };
        if (this->m_bestScore < threadScore) {
          this->m_bestScore = threadScore;
          this->m_bestMove = searchInstance.bestMove();
        }
      }
    } else {
      // 如果被迫停止，那么这一层没有搜索完成
      --this->m_currentDepth;
    }

    // 如果只有一个合法走法或者超过了最大层数或者超过时间就停止搜索就不用再往下搜索了
    if (searchInstances.front().isStopped() or
        searchInstances.front().legalMove() == 1 or
        this->m_currentDepth >= 99 or
        clock() - startTimeStamp > this->m_searchTime) {
      // 停止时间管理
      searchInstances.front().stopSearch();
      timeMan.waitForFinished();
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
