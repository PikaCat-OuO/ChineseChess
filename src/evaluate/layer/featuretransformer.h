#pragma once
#include "global.h"
#include "model.h"
#include "move.h"
#include "accumulator.h"

namespace PikaChess {
/** 偏差的类型 */
using BiasType = qint16;
/** 权重的类型 */
using WeightType = qint16;
using PSQTWeightType = qint32;

/** 对于AVX2而言最佳的寄存器个数，CPU内一共有16个YMM寄存器 */
static constexpr quint8 NUM_REGS = 16;

/** 特征转换器 */
class FeatureTransformer {
private:
  /** 一边的特征转换后的结果维度 */
  static constexpr quint32 HalfDimensions = TRANSFORMED_FEATURE_DIMENSIONS;
  /** 一次可以处理多少个数据，16个寄存器，每个可以处理16个数据，一次可以处理256个数据 */
  static constexpr quint32 TileHeight = 256;

  /** 在NNUE文件中绑定的哈希值 */
  static constexpr quint32 HASH_VALUE = 0x5f234cb8u;

public:
  /** 特征转换的输出类型 */
  using OutputType = quint8;

  /** 输入维度，输出维度 */
  static constexpr quint32 InputDimensions = INPUT_DIMENSION;
  static constexpr quint32 OutputDimensions = HalfDimensions * 2;

  /** 本层需要使用到的缓冲区大小 */
  static constexpr quint64 BufferSize = OutputDimensions * sizeof(OutputType);

  /** NNUE网络文件中嵌入的哈希值 */
  static constexpr quint32 getHashValue() { return HASH_VALUE ^ OutputDimensions; }

  /** 将网络的权重和偏差从文件中读取到内存中 */
  bool readParameters(std::istream& stream) {
    ReadInt<BiasType>(stream, biases, HalfDimensions);
    ReadInt<WeightType>(stream, weights, HalfDimensions * INPUT_DIMENSION);
    ReadInt<PSQTWeightType>(stream, psqtWeights, PSQT_BUCKETS * INPUT_DIMENSION);
    return !stream.fail();
  }

  /** 将一个棋盘的特征转换，并且将PSQT部分的分数返回 */
  qint32 transform(Accumulator &accumulator, quint8 side, OutputType* output, int bucket) const {
    const quint8 perspectives[2] = { side, quint8(side ^ OPP_SIDE) };
    const auto &accumulation = accumulator.accumulation;
    const auto &psqtAccumulation = accumulator.psqtAccumulation;

    // 直接获得PSQT部分的分数
    const auto psqt = (psqtAccumulation[perspectives[0]][bucket]
                       - psqtAccumulation[perspectives[1]][bucket]) >> 1;

    // 一共有多少块需要处理
    constexpr quint32 NumChunks = HalfDimensions / SIMD_WIDTH;
    // 用于ClippedReLU的下限
    const __m256i Zero = _mm256_setzero_si256();

    // 对双方转换后的特征进行ClippedReLU操作
    for (quint8 p = 0; p < 2; ++p) {
      const quint32 offset = HalfDimensions * p;
      auto out = reinterpret_cast<__m256i*>(&output[offset]);
      for (quint8 j = 0; j < NumChunks; ++j)
      {
        __m256i sum0 = _mm256_load_si256(&reinterpret_cast<const __m256i*>
                                         (accumulation[perspectives[p]])[j * 2 + 0]);
        __m256i sum1 = _mm256_load_si256(&reinterpret_cast<const __m256i*>
                                         (accumulation[perspectives[p]])[j * 2 + 1]);

        _mm256_store_si256(&out[j], _mm256_permute4x64_epi64(
                                        _mm256_max_epi8(_mm256_packs_epi16(sum0, sum1), Zero),
                                        0b11011000));
      }
    }

    return psqt;
  }

  /** 更新累加器，针对side方来更新，通常每走一步棋要调用两次本函数，更新双方的累加器 */
  void updateAccumulator(const Accumulator &oldAcc, Accumulator &newAcc,
                         quint8 side, const Move& move) const {
    // 定义需要使用的寄存器
    __m256i acc[NUM_REGS];
    __m256i psqt;

    // 先复制将的位置
    newAcc.kingPos[side] = oldAcc.kingPos[side];

    // 一步只可能添加一个特征，也就是走子方走到的那个地方
    quint32 added { FeatureIndex(side, move.to(), move.chess(), newAcc.kingPos[side]) };

    // 一步可能删除一到两个特征，也就是走子方离开的那个地方，加上吃掉走到的那个位置的子
    qint32 removed[2];
    // 走到的那个地方
    removed[0] = FeatureIndex(side, move.from(), move.chess(), newAcc.kingPos[side]);
    // 如果是吃子步则添加上第二个特征，如果不是就置为-1表示没有吃子
    if (move.isCapture()) {
      removed[1] = FeatureIndex(side, move.to(), move.victim(), newAcc.kingPos[side]);
    } else removed[1] = -1;

    // 首先处理特征转换的部分
    for (quint32 j = 0; j < HalfDimensions / TileHeight; ++j) {
      // 将特征从旧的累加器载入到寄存器中
      auto accTile = (__m256i*)(&oldAcc.accumulation[side][j * TileHeight]);
      for (quint32 k = 0; k < NUM_REGS; ++k) acc[k] = _mm256_load_si256(&accTile[k]);

      // 删除那些已经移除的特征
      for (qint32 index : removed) {
        if (-1 not_eq index) {
          quint32 offset = HalfDimensions * index + j * TileHeight;
          auto column = (const __m256i*)(&weights[offset]);
          for (quint32 k = 0; k < NUM_REGS; ++k) acc[k] = _mm256_sub_epi16(acc[k], column[k]);
        }
      }

      // 添加上新增的特征，也就是本步走到的那个地方
      quint32 offset = HalfDimensions * added + j * TileHeight;
      auto column = (const __m256i*)(&weights[offset]);
      for (quint32 k = 0; k < NUM_REGS; ++k) acc[k] = _mm256_add_epi16(acc[k], column[k]);

      // 将处理后的结果保存到新的累加器中
      accTile = (__m256i*)(&newAcc.accumulation[side][j * TileHeight]);
      for (quint32 k = 0; k < NUM_REGS; ++k) _mm256_store_si256(&accTile[k], acc[k]);
    }

    // 接着处理PSQT的部分，加载PSQT部分的累加器到寄存器中
    auto accTilePsqt = (__m256i*)(&oldAcc.psqtAccumulation[side][0]);
    psqt = _mm256_load_si256(accTilePsqt);

    // 删除那些已经移除的特征
    for (const auto index : removed) {
      if (-1 not_eq index) {
        const quint32 offset = PSQT_BUCKETS * index;
        auto columnPsqt = (const __m256i*)(&psqtWeights[offset]);
        psqt = _mm256_sub_epi32(psqt, columnPsqt[0]);
      }
    }

    // 添加上新增的特征，也就是本步走到的那个地方
    const quint32 offset = PSQT_BUCKETS * added;
    auto columnPsqt = (const __m256i*)(&psqtWeights[offset]);
    psqt = _mm256_add_epi32(psqt, *columnPsqt);

    // 将处理后的结果保存到新的累加器中
    accTilePsqt = (__m256i*)(&newAcc.psqtAccumulation[side][0]);
    _mm256_store_si256(accTilePsqt, psqt);
  }

  /** 刷新累加器，使用提供的特征位置重置整个累加器，只针对side方更新
   *  由于将走动了，所以需要调用updateAccumulator，更新另一方的累加器 */
  void refreshAccumulator(Accumulator &accumulator, quint8 side, qint32 *featureIndexes) const {
    // 定义需要使用的寄存器
    __m256i acc[NUM_REGS];
    __m256i psqt;

    // 首先处理特征转换的部分
    for (quint32 j = 0; j < HalfDimensions / TileHeight; ++j) {
      // 将偏差复制到寄存器中
      auto biasesTile = (const __m256i*)(&biases[j * TileHeight]);
      for (quint32 k = 0; k < NUM_REGS; ++k) acc[k] = biasesTile[k];

      // 将特征逐个添加到寄存器中，直到累加完成
      qint32 *now = featureIndexes;
      while (-1 not_eq *now) {
        quint32 index = *now++;
        const quint32 offset = HalfDimensions * index + j * TileHeight;
        auto column = (const __m256i*)(&weights[offset]);

        for (quint8 k = 0; k < NUM_REGS; ++k) acc[k] = _mm256_add_epi16(acc[k], column[k]);
      }

      // 将处理后的结果保存回累加器中
      auto accTile = (__m256i*)(&accumulator.accumulation[side][j * TileHeight]);
      for (quint8 k = 0; k < NUM_REGS; ++k) _mm256_store_si256(&accTile[k], acc[k]);
    }

    // 接着处理PSQT的部分
    psqt = _mm256_setzero_si256();

    // 将特征逐个累加到寄存器中
    while (-1 not_eq *featureIndexes) {
      quint32 index = *featureIndexes++;
      const quint32 offset = PSQT_BUCKETS * index;
      auto columnPsqt = (const __m256i*)(&psqtWeights[offset]);
      psqt = _mm256_add_epi32(psqt, *columnPsqt);
    }

    // 将处理后的结果保存回累加器中
    auto accTilePsqt = (__m256i*)(&accumulator.psqtAccumulation[side][0]);
    _mm256_store_si256(accTilePsqt, psqt);
  }

  /** 本层所用的偏差，权重和PSQT权重，对其到CPU的缓存块大小 */
  alignas(CACHE_LINE_SIZE) BiasType biases[HalfDimensions];
  alignas(CACHE_LINE_SIZE) WeightType weights[HalfDimensions * InputDimensions];
  alignas(CACHE_LINE_SIZE) PSQTWeightType psqtWeights[InputDimensions * PSQT_BUCKETS];
};
}
