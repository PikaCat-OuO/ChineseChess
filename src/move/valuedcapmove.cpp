#include "valuedcapmove.h"

namespace PikaChess {
qint8 ValuedCapMove::score() const { return this->m_score; }

void ValuedCapMove::setScore(qint8 score) { this->m_score = score; }

bool operator<(const ValuedCapMove &lhs, const ValuedCapMove &rhs) {
  return lhs.m_score > rhs.m_score;
}
}
