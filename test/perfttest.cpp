#include "perfttest.h"

// 以下所有的测试用例来自https://www.chessprogramming.org/Chinese_Chess_Perft_Results
namespace PikaChess {
void PerftTest::position1() {
  runPerftTest("rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w",
               { { 1, 44 }, { 2, 1920 }, { 3, 79666 }, { 4, 3290240 }, { 5, 133312995 } });
}

void PerftTest::position2() {
  runPerftTest("r1ba1a3/4kn3/2n1b4/pNp1p1p1p/4c4/6P2/P1P2R2P/1CcC5/9/2BAKAB2 w",
               { { 1, 38 }, { 2, 1128 }, { 3, 43929 }, { 4, 1339047 }, { 5, 53112976 } });
}

void PerftTest::position3() {
  runPerftTest("1cbak4/9/n2a5/2p1p3p/5cp2/2n2N3/6PCP/3AB4/2C6/3A1K1N1 w",
               { { 1, 7 }, { 2, 281 }, { 3, 8620 }, { 4, 326201 }, { 5, 10369923 } });
}

void PerftTest::position4() {
  runPerftTest("5a3/3k5/3aR4/9/5r3/5n3/9/3A1A3/5K3/2BC2B2 w",
               { { 1, 25 }, { 2, 424 }, { 3, 9850 },
                { 4, 202884 }, { 5, 4739553 }, { 6, 100055401 } });
}

void PerftTest::position5() {
  runPerftTest("CRN1k1b2/3ca4/4ba3/9/2nr5/9/9/4B4/4A4/4KA3 w",
               { { 1, 28 }, { 2, 516 }, { 3, 14808 }, { 4, 395483 }, { 5, 11842230 } });
}

void PerftTest::position6() {
  runPerftTest("R1N1k1b2/9/3aba3/9/2nr5/2B6/9/4B4/4A4/4KA3 w",
               { { 1, 21 }, { 2, 364 }, { 3, 7626 },
                { 4, 162837 }, { 5, 3500505 }, { 6, 81195154 } });
}

void PerftTest::position7() {
  runPerftTest("C1nNk4/9/9/9/9/9/n1pp5/B3C4/9/3A1K3 w",
               { { 1, 28 }, { 2, 222 }, { 3, 6241 },
                { 4, 64971 }, { 5, 1914306 }, { 6, 23496493 } });
}

void PerftTest::position8() {
  runPerftTest("4ka3/4a4/9/9/4N4/p8/9/4C3c/7n1/2BK5 w",
               { { 1, 23 }, { 2, 345 }, { 3, 8124 },
                { 4, 149272 }, { 5, 3513104 }, { 6, 71287903 } });
}

void PerftTest::position9() {
  runPerftTest("2b1ka3/9/b3N4/4n4/9/9/9/4C4/2p6/2BK5 w",
               { { 1, 21 }, { 2, 195 }, { 3, 3883 },
                { 4, 48060 }, { 5, 933096 }, { 6, 12250386 } });
}

void PerftTest::position10() {
  runPerftTest("1C2ka3/9/C1Nab1n2/p3p3p/6p2/9/P3P3P/3AB4/3p2c2/c1BAK4 w",
               { { 1, 30 }, { 2, 830 }, { 3, 22787 }, { 4, 649866 }, { 5, 17920736 }});
}

void PerftTest::position11() {
  runPerftTest("CnN1k1b2/c3a4/4ba3/9/2nr5/9/9/4C4/4A4/4KA3 w",
               { { 1, 19 }, { 2, 583 }, { 3, 11714 }, { 4, 376467 }, { 5, 8148177 }});
}

void PerftTest::runPerftTest(const QString &fen, const TestCases &testCases) {
  this->m_chessboard.parseFen(fen);
  for (const auto &[depth, nodes] : testCases) {
    this->m_nodes = 0;
    perft(depth);
    QVERIFY2(this->m_nodes == nodes,
             QString { "😣第%1层的节点数应该为%2，但实际测试为%3" }.arg(depth)
                 .arg(nodes).arg(this->m_nodes).toStdString().c_str());
  }
}

void PerftTest::perft(quint8 depth) {
  if (depth == 0) { ++this->m_nodes; return; }
  Move move;
  SearchMachine search { this->m_chessboard, INVALID_MOVE, INVALID_MOVE, INVALID_MOVE };
  while ((move = search.getNextMove()).isVaild()) {
    if (this->m_chessboard.makeMove(move)) {
      perft(depth - 1);
      this->m_chessboard.unMakeMove();
    }
  }
}
}

QTEST_MAIN(PikaChess::PerftTest)
