#include "historymove.h"

namespace PikaChess {
quint64 HistoryMove::zobrist() const { return this->m_zobrist; }

bool HistoryMove::isChecked() const { return this->m_flag & 0x4000; }

bool HistoryMove::isNullMove() const { return this->m_flag & 0x8000; }

quint16 HistoryMove::getFlag() const { return this->m_flag; }

void HistoryMove::setChase(quint16 chessFlag) { this->m_flag = chessFlag; }

void HistoryMove::setChecked() { this->m_flag = 0x4000; }

void HistoryMove::setNullMove() { this->m_flag = 0x8000; }

void HistoryMove::setZobrist(quint64 zobrist) { this->m_zobrist = zobrist; }

void HistoryMove::setMove(const Move &move) {
  this->m_chess = move.chess();
  this->m_victim = move.victim();
  this->m_fromTo = move.fromTo();
}
}
