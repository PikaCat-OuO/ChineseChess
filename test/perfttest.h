#pragma once
#include <QTest>
#include "searchmachine.h"

namespace PikaChess {
/** 测试用例，如第1层44个，第2层1920个，则测试用例为[(1, 44), (2, 1920)] */
using TestCases = QVector<QPair<quint8, quint64>>;

class PerftTest : public QObject {
  Q_OBJECT
private slots:
  void position1();
  void position2();
  void position3();
  void position4();
  void position5();
  void position6();
  void position7();
  void position8();
  void position9();
  void position10();
  void position11();

protected:
  /**
   * @brief 执行perft测试
   * @param testCases 所有的测试数据
   */
  void runPerftTest(const QString &fen, const TestCases &testCases);
  void perft(quint8 depth);

private:
  /** 总共走过的perft节点数 */
  quint64 m_nodes;
  /** 棋盘 */
  Chessboard m_chessboard;
};
}
