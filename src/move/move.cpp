#include "move.h"

namespace PikaChess {
Move INVALID_MOVE;

quint8 Move::chess() const { return this->m_chess; }

quint8 Move::victim() const { return this->m_victim; }

quint8 Move::from() const { return this->m_from; }

quint8 Move::to() const { return this->m_to; }

quint16 Move::fromTo() const { return this->m_fromTo; }

bool Move::isVaild() const { return this->m_fromTo; }

bool Move::isCapture() const { return this->m_victim not_eq EMPTY; }

void Move::setChess(quint8 chess) { this->m_chess = chess; }

void Move::setVictim(quint8 victim) { this->m_victim = victim; }

void Move::setMove(const quint8 from, const quint8 to) {
  this->m_from = from;
  this->m_to = to;
}

void Move::setMove(const quint8 chess, const quint8 victim, const quint8 from, const quint8 to) {
  this->m_chess = chess;
  this->m_victim = victim;
  this->m_from = from;
  this->m_to = to;
}

bool operator==(const Move &lhs, const Move &rhs) { return lhs.m_fromTo == rhs.m_fromTo; }

bool operator!=(const Move &lhs, const Move &rhs) { return lhs.m_fromTo not_eq rhs.m_fromTo; }
}
