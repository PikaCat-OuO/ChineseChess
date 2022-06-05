#pragma once
#include "global.h"

namespace PikaChess {
/** 神经网络的ClippedReLU层，<上一层> */
template <typename PreviousLayer>
class ClippedReLU {
public:
  /** 输入输出的类型 */
  using InputType = typename PreviousLayer::OutputType;
  using OutputType = quint8;

  /** 输入输出的张量维度，因为是relu所以输入输出维度相同 */
  static constexpr quint32 InputDimensions = PreviousLayer::OutputDimensions;
  static constexpr quint32 OutputDimensions = InputDimensions;

  /** 本层网络需要使用到的缓冲区大小，对其到CPU的缓冲块，以字节为单位 */
  static constexpr quint32 SelfBufferSize =
      CeilToMultiple<quint32>(OutputDimensions * sizeof(OutputType), CACHE_LINE_SIZE);

  /** 本层网络所在的缓冲区的总大小，包括前面网络的缓冲区和本层网络所需的缓冲区大小 */
  static constexpr quint32 BufferSize = PreviousLayer::BufferSize + SelfBufferSize;

  /** NNUE网络文件中嵌入的哈希值 */
  static constexpr std::uint32_t getHashValue() {
    quint32 hashValue = 0x538D24C7u;
    hashValue += PreviousLayer::getHashValue();
    return hashValue;
  }

  /** 将网络的权重和偏差从文件中读取到内存中，直接调用上一层的读取操作即可，本层没有权重和偏差 */
  bool readParameters(std::istream& stream) { return previousLayer.readParameters(stream); }

  /** 前向传播函数 */
  const OutputType* propagate(const quint8 *transformedFeatures, char* buffer) const {
    // 首先调用上一层的传播函数得到本层的输入
    const auto input = previousLayer.propagate(transformedFeatures, buffer + SelfBufferSize);
    // 输出指针
    const auto output = reinterpret_cast<OutputType*>(buffer);

    // 如果输入正好是SIMD_WIDTH的倍数，说明输入维度是32，上__m256i
    if constexpr (InputDimensions % SIMD_WIDTH == 0) {
      // 用于确定下限
      const __m256i Zero = _mm256_setzero_si256();
      // 用于重排序
      const __m256i Offsets = _mm256_set_epi32(7, 3, 6, 2, 5, 1, 4, 0);

      // 以__m256i为单位的输入输出指针
      const auto in = reinterpret_cast<const __m256i*>(input);
      const auto out = reinterpret_cast<__m256i*>(output);

      // 一次操作两个__m256i，经历32->16->16(在这里给予一定的右位移)
      const __m256i words0 = _mm256_srai_epi16(_mm256_packs_epi32(
                                                   _mm256_load_si256(&in[0]),
                                                   _mm256_load_si256(&in[1])),
                                               WEIGHTS_SCALE_BITS);
      const __m256i words1 = _mm256_srai_epi16(_mm256_packs_epi32(
                                                   _mm256_load_si256(&in[2]),
                                                   _mm256_load_si256(&in[3])),
                                               WEIGHTS_SCALE_BITS);

      // 将上面得到的结果进行上下钳位，16->8->clamp(0, 127)，因为这一系列操作会打乱顺序，所以最后进行重排序
      _mm256_store_si256(out, _mm256_permutevar8x32_epi32(
                                  _mm256_max_epi8(_mm256_packs_epi16(words0, words1),
                                                  Zero), Offsets));
    }
    // 其他情况说明输入维度是16，上__m128i，具体步骤同上，只是不会打乱，不需要重排序
    else {
      const __m128i Zero = _mm_setzero_si128();
      const auto in = reinterpret_cast<const __m128i*>(input);
      const auto out = reinterpret_cast<__m128i*>(output);
      const __m128i words0 = _mm_srai_epi16(_mm_packs_epi32(
                                                _mm_load_si128(&in[0]),
                                                _mm_load_si128(&in[1])),
                                            WEIGHTS_SCALE_BITS);
      const __m128i words1 = _mm_srai_epi16(_mm_packs_epi32(
                                                _mm_load_si128(&in[2]),
                                                _mm_load_si128(&in[3])),
                                            WEIGHTS_SCALE_BITS);
      _mm_store_si128(out, _mm_max_epi8(_mm_packs_epi16(words0, words1), Zero));
    }

    return output;
  }

private:
  /** 上一层 */
  PreviousLayer previousLayer;
};
}
