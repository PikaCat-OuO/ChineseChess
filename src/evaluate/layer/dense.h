#pragma once
#include "global.h"

namespace PikaChess {
/** 神经网络的Dense层 <上一层网络，这层网络的输出张量维度1 x OutDims> */
template <typename PreviousLayer, quint32 OutDims>
class Dense {
public:
  /** 输入张量数据类型，输出张量数据类型 */
  using InputType = typename PreviousLayer::OutputType;
  using OutputType = qint32;

  /** 输入张量维度，也就是上一层网络的输出张量维度 */
  static constexpr quint32 InputDimensions = PreviousLayer::OutputDimensions;
  /** 输入张量维度，对齐到SIMD宽度 */
  static constexpr quint32 PaddedInputDimensions =
      CeilToMultiple<quint32>(InputDimensions, MAX_SIMD_WIDTH);
  /** 输出张量维度 */
  static constexpr quint32 OutputDimensions = OutDims;
  /** 使用SIMD指令集，每一次可以处理多少个输出元素
   *  因为使用的是AVX2(__m256i)，输出类型的qint32，所以一次最多处理32 / 4 = 8个输出元素 */
  static constexpr quint32 OutputSIMDWidth = SIMD_WIDTH / 4;

  /** 本层网络需要使用到的缓冲区大小，对其到CPU的缓冲单元，以字节为单位 */
  static constexpr quint64 SelfBufferSize =
      CeilToMultiple<quint32>(OutputDimensions * sizeof(OutputType), CACHE_LINE_SIZE);

  /** 本层网络所在的缓冲区的总大小，包括前面网络的缓冲区和本层网络所需的缓冲区大小 */
  static constexpr quint64 BufferSize = PreviousLayer::BufferSize + SelfBufferSize;

  /** 用于_mm256_madd_epi16的固定乘法因子 */
  static inline const __m256i Ones256 = _mm256_set1_epi16(1);

  /** NNUE网络文件中嵌入的哈希值 */
  static constexpr quint32 getHashValue() {
    quint32 hashValue = 0xCC03DAE4u;
    hashValue += OutputDimensions;
    hashValue ^= PreviousLayer::getHashValue() >> 1;
    hashValue ^= PreviousLayer::getHashValue() << 31;
    return hashValue;
  }

  /** 将网络的权重和偏差从文件中读取到内存中 */
  bool readParameters(std::istream& stream) {
    // 首先递归调用上一层网络的读取函数
    if (!previousLayer.readParameters(stream)) return false;
    // 接着读取偏差，大小为输出维度
    for (quint64 i = 0; i < OutputDimensions; ++i) {
      biases[i] = ReadInt<BiasType>(stream);
    }
    // 接着读取权重，因为后面需要作SIMD处理，所以这里需要将权重以四个为单位将列转换为行，读者可以自行打印查看
    for (quint64 i = 0; i < OutputDimensions * PaddedInputDimensions; ++i) {
      weights[
          (i / 4) % (PaddedInputDimensions / 4) * OutputDimensions * 4 +
          i / PaddedInputDimensions * 4 +
          i % 4
      ] = ReadInt<WeightType>(stream);
    }
    return !stream.fail();
  }

  /** 前向传播函数 */
  const OutputType *propagate(const quint8* transformedFeatures, char *buffer) const {
    // 首先调用上一层的传播函数得到本层的输入
    const auto input = previousLayer.propagate(transformedFeatures, buffer + SelfBufferSize);

    // 输出数组指针
    const auto output = (OutputType*)(buffer);

    /* 输出维度只能处理元素的整数倍，或者是1（最后一层），因为输出维度其实也是输入维度，是对其到MAX_SIMD_WIDTH的
     * 这里使用constexpr if留给编译器根据不同的情况进行优化*/
    if constexpr (OutputDimensions % OutputSIMDWidth == 0) {
      // 一次处理4列数据，有多少列需要处理
      constexpr qint32 NumChunks = InputDimensions / 4;

      // 将输出转换为32位指针的形式，以供下面_mm256_set1_epi32读取使用，因为我们一次性处理4列
      const auto input32 = (const qint32*)(input);
      // 输出指针
      __m256i *outptr = (__m256i*)(output);
      // 首先将偏差复制到output中
      std::memmove(output, biases, OutputDimensions * sizeof(OutputType));

      /* 每次处理4个4列，最后一个4列的坐标是NumChunks(从1开始计算)
       * 所以NumChunks - 3就是最后一个4列的第1列
       * 因为我们是从0开始的，所以也就是最后一个4列的第2列，刚好满足i的最后一组条件 */
      for (qint32 i = 0; i < NumChunks - 3; i += 4) {
        const __m256i in0 = _mm256_set1_epi32(input32[i + 0]);
        const __m256i in1 = _mm256_set1_epi32(input32[i + 1]);
        const __m256i in2 = _mm256_set1_epi32(input32[i + 2]);
        const __m256i in3 = _mm256_set1_epi32(input32[i + 3]);
        const auto col0 = (const __m256i*)(&weights[(i + 0) * OutputDimensions * 4]);
        const auto col1 = (const __m256i*)(&weights[(i + 1) * OutputDimensions * 4]);
        const auto col2 = (const __m256i*)(&weights[(i + 2) * OutputDimensions * 4]);
        const auto col3 = (const __m256i*)(&weights[(i + 3) * OutputDimensions * 4]);
        // // 以4个8位为单位对应位置相乘相加，一次操作四个__m256i
        for (qint32 j = 0; j * OutputSIMDWidth < OutputDimensions; ++j) {
          __m256i product0 = _mm256_maddubs_epi16(in0, col0[j]);
          __m256i product1 = _mm256_maddubs_epi16(in1, col1[j]);
          __m256i product2 = _mm256_maddubs_epi16(in2, col2[j]);
          __m256i product3 = _mm256_maddubs_epi16(in3, col3[j]);
          product0 = _mm256_adds_epi16(product0, product1);
          product0 = _mm256_madd_epi16(product0, Ones256);
          product2 = _mm256_adds_epi16(product2, product3);
          product2 = _mm256_madd_epi16(product2, Ones256);
          outptr[j] = _mm256_add_epi32(outptr[j], _mm256_add_epi32(product0, product2));
        }
      }
    }
    // 如果输出维度只有1，就不需要额外的处理了，直接将输入和权重对应相乘再相加即可
    else if constexpr (OutputDimensions == 1) {
      const auto inputVector = (const __m256i*)(input);

      // 这里计算处理完所有的输入需要多少次SIMD运算
      constexpr qint32 NumChunks = PaddedInputDimensions / SIMD_WIDTH;
      __m256i sum0 = _mm256_setzero_si256();
      const auto row0 = (const __m256i*)(&weights[0]);

      for (qint32 j = 0; j < NumChunks; ++j) {
        // 以4个8位为单位对应位置相乘相加
        sum0 = _mm256_add_epi32(sum0, _mm256_madd_epi16(_mm256_maddubs_epi16(
                                                            inputVector[j], row0[j]), Ones256));
      }

      // 最后要加上偏差，将一个__mm256i以32位为单位加在一起，转换成两个__mm128i 对应32位相加
      __m128i sum128 = _mm_add_epi32(_mm256_castsi256_si128(sum0),
                                     _mm256_extracti128_si256(sum0, 1));
      // A+B B+A D+C C+D
      sum128 = _mm_add_epi32(sum128, _mm_shuffle_epi32(sum128, _MM_PERM_BADC));
      // B+A+D+C A+B+C+D C+D+A+B D+C+B+A
      sum128 = _mm_add_epi32(sum128, _mm_shuffle_epi32(sum128, _MM_PERM_CDAB));
      // D+C+B+A+bias
      output[0] = _mm_cvtsi128_si32(sum128) + biases[0];
    }

    return output;
  }

private:
  /** 偏差的类型和网络的输出类型匹配 */
  using BiasType = OutputType;
  /** 权重的类型和网络的输入类型匹配，但是可以为负数 */
  using WeightType = qint8;

  /** 上一层网络 */
  PreviousLayer previousLayer;

  /** 对其到缓冲块的权重和偏差，加速SIMD运算效率 */
  alignas(CACHE_LINE_SIZE) BiasType biases[OutputDimensions];
  alignas(CACHE_LINE_SIZE) WeightType weights[OutputDimensions * PaddedInputDimensions];
};
}
