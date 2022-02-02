#include "valuedmove.h"

namespace PikaChess {
qint64 ValuedMove::score() const { return this->m_score; }

void ValuedMove::setScore(qint64 score) { this->m_score = score; }

bool operator<(const ValuedMove &lhs, const ValuedMove &rhs) { return lhs.m_score > rhs.m_score; }
}
