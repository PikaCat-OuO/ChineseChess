#include "killertable.h"

namespace PikaChess {
Move &KillerTable::getKiller(const quint8 distance, const quint8 i) {
  return this->m_killerMoves[distance][i];
}

void KillerTable::updateKiller(const quint8 distance, const Move &move) {
  if (this->m_killerMoves[distance][0] not_eq move) {
    this->m_killerMoves[distance][1] = this->m_killerMoves[distance][0];
    this->m_killerMoves[distance][0] = move;
  }
}
}
