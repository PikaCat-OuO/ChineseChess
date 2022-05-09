#include "historytable.h"

namespace PikaChess {
quint64 HistoryTable::getValue(const Move &move) const {
  return this->m_historyValues[move.chess()][move.to()];
}

void HistoryTable::updateValue(const Move &move, quint8 depth) {
  this->m_historyValues[move.chess()][move.to()] += depth * depth;
}
}
