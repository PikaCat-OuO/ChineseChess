#pragma once
#include "global.h"

namespace PikaChess {
/** 神经网络的输入层 <这层网络的输出张量维度1 x OutDims> */
template <quint32 OutDims>
class Input {
 public:
  /** 输出类型 */
  using OutputType = quint8;

  /** 输出维度 */
  static constexpr quint32 OutputDimensions = OutDims;

  /** 输入层不需要缓冲区 */
  static constexpr quint32 BufferSize = 0;

  /** NNUE网络文件中嵌入的哈希值 */
  static constexpr quint32 getHashValue() {
    quint32 hashValue = 0xEC42E90Du;
    hashValue ^= OutputDimensions;
    return hashValue;
  }

  /** 读取网络的权重，输入层没有权重，直接返回 */
  bool readParameters(std::istream&) { return true; }

  /** 前向传播，直接将输入特征返回即可 */
  const OutputType *propagate(const quint8 *transformedFeatures, char*) const {
    return transformedFeatures;
  }
};
}
