#pragma once
#include "global.h"

namespace PikaChess {
class Move {
public:
  quint8 from() const;
  quint8 to() const;
  quint16 fromTo() const;

  quint8 chess() const;
  quint8 victim() const;

  /**
   * @brief 该步是否合法
   */
  bool isVaild() const;

  /**
   * @brief 该步是否为吃子走法
   */
  bool isCapture() const;

  void setChess(quint8 chess);

  void setVictim(quint8 victim);

  /**
   *  @brief 设置一个走法
   *  @param from 从哪里来
   *  @param to 到哪里去
   */
  void setMove(const quint8 from, const quint8 to);

  /**
   *  @brief 设置一个走法
   *  @param chess 哪一个棋子
   *  @param victim 吃掉了什么棋子
   *  @param from 从哪里来
   *  @param to 到哪里去
   */
  void setMove(const quint8 chess, const quint8 victim, const quint8 from, const quint8 to);

  /** 比较函数，用于比较两个走法是否相同 */
  friend bool operator==(const Move &lhs, const Move &rhs);

  /** 比较函数，用于比较两个走法是否相同 */
  friend bool operator!=(const Move &lhs, const Move &rhs);

protected:
  /** 哪一个棋子 */
  quint8 m_chess;
  /** 吃掉了什么棋子 */
  quint8 m_victim;

  union {
    struct {
      /** 从哪里来 */
      quint8 m_from;
      /** 到哪里去 */
      quint8 m_to;
    };
    quint16 m_fromTo;
  };
};
/** 默认的无效走法 */
extern Move INVALID_MOVE;
}
