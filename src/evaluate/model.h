#pragma once
#include "global.h"

#include "input.h"
#include "dense.h"
#include "clippedrelu.h"

namespace PikaChess {
/** HalfKAv2模型
 *  9(将的位置) * 13(棋子的个数{2(双方) * 7(每方7种棋子) - 1(将的位置交集为空集，所以合并为一个特征)}) *
 *  90(每个棋子的位置) -> 特征转换 ->
 *  (512(转换后的特征) + 8(PSQT的部分)) x 2 -> 全连接层1 -> ClippedReLU ->
 *  16 -> 全连接层2 -> ClippedReLU ->
 *  32 -> 输出层
 *  -> 1(NNUE神经网络的部分估值) 与前面的PSQT部分合并 -> 最终局面评分
*/
static constexpr const char *MODEL_NAME = "HalfKAv2(Friend)";

/** 每一个棋子在输入层的位置(A Feature)，其中包括将在输入层的位置(K Feature) */
enum {
  PS_R_ROOK = 0 * 90, PS_B_ROOK = 1 * 90,
  PS_R_ADVISOR = 2 * 90, PS_B_ADVISOR = 3 * 90,
  PS_R_CANNON = 4 * 90, PS_B_CANNON = 5 * 90,
  PS_R_PAWN = 6 * 90, PS_B_PAWN = 7 * 90,
  PS_R_KNIGHT = 8 * 90, PS_B_KNIGHT = 9 * 90,
  PS_R_BISHOP = 10 * 90, PS_B_BISHOP = 11 * 90,
  PS_KING = 12 * 90, PS_ALL = 13 * 90,
  PS_KING_0 = 0 * PS_ALL, PS_KING_1 = 1 * PS_ALL, PS_KING_2 = 2 * PS_ALL,
  PS_KING_3 = 3 * PS_ALL, PS_KING_4 = 4 * PS_ALL, PS_KING_5 = 5 * PS_ALL,
  PS_KING_6 = 6 * PS_ALL, PS_KING_7 = 7 * PS_ALL, PS_KING_8 = 8 * PS_ALL
};

/** 整个模型的输入维度 */
static constexpr quint32 INPUT_DIMENSION = 9 * PS_ALL;
/** 一边经过特征转换层转换后的维度，两边就是512 x 2 = 1024 */
constexpr quint32 TRANSFORMED_FEATURE_DIMENSIONS = 512;
/** 一边特征转换后的PSQT的特征的个数 */
constexpr quint32 PSQT_BUCKETS = 8;
/** 模型的全连接层子网络的个数 */
constexpr quint32 LAYER_STACKS = 8;

/** 全连接层1的输入，经过特征转换后一共有512 x 2也就是1024个特征 */
using InputLayer = Input<TRANSFORMED_FEATURE_DIMENSIONS * 2>;
/** 全连接层1，激活函数采用ClippedReLU */
using HiddenLayer1 = ClippedReLU<Dense<InputLayer, 16>>;
/** 全连接层2，激活函数采用ClippedReLU */
using HiddenLayer2 = ClippedReLU<Dense<HiddenLayer1, 32>>;
/** 输出层，本层没有激活函数 */
using OutputLayer = Dense<HiddenLayer2, 1>;

/** 整个模型不包括特征转换层的架构，因为特征转换层需要单独出来以达到快速更新的目的 */
using Model = OutputLayer;

/** 根据当前的走子方和子的编号获得子力的特征位置(A Feature)，红黑翻转满足NNUE的翻转需求 */
static constexpr quint32 PIECE_FEATURE_INDEX[8][14] {
    { PS_R_ROOK, PS_R_KNIGHT, PS_R_CANNON, PS_R_BISHOP, PS_R_PAWN, PS_R_ADVISOR, PS_KING,
     PS_B_ROOK, PS_B_KNIGHT, PS_B_CANNON, PS_B_BISHOP, PS_B_PAWN, PS_B_ADVISOR, PS_KING },
    {}, {}, {}, {}, {}, {},
    { PS_B_ROOK, PS_B_KNIGHT, PS_B_CANNON, PS_B_BISHOP, PS_B_PAWN, PS_B_ADVISOR, PS_KING,
     PS_R_ROOK, PS_R_KNIGHT, PS_R_CANNON, PS_R_BISHOP, PS_R_PAWN, PS_R_ADVISOR, PS_KING }
};

/** 用将的位置获得将的特征位置(K Feature) */
static constexpr uint32_t KING_FEATURE_INDEX[90] {
    0, 0, 0, PS_KING_0, PS_KING_1, PS_KING_2, 0, 0, 0,
    0, 0, 0, PS_KING_3, PS_KING_4, PS_KING_5, 0, 0, 0,
    0, 0, 0, PS_KING_6, PS_KING_7, PS_KING_8, 0, 0, 0,
    0, 0, 0,         0,         0,         0, 0, 0, 0,
    0, 0, 0,         0,         0,         0, 0, 0, 0,
    0, 0, 0,         0,         0,         0, 0, 0, 0,
    0, 0, 0,         0,         0,         0, 0, 0, 0,
    0, 0, 0, PS_KING_6, PS_KING_7, PS_KING_8, 0, 0, 0,
    0, 0, 0, PS_KING_3, PS_KING_4, PS_KING_5, 0, 0, 0,
    0, 0, 0, PS_KING_0, PS_KING_1, PS_KING_2, 0, 0, 0,
};

/** 根据当前走子方翻转红方和黑方的位置，以满足NNUE的翻转需求 */
static constexpr quint8 ORIENT[8][90] {
    {   81, 82, 83, 84, 85, 86, 87, 88, 89,
        72, 73, 74, 75, 76, 77, 78, 79, 80,
        63, 64, 65, 66, 67, 68, 69, 70, 71,
        54, 55, 56, 57, 58, 59, 60, 61, 62,
        45, 46, 47, 48, 49, 50, 51, 52, 53,
        36, 37, 38, 39, 40, 41, 42, 43, 44,
        27, 28, 29, 30, 31, 32, 33, 34, 35,
        18, 19, 20, 21, 22, 23, 24, 25, 26,
         9, 10, 11, 12, 13, 14, 15, 16, 17,
         0,  1,  2,  3,  4,  5,  6,  7,  8, },
    {}, {}, {}, {}, {}, {},
    {    0,  1,  2,  3,  4,  5,  6,  7,  8,
         9, 10, 11, 12, 13, 14, 15, 16, 17,
        18, 19, 20, 21, 22, 23, 24, 25, 26,
        27, 28, 29, 30, 31, 32, 33, 34, 35,
        36, 37, 38, 39, 40, 41, 42, 43, 44,
        45, 46, 47, 48, 49, 50, 51, 52, 53,
        54, 55, 56, 57, 58, 59, 60, 61, 62,
        63, 64, 65, 66, 67, 68, 69, 70, 71,
        72, 73, 74, 75, 76, 77, 78, 79, 80,
        81, 82, 83, 84, 85, 86, 87, 88, 89, }
};

/** 根据当前的走子方，棋子类型，棋子位置，王的位置获取这个特征在特征转换层的输入位置(KA Feature) */
inline quint32 FeatureIndex(quint8 side, quint8 index, quint8 chess, quint8 kingIndex) {
  return KING_FEATURE_INDEX[kingIndex] + PIECE_FEATURE_INDEX[side][chess] + ORIENT[side][index];
}
}
