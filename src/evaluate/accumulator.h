#pragma once

#include "model.h"

namespace PikaChess {
/** 累加器，记录着特征转换层的输出值 */
struct Accumulator {
  /** 双方将的位置 */
  quint8 kingPos[8];
  /** 特征转换层的输出，这些值将被输入到全连接层1 */
  alignas(CACHE_LINE_SIZE) qint16 accumulation[8][512];
  /** PSQT部分的输出值，这些值将直接用于评分 */
  alignas(CACHE_LINE_SIZE) qint32 psqtAccumulation[8][PSQT_BUCKETS];

  void copyFrom(const Accumulator &acc) {
    memmove(accumulation[RED], acc.accumulation[RED], sizeof(accumulation[RED]));
    memmove(accumulation[BLACK], acc.accumulation[BLACK], sizeof(accumulation[BLACK]));
    memmove(psqtAccumulation[RED], acc.psqtAccumulation[RED], sizeof(psqtAccumulation[RED]));
    memmove(psqtAccumulation[BLACK], acc.psqtAccumulation[BLACK],
            sizeof(psqtAccumulation[BLACK]));
    kingPos[RED] = acc.kingPos[RED];
    kingPos[BLACK] = acc.kingPos[BLACK];
  }
};
}
