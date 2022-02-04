#include "historymove.h"

namespace PikaChess {
quint64 HistoryMove::zobrist() const { return this->m_zobrist; }

bool HistoryMove::isChecked() const { return this->m_isChecked; }

bool HistoryMove::isNullMove() const { return this->m_isNullMove; }

void HistoryMove::setCheck(bool checked) { this->m_isChecked = checked; }

void HistoryMove::setZobrist(quint64 zobrist) { this->m_zobrist = zobrist; }

void HistoryMove::setNullMove(bool isNullMove) { this->m_isNullMove = isNullMove; }

void HistoryMove::setMove(const Move &move) {
  this->m_chess = move.chess();
  this->m_victim = move.victim();
  this->m_fromTo = move.fromTo();
}
}
