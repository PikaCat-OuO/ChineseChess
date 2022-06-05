#pragma once
#include <QtConcurrent/QtConcurrent>
#include "qglobal.h"
#include <iostream>

namespace PikaChess {
/** 选边 */
constexpr quint8 RED { 0 }, BLACK { 7 }, OPP_SIDE { 7 };

/** 棋子种类 */
constexpr quint8 ROOK { 0 }, KNIGHT { 1 }, CANNON { 2 }, BISHOP { 3 };
constexpr quint8 PAWN { 4 }, ADVISOR { 5 }, KING { 6 };

/** 空棋子 */
constexpr quint8 EMPTY { 14 };
/** 所有的棋子类型，上层为滑动棋子，下层为跳跃棋子 */
constexpr quint8 RED_ROOK { 0 }, RED_KNIGHT { 1 }, RED_CANNON { 2 }, RED_BISHOP { 3 };
constexpr quint8 RED_PAWN { 4 }, RED_ADVISOR { 5 }, RED_KING { 6 };
constexpr quint8 BLACK_ROOK { 7 }, BLACK_KNIGHT { 8 }, BLACK_CANNON { 9 }, BLACK_BISHOP { 10 };
constexpr quint8 BLACK_PAWN { 11 }, BLACK_ADVISOR { 12 }, BLACK_KING { 13 };

/** 定义赢棋输棋的分数 */
constexpr qint16 MATE_SCORE { 30000 }, LOSS_SCORE { -30000 };
/** 长将判负的分值，在该值之内则不写入置换表 */
constexpr qint16 BAN_SCORE_MATE { 29500 }, BAN_SCORE_LOSS { -29500 };
/** 搜索出赢棋和输棋的分值界限，超出此值就说明已经搜索出杀棋了 */
constexpr qint16 WIN_SCORE { 29000 }, LOST_SCORE { -29000 };

/** 搜索状态机的阶段 */
constexpr quint8 PHASE_HASH { 0 };
constexpr quint8 PHASE_CAPTURE_GEN { 1 };
constexpr quint8 PHASE_CAPTURE { 2 };
constexpr quint8 PHASE_KILLER1 { 3 };
constexpr quint8 PHASE_KILLER2 { 4 };
constexpr quint8 PHASE_NOT_CAPTURE_GEN { 5 };
constexpr quint8 PHASE_REST { 6 };

/** MVV/LVA Most Valuable Victim Least Valuable Attacker
 *  每种子力的MVVLVA价值，车马炮象兵士将 */
constexpr qint8 MVVLVA[14] { 50, 20, 20, 2, 5, 2, 100,
                            50, 20, 20, 2, 5, 2, 100 };

/** 置换表的大小 */
constexpr quint32 HASH_SIZE { 1 << 23 };
/** 置换表掩码 */
constexpr quint32 HASH_MASK { HASH_SIZE - 1 };

/** 走法类型 */
constexpr quint8 HASH_ALPHA { 0 }, HASH_BETA { 1 }, HASH_PV { 2 };

/** 空着裁剪标志 */
constexpr bool NO_NULL { false };

/** 判断两个位置是否同一行 */
extern bool SAME_RANK[90][90];

/** 判断是否需要检测被捉 */
extern quint8 CHASE_INFO[14][14];

/** 棋子的flag */
constexpr quint16 CHESS_FLAG[14] {
    1 << 0, 1 << 1, 1 << 2, 1 << 3, 1 << 4,
    1 << 5, 1 << 6, 1 << 7, 1 << 8, 1 << 9,
    1 << 10, 1 << 11, 1 << 12, 1 << 13
};

/** 棋子的残局价值，用于差值裁剪，车马炮象兵士将 */
constexpr quint16 PIECE_VALUE[14] {
    1380, 800, 700, 300, 270, 450, 0,
    1380, 800, 700, 300, 270, 450, 0
};

/** NNUE文件的版本 */
constexpr quint32 VERSION = 0x7AF32F20u;
/** 评分时输出的放缩系数 */
constexpr quint8 OUTPUT_SCALE_BITS = 4;
/** 权重的放缩系数 */
constexpr quint8 WEIGHTS_SCALE_BITS = 6;

/** CPU缓存单元的大小 */
constexpr quint8 CACHE_LINE_SIZE = 64;

/** SIMD的宽度 */
constexpr quint8 SIMD_WIDTH = 32;
constexpr quint8 MAX_SIMD_WIDTH = 32;

/** 将n向上取整为base的整数倍 */
template <typename IntType>
constexpr IntType CeilToMultiple(IntType n, IntType base) {
  return (n + base - 1) / base * base;
}

/** 从NNUE文件中读取数据 */
template <typename IntType>
inline IntType ReadInt(std::istream &stream) {
  IntType result;
  stream.read((char*)(&result), sizeof(IntType));
  return result;
}

/** 从NNUE文件中读取数据 */
template <typename IntType>
inline void ReadInt(std::istream &stream, IntType *out, quint64 count) {
  stream.read((char*)(out), sizeof(IntType) * count);
}
}
