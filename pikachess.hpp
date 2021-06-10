#include "httprequest.h"
#include <QReadWriteLock>
#include <QString>
#include <QtConcurrent/QtConcurrent>
// 本人懒狗，所以用这两句咯~
#include <bits/stdc++.h>
using namespace std;

/*
 * 给它们取一些有意义的别名吧!
 * MetaType定义
 * 为什么不使用stl中的轮子呢？读者不妨可以自行尝试，结果会让你心服口服的!
 */
// 游戏选边相关
using Side = uint8_t;

// 其他相关
using Index = uint8_t;
using Count = uint8_t;

// 棋盘相关
using Chess = uint8_t;
using ChessBoard = Chess[256];

// 位置、走法相关
using Position = uint8_t;
using Move = uint16_t;
using Delta = int8_t;
using SpanBoard = Delta[512];
using Step = std::tuple<Position, Position, Position, Position>;

// 搜索相关
// 每一个局面的所有走法列表，保留128个，不可能产生超过128个走法
using MoveList = Move[128];
using Score = int16_t;
using Depth = int8_t;
using MoveFlag = bool;
using NullFlag = bool;
using Clock = clock_t;

// fen相关
using FenMap = unordered_map<Chess, char>;
// 有限状态机的阶段
using Phase = uint8_t;

// 历史表相关
// 历史表分数
using HistoryScore = uint64_t;
// 历史表，一个走法的最大值为: (203 << 8) | 202 = 52170
using History = HistoryScore[52171];

// 杀手走法表相关
using Killer = Move[128][2];

// Zobrist相关
// 棋盘的zobrist值，低位索引，高位校验
using ZobristValue = uint64_t;
// 棋盘上每一个位置的zobrist值
using Zobrist = ZobristValue[16][256];

// 历史走法相关
// 重复局面标志
using RepeatFlag = uint8_t;
// 每一步的结构
struct MoveItem {
  // 走该走法前对应的Zobrist，用于检查重复
  ZobristValue mZobrist;
  // 走法
  Move mMove;
  // 保存被吃掉的子，也可以标志这一步是不是吃子走法，因为空的值为0
  Chess mVictim;
  // 是否是将军步
  bool mCheck;
};
// 历史走法表
using HistoryMove = MoveItem[256];

// 置换表相关
using HashMask = uint64_t;
using HashFlag = uint8_t;
// 置换表大小
constexpr size_t HASH_SIZE = 1 << 20;
// 取置换表项时的掩码
HashMask HASH_MASK{HASH_SIZE - 1};
// 定义置换表项的flag
// ALPHA节点的置换表项
constexpr HashFlag HASH_ALPHA{0};
// BETA节点的置换表项
constexpr HashFlag HASH_BETA{1};
// PV节点的置换表项
constexpr HashFlag HASH_PV{2};
struct HashItem {
  // 走该走法前对应的Zobrist，用于校验
  ZobristValue mZobrist;
  // 走法
  Move mMove;
  // 该走法对应的分数
  Score mScore;
  // 记录该项时所处的深度
  Depth mDepth;
  // 该走法对应的类型（ALPHA，PV，BETA）
  HashFlag mFlag;
};
// 置换表
using HashTable = HashItem[HASH_SIZE];

// 棋盘位置信息，用于多线程搜索
// 定义move的初始值
Move INVALID_MOVE{0};
// 定义棋盘位置信息
struct PositionInfo {
  // 棋盘局面信息
  ChessBoard mChessBoard{};
  // 现在距离根节点的深度
  Depth mDistance{0};
  // 历史表，用于加速
  History mHistory{};
  // 杀手表，记录beta截断的走法
  Killer mKiller{};
  // 历史走法表目前多大
  size_t mHistorySize{1};
  // 历史走法表
  HistoryMove mHistoryMove{};
  // 棋盘现在的Zobrist值
  ZobristValue mZobrist{0};
  // 定义最好走法
  Move mBestMove{INVALID_MOVE};
  // 红方的分数
  Score mRedScore{888};
  // 黑方的分数
  Score mBlackScore{888};
  // 构造函数
  PositionInfo();
  // 初始化函数
  void resetBoard();
  // 多线程时使用，将主局面的PositionInfo拷贝到本局面
  void setThreadPositionInfo(const PositionInfo &positionInfo);
  // 判断该棋子是红方棋子还是黑方棋子, 不能传入空，否则当作红方子力处理
  inline Side getChessSide(const Position pos);
  // 该位置是否是空的
  inline bool isEmpty(const Position pos);
  // 该位置是否是对方的棋子
  inline bool isOppChess(const Position pos, const Side side);
  // 该位置是否为空或为对方的棋子
  inline bool isEmptyOrOppChess(const Position pos, const Side side);
  // 获取棋盘上的将
  Position getKING(const Side side);
  // 判断该位置为否被将军,side为走棋方
  bool isChecked(const Side side);
  // 判断一个走法是否是合法的走法
  bool isLegalMove(const Move move, const Side side);
  // 检查重复走法
  inline RepeatFlag getRepeatFlag();
  // 返回和棋的分值
  inline Score getDrawScore();
  // 从重复检查状态码中提取分数
  inline Score scoreRepeatFlag(const RepeatFlag repeatFlag);
  // 计算红方走了某个位置后的分数
  inline void calcRedMove(const Move move);
  // 计算黑方走了某个位置后的分数
  inline void calcBlackMove(const Move move);
  // 计算红方还原了某一步棋后的分数
  inline void calcRedUnMove(const Move move, const Chess victim);
  // 计算黑方还原了某一步棋后的分数
  inline void calcBlackUnMove(const Move move, const Chess victim);
  // 计算走一步的分数和Zobrist
  inline void calcMove(const Move move, const Side side);
  // 计算撤销走一步的分数和Zobrist
  inline void calcUnMove(const Move move, const Chess victim, const Side side);
  // 评分函数
  inline Score evaluate(const Side side);
  // 搜索置换表
  Score probeHash(Score alpha, Score beta, Depth depth, Move &hashMove);
  // 保存到置换表
  void recordHash(HashFlag hashFlag, Score score, Depth depth, Move move);
  // 用于设置历史表、杀手表
  inline void setBestMove(const Move move, const Depth depth);
  // 走法生成
  Count moveGen(MoveList &moves, const Side side,
                const MoveFlag capture = false);
  // 撤销走棋
  inline void unMakeMove(const Move move, const Side side);
  // 走棋,返回是否走成功
  inline bool makeMove(const Move move, const Side side);
  // 当前是否可以走空步，即不在残局阶段
  inline bool isNullOk(const Side side);
  // 走一步空步
  inline void makeNullMove();
  // 撤销走一步空步
  inline void unMakeNullMove();
  // 静态搜索
  Score searchQuiescence(Score alpha, const Score beta, const Side side);
  // 完全局面搜索
  Score searchFull(Score alpha, const Score beta, const Depth depth,
                   const Side side, const NullFlag nullOk = true);
  // 根节点的搜索
  Score searchRoot(const Depth depth);
  // fen生成
  string fenGen();
  // 搜索云库
  tuple<QString, Step> searchBook();
};

// 定义搜索有限状态机的阶段
constexpr Phase PHASE_HASH{0};
constexpr Phase PHASE_KILLER1{1};
constexpr Phase PHASE_KILLER2{2};
constexpr Phase PHASE_GEN_MOVE{3};
constexpr Phase PHASE_ALL_MOVE{4};
// 搜索的有限状态机
struct SearchMachine {
  // 位置信息
  PositionInfo &mPositionInfo;
  // 现在在第几个阶段，初始值为哈希表走法处
  Phase mNowPhase{PHASE_HASH};
  // 选边标志
  Side mSide;
  // 现在正在遍历第几个走法
  Index mNowIndex{0};
  // 总共有多少种走法
  Count mTotalMoves{0};
  // 所有走法的列表
  MoveList mMoves{};
  // 置换表走法，杀手走法1、2
  Move mHash, mKiller1, mKiller2;
  // 构造函数
  SearchMachine(PositionInfo &positionInfo, const Move hashMove,
                const Side side);
  // 返回走法的函数
  Move nextMove();
};

/* 真正的数据定义 */

// 定义棋子类型和边表示，车马炮兵象士将
constexpr Side RED{0}, BLACK{8};
constexpr Chess EMPTY{0};
constexpr Chess ROOK{1}, KNIGHT{2}, CANNON{3};
constexpr Chess PAWN{4}, BISHOP{5}, ADVISOR{6};
constexpr Chess KING{7};
constexpr Chess RED_KING{7};
constexpr Chess BLACK_KING{15};

// 定义棋盘，就是中国象棋的初始摆法啦~
ChessBoard INIT_BOARD{
    0, 0,  0, 0,  0,  0,  0, 0,  0, 0, 0,  0, 0,  0,  0,  0,  0,  0,  0,  0,
    0, 0,  0, 0,  0,  0,  0, 0,  0, 0, 0,  0, 0,  0,  0,  0,  0,  0,  0,  0,
    0, 0,  0, 0,  0,  0,  0, 0,  0, 0, 0,  9, 10, 13, 14, 15, 14, 13, 10, 9,
    0, 0,  0, 0,  0,  0,  0, 0,  0, 0, 0,  0, 0,  0,  0,  0,  0,  0,  0,  0,
    0, 0,  0, 0,  11, 0,  0, 0,  0, 0, 11, 0, 0,  0,  0,  0,  0,  0,  0,  12,
    0, 12, 0, 12, 0,  12, 0, 12, 0, 0, 0,  0, 0,  0,  0,  0,  0,  0,  0,  0,
    0, 0,  0, 0,  0,  0,  0, 0,  0, 0, 0,  0, 0,  0,  0,  0,  0,  0,  0,  0,
    0, 0,  0, 0,  0,  0,  0, 4,  0, 4, 0,  4, 0,  4,  0,  4,  0,  0,  0,  0,
    0, 0,  0, 0,  3,  0,  0, 0,  0, 0, 3,  0, 0,  0,  0,  0,  0,  0,  0,  0,
    0, 0,  0, 0,  0,  0,  0, 0,  0, 0, 0,  0, 0,  0,  0,  1,  2,  5,  6,  7,
    6, 5,  2, 1,  0,  0,  0, 0,  0, 0, 0,  0, 0,  0,  0,  0,  0,  0,  0,  0,
    0, 0,  0, 0,  0,  0,  0, 0,  0, 0, 0,  0, 0,  0,  0,  0,  0,  0,  0,  0,
    0, 0,  0, 0,  0,  0,  0, 0,  0, 0, 0,  0, 0,  0,  0,  0};

//子力的位置分数
constexpr Score RED_VALUE[8][256]{
    {// 空
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {// 车
     0, 0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 0, 0, 0,
     0, 0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 0, 0, 0,
     0, 0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 0, 0, 0,
     0, 0, 0, 206, 208, 207, 213, 214, 213, 207, 208, 206, 0, 0, 0, 0,
     0, 0, 0, 206, 212, 209, 216, 233, 216, 209, 212, 206, 0, 0, 0, 0,
     0, 0, 0, 206, 208, 207, 214, 216, 214, 207, 208, 206, 0, 0, 0, 0,
     0, 0, 0, 206, 213, 213, 216, 216, 216, 213, 213, 206, 0, 0, 0, 0,
     0, 0, 0, 208, 211, 211, 214, 215, 214, 211, 211, 208, 0, 0, 0, 0,
     0, 0, 0, 208, 212, 212, 214, 215, 214, 212, 212, 208, 0, 0, 0, 0,
     0, 0, 0, 204, 209, 204, 212, 214, 212, 204, 209, 204, 0, 0, 0, 0,
     0, 0, 0, 198, 208, 204, 212, 212, 212, 204, 208, 198, 0, 0, 0, 0,
     0, 0, 0, 200, 208, 206, 212, 200, 212, 206, 208, 200, 0, 0, 0, 0,
     0, 0, 0, 194, 206, 204, 212, 200, 212, 204, 206, 194, 0, 0, 0, 0,
     0, 0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 0, 0, 0,
     0, 0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 0, 0, 0,
     0, 0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 0, 0, 0},
    {// 马
     0, 0, 0, 0,  0,   0,   0,   0,   0,   0,   0,   0,  0, 0, 0, 0,
     0, 0, 0, 0,  0,   0,   0,   0,   0,   0,   0,   0,  0, 0, 0, 0,
     0, 0, 0, 0,  0,   0,   0,   0,   0,   0,   0,   0,  0, 0, 0, 0,
     0, 0, 0, 90, 90,  90,  96,  90,  96,  90,  90,  90, 0, 0, 0, 0,
     0, 0, 0, 90, 96,  103, 97,  94,  97,  103, 96,  90, 0, 0, 0, 0,
     0, 0, 0, 92, 98,  99,  103, 99,  103, 99,  98,  92, 0, 0, 0, 0,
     0, 0, 0, 93, 108, 100, 107, 100, 107, 100, 108, 93, 0, 0, 0, 0,
     0, 0, 0, 90, 100, 99,  103, 104, 103, 99,  100, 90, 0, 0, 0, 0,
     0, 0, 0, 90, 98,  101, 102, 103, 102, 101, 98,  90, 0, 0, 0, 0,
     0, 0, 0, 92, 94,  98,  95,  98,  95,  98,  94,  92, 0, 0, 0, 0,
     0, 0, 0, 93, 92,  94,  95,  92,  95,  94,  92,  93, 0, 0, 0, 0,
     0, 0, 0, 85, 90,  92,  93,  78,  93,  92,  90,  85, 0, 0, 0, 0,
     0, 0, 0, 88, 85,  90,  88,  90,  88,  90,  85,  88, 0, 0, 0, 0,
     0, 0, 0, 0,  0,   0,   0,   0,   0,   0,   0,   0,  0, 0, 0, 0,
     0, 0, 0, 0,  0,   0,   0,   0,   0,   0,   0,   0,  0, 0, 0, 0,
     0, 0, 0, 0,  0,   0,   0,   0,   0,   0,   0,   0,  0, 0, 0, 0},
    {// 炮
     0, 0, 0, 0,   0,   0,   0,  0,   0,  0,   0,   0,   0, 0, 0, 0,
     0, 0, 0, 0,   0,   0,   0,  0,   0,  0,   0,   0,   0, 0, 0, 0,
     0, 0, 0, 0,   0,   0,   0,  0,   0,  0,   0,   0,   0, 0, 0, 0,
     0, 0, 0, 100, 100, 96,  91, 90,  91, 96,  100, 100, 0, 0, 0, 0,
     0, 0, 0, 98,  98,  96,  92, 89,  92, 96,  98,  98,  0, 0, 0, 0,
     0, 0, 0, 97,  97,  96,  91, 92,  91, 96,  97,  97,  0, 0, 0, 0,
     0, 0, 0, 96,  99,  99,  98, 100, 98, 99,  99,  96,  0, 0, 0, 0,
     0, 0, 0, 96,  96,  96,  96, 100, 96, 96,  96,  96,  0, 0, 0, 0,
     0, 0, 0, 95,  96,  99,  96, 100, 96, 99,  96,  95,  0, 0, 0, 0,
     0, 0, 0, 96,  96,  96,  96, 96,  96, 96,  96,  96,  0, 0, 0, 0,
     0, 0, 0, 97,  96,  100, 99, 101, 99, 100, 96,  97,  0, 0, 0, 0,
     0, 0, 0, 96,  97,  98,  98, 98,  98, 98,  97,  96,  0, 0, 0, 0,
     0, 0, 0, 96,  96,  97,  99, 99,  99, 97,  96,  96,  0, 0, 0, 0,
     0, 0, 0, 0,   0,   0,   0,  0,   0,  0,   0,   0,   0, 0, 0, 0,
     0, 0, 0, 0,   0,   0,   0,  0,   0,  0,   0,   0,   0, 0, 0, 0,
     0, 0, 0, 0,   0,   0,   0,  0,   0,  0,   0,   0,   0, 0, 0, 0},
    {// 兵
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  9,  9,  9,  11, 13, 11,
     9,  9,  9,  0,  0,  0,  0,  0,  0,  0,  19, 24, 34, 42, 44, 42, 34, 24, 19,
     0,  0,  0,  0,  0,  0,  0,  19, 24, 32, 37, 37, 37, 32, 24, 19, 0,  0,  0,
     0,  0,  0,  0,  19, 23, 27, 29, 30, 29, 27, 23, 19, 0,  0,  0,  0,  0,  0,
     0,  14, 18, 20, 27, 29, 27, 20, 18, 14, 0,  0,  0,  0,  0,  0,  0,  7,  0,
     13, 0,  16, 0,  13, 0,  7,  0,  0,  0,  0,  0,  0,  0,  7,  0,  7,  0,  15,
     0,  7,  0,  7,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0},
    {// 象
     0, 0,  0, 0,  0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
     0, 0,  0, 0,  0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
     0, 0,  0, 0,  0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
     0, 0,  0, 0,  0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
     0, 0,  0, 0,  0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
     0, 0,  0, 0,  0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
     0, 20, 0, 0,  0, 20, 0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
     0, 0,  0, 0,  0, 0,  0, 0, 0, 18, 0, 0, 0, 23, 0, 0, 0, 18, 0, 0, 0, 0,
     0, 0,  0, 0,  0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 20,
     0, 0,  0, 20, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
     0, 0,  0, 0,  0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
     0, 0,  0, 0,  0, 0,  0, 0, 0, 0,  0, 0, 0, 0},
    {// 士
     0,  0, 0,  0, 0, 0, 0, 0,  0, 0, 0, 0, 0,  0, 0,  0, 0, 0, 0, 0, 0, 0,
     0,  0, 0,  0, 0, 0, 0, 0,  0, 0, 0, 0, 0,  0, 0,  0, 0, 0, 0, 0, 0, 0,
     0,  0, 0,  0, 0, 0, 0, 0,  0, 0, 0, 0, 0,  0, 0,  0, 0, 0, 0, 0, 0, 0,
     0,  0, 0,  0, 0, 0, 0, 0,  0, 0, 0, 0, 0,  0, 0,  0, 0, 0, 0, 0, 0, 0,
     0,  0, 0,  0, 0, 0, 0, 0,  0, 0, 0, 0, 0,  0, 0,  0, 0, 0, 0, 0, 0, 0,
     0,  0, 0,  0, 0, 0, 0, 0,  0, 0, 0, 0, 0,  0, 0,  0, 0, 0, 0, 0, 0, 0,
     0,  0, 0,  0, 0, 0, 0, 0,  0, 0, 0, 0, 0,  0, 0,  0, 0, 0, 0, 0, 0, 0,
     0,  0, 0,  0, 0, 0, 0, 0,  0, 0, 0, 0, 20, 0, 20, 0, 0, 0, 0, 0, 0, 0,
     0,  0, 0,  0, 0, 0, 0, 23, 0, 0, 0, 0, 0,  0, 0,  0, 0, 0, 0, 0, 0, 0,
     20, 0, 20, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0,  0, 0,  0, 0, 0, 0, 0, 0, 0,
     0,  0, 0,  0, 0, 0, 0, 0,  0, 0, 0, 0, 0,  0, 0,  0, 0, 0, 0, 0, 0, 0,
     0,  0, 0,  0, 0, 0, 0, 0,  0, 0, 0, 0, 0,  0},
    {// 将
     0, 0, 0, 0, 0, 0, 0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
     1, 0, 0, 0, 0, 0, 0,  0,  0,  0, 0, 0, 0, 0, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 11, 15, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0,  0,  0,  0, 0, 0, 0, 0, 0, 0}};

// 是否在棋盘内
constexpr ChessBoard IN_BOARD{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
    0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
    0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
    0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// 是否在九宫内
constexpr ChessBoard IN_SQRT{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// 是否在红子半边
constexpr ChessBoard IN_RED_HALF{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
    0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
    0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// 是否在黑子半边
constexpr ChessBoard IN_BLACK_HALF{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
    0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// 判断步长是否符合特定走法的数组，1=将，2=士，3=象
constexpr SpanBoard LEGAL_SPAN{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0,
    0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 2, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// 根据步长判断马是否蹩腿的数组
constexpr SpanBoard LEGAL_KNIGHT_PIN_SPAN{
    0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0,  0, 0,  0, 0, 0,   0,  0,   0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0,  0, 0,  0, 0, 0,   0,  0,   0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0,  0, 0,  0, 0, 0,   0,  0,   0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0,  0, 0,  0, 0, 0,   0,  0,   0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0,  0, 0,  0, 0, 0,   0,  0,   0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0,  0, 0,  0, 0, 0,   0,  0,   0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0,  0, 0,  0, 0, 0,   0,  0,   0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0,  0, 0,  0, 0, 0,   0,  0,   0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0,  0, 0,  0, 0, 0,   0,  0,   0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0,  0, 0,  0, 0, -16, 0,  -16, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0,  1, 0,  0, 0, 0,   0,  0,   0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0,  0, 0,  0, 0, 0,   -1, 0,   0, 0, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 16, 0, 16, 0, 0, 0,   0,  0,   0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0,  0, 0,  0, 0, 0,   0,  0,   0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0,  0, 0,  0, 0, 0,   0,  0,   0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0,  0, 0,  0, 0, 0,   0,  0,   0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0,  0, 0,  0, 0, 0,   0,  0,   0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0,  0, 0,  0, 0, 0,   0,  0,   0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0,  0, 0,  0, 0, 0,   0,  0,   0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0,  0, 0,  0, 0, 0,   0,  0,   0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0,  0, 0,  0, 0, 0,   0,  0,   0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0,  0, 0,  0, 0, 0,   0,  0,   0, 0, 0, 0,
    0, 0, 0, 0, 0, 0};

// MVV/LVA Most Valuable Victim Least Valuable Attacker
// 每种子力的价值，车马炮兵象士将
constexpr Score MVVLVA[16] = {0, 4, 3, 3, 2, 1, 1, 5, 0, 4, 3, 3, 2, 1, 1, 5};

// 定义用于生成fen的map
FenMap CHESS_FEN;

// 吃子走法标志
constexpr MoveFlag CAPTURE{true};

// 空着裁剪标志
constexpr NullFlag NO_NULL{false};

// 将的步长, 上下左右
constexpr Delta KING_DELTA[4]{-16, 16, -1, 1};

// 士的步长，斜线
constexpr Delta ADVISOR_DELTA[4]{-17, -15, 15, 17};

// 象的步长，二步斜线
constexpr Delta BISHOP_DELTA[4]{-34, -30, 30, 34};

// 象眼的位置，与士的步长相同
constexpr const Delta (&BISHOP_PIN)[4]{ADVISOR_DELTA};

// 车(炮),以及飞将的步长
constexpr const Delta (&LINE_CHESS_DELTA)[4]{KING_DELTA};

// 马的步长，日字走法
constexpr Delta KNIGHT_DELTA[4][2]{
    {-31, -33}, // 和马腿 -16 对应
    {31, 33},   // 和马腿 16 对应
    {14, -18},  // 和马腿 -1 对应
    {-14, 18},  // 和马腿 1 对应
};

// 马腿的位置，与将的步长相同
constexpr const Delta (&KNIGHT_PIN)[4]{KING_DELTA};

// 过河红兵的步长
constexpr Delta PROMOTED_RED_PAWN_DELTA[3]{-16, -1, 1};

// 过河黑兵的步长
constexpr Delta PROMOTED_BLACK_PAWN_DELTA[3]{16, -1, 1};

// 没过河的红兵
constexpr Delta RED_PAWN_DELTA{-16};

// 没过河的黑兵
constexpr Delta BLACK_PAWN_DELTA{16};

// 被将军时马的步长，日字走法
constexpr Delta CHECK_KNIGHT_DELTA[4][2]{
    {-18, -33}, // 和被将军时马腿 -17 对应
    {-31, -14}, // 和被将军时马腿 -15 对应
    {14, 31},   // 和被将军时马腿 15 对应
    {18, 33},   // 和被将军时马腿 17 对应
};

// 被将军时马腿的位置,与士的步长相同
const Delta (&CHECK_KNIGHT_PIN)[4]{ADVISOR_DELTA};

// 所以红将可能出现的位置
constexpr Position RED_KING_POSITION[9]{166, 167, 168, 182, 183,
                                        184, 198, 199, 200};

// 所以黑将可能出现的位置
constexpr Position BLACK_KING_POSITION[9]{54, 55, 56, 70, 71, 72, 86, 87, 88};

// 边指示器，指示是红方还是黑方
Side COMPUTER_SIDE{BLACK};

// 棋盘上每一个位置的Zobrist值表
Zobrist CHESS_ZOBRIST;

// 选边的Zobrist值，换边走棋时就要异或这个值
ZobristValue SIDE_ZOBRIST;

// 置换表
HashTable HASH_TABLE;

// 置换表锁
QReadWriteLock HASH_TABLE_LOCK;

// 定义搜索时间
Clock SEARCH_TIME{1000};

// 定义先行棋的分数
constexpr Score ADVANCED_SCORE{3};

// 定义输棋的分数
constexpr Score LOSS_SCORE{-1000};

// 定义和棋的分数
constexpr Score DRAW_SCORE{-20};

// 定义赢棋的分数
constexpr Score MATE_SCORE{1000};

// 长将判负的分值，低于该值将不写入置换表
constexpr Score BAN_SCORE_MATE{950};

// 长将判负的分值，高于该值将不写入置换表
constexpr Score BAN_SCORE_LOSS{-950};

// 搜索出赢棋的分值界限，超出此值就说明已经搜索出杀棋了
constexpr Score WIN_SCORE{900};

// 搜索出输棋的分值界限，超出此值就说明已经搜索出杀棋了
constexpr Score LOST_SCORE{-900};

// 最大并发数，现代cpu一般为超线程cpu，所以要除以2获得物理核心数
Count MAX_CONCURRENT{static_cast<Count>(thread::hardware_concurrency() / 2)};

// 电脑搜索的深度
Depth CURRENT_DEPTH{0};

// 定义全局线程局面
PositionInfo POSITION_INFO{};

// 用于拆分走法，取高8位
inline Position getSrc(const Move move) { return move >> 8; }

// 用于拆分走法，取低8位
inline Position getDest(const Move move) { return move & 0xFF; }

// 用于合并走法
inline Move toMove(const Position from, const Position to) {
  return from << 8 | to;
}

// 用于获取一个走法的MVVLVA值，数值越大说明越值
inline Score getMVVLVA(const Move move) {
  return (MVVLVA[getDest(move)] << 3) - MVVLVA[getSrc(move)];
}

// 走法是否符合帅(将)的步长
inline bool isKingSpan(const Move move) {
  return LEGAL_SPAN[256 + getDest(move) - getSrc(move)] == 1;
}

// 走法是否符合仕(士)的步长
inline bool isAdvisorSpan(const Move move) {
  return LEGAL_SPAN[256 + getDest(move) - getSrc(move)] == 2;
}

// 走法是否符合相(象)的步长
inline bool isBishopSpan(const Move move) {
  return LEGAL_SPAN[256 + getDest(move) - getSrc(move)] == 3;
}

// 返回兵向前走一步的位置
inline Position getPawnForwardPos(const Position pos, const Side side) {
  if (side == RED) {
    // 如果是红方，兵向前走
    return pos + RED_PAWN_DELTA;
  } else {
    // 如果是黑方，兵向前走
    return pos + BLACK_PAWN_DELTA;
  }
}

// 象眼的位置
inline Position getBishopPinPos(const Move move) {
  return (getSrc(move) + getDest(move)) >> 1;
}

// 马腿的位置
inline Position getKnightPinPos(const Move move) {
  return getSrc(move) +
         LEGAL_KNIGHT_PIN_SPAN[256 + getDest(move) - getSrc(move)];
}

// 是否在同一边
inline bool isSameHalf(const Move move) {
  return ((getSrc(move) ^ getDest(move)) & 0x80) == 0;
}

// 是否在同一行
inline bool isSameRow(const Move move) {
  return ((getSrc(move) ^ getDest(move)) & 0xF0) == 0;
}

// 是否在同一列
inline bool isSameColumn(const Move move) {
  return ((getSrc(move) ^ getDest(move)) & 0x0F) == 0;
}

// 用于计分的翻转
inline Position flipPosition(const Position pos) { return 254 - pos; }

// 重置置换表与深度信息
inline void resetCache(PositionInfo &positionInfo) {
  // 重置深度信息
  positionInfo.mDistance = 0;
  // 重置置换表
  memset(HASH_TABLE, 0, sizeof(HASH_TABLE));
}

// 局面信息的构造函数
PositionInfo::PositionInfo() {
  // 初始化棋盘
  memcpy(this->mChessBoard, INIT_BOARD, 256);
  //清空表
  memset(this->mHistory, 0, sizeof(this->mHistory));
  memset(this->mKiller, 0, sizeof(this->mKiller));
  // 初始化棋盘的Zobrist，每一个位置 51 - 203
  for (Position pos{51}; pos < 204; ++pos) {
    // 如果有棋子就异或上棋子的Zobrist值
    if (this->mChessBoard[pos]) {
      this->mZobrist ^= CHESS_ZOBRIST[this->mChessBoard[pos]][pos];
    }
  }
}

// 重置棋盘
void PositionInfo::resetBoard() {
  // 初始化棋盘
  memcpy(this->mChessBoard, INIT_BOARD, 256);
  // 初始化距离根节点的深度信息
  this->mDistance = 0;
  // 清空表
  memset(this->mHistory, 0, sizeof(this->mHistory));
  memset(this->mKiller, 0, sizeof(this->mKiller));
  // 历史表大小置0
  this->mHistorySize = 0;
  // 清空历史走法表
  memset(this->mHistoryMove, 0, sizeof(this->mHistoryMove));
  // 清空Zobrist
  this->mZobrist = 0;
  // 初始化棋盘的Zobrist，每一个位置 51 - 203
  for (Position pos{51}; pos < 204; ++pos) {
    // 如果有棋子就异或上棋子的Zobrist值
    if (this->mChessBoard[pos]) {
      this->mZobrist ^= CHESS_ZOBRIST[this->mChessBoard[pos]][pos];
    }
  }
  // 清空BestMove
  this->mBestMove = INVALID_MOVE;
  // 初始化分数
  this->mRedScore = this->mBlackScore = 888;
}

// 局面信息复制，多线程关键
void PositionInfo::setThreadPositionInfo(const PositionInfo &positionInfo) {
  // 拷贝棋盘
  memcpy(this->mChessBoard, positionInfo.mChessBoard,
         sizeof(this->mChessBoard));
  // 拷贝历史走法
  memcpy(this->mHistoryMove, positionInfo.mHistoryMove,
         sizeof(this->mHistoryMove));
  // 拷贝分数
  this->mRedScore = positionInfo.mRedScore;
  this->mBlackScore = positionInfo.mBlackScore;
  // 拷贝Zobrist
  this->mZobrist = positionInfo.mZobrist;
  // 拷贝大小
  this->mHistorySize = positionInfo.mHistorySize;
}

// 判断该棋子是红方棋子还是黑方棋子, 不能传入空，否则当作红方子力处理
inline Side PositionInfo::getChessSide(const Position pos) {
  return this->mChessBoard[pos] & 0b1000;
};

// 该位置是否是空的
inline bool PositionInfo::isEmpty(const Position pos) {
  return not this->mChessBoard[pos];
}

// 该位置是否是对方的棋子
inline bool PositionInfo::isOppChess(const Position pos, const Side side) {
  return this->mChessBoard[pos] and getChessSide(pos) not_eq side;
}

// 该位置是否为空或为对方的棋子
inline bool PositionInfo::isEmptyOrOppChess(const Position pos,
                                            const Side side) {
  return isEmpty(pos) or getChessSide(pos) not_eq side;
}

// 获得对手的选边
inline Side getOppSide(const Side side) { return side ^ 0b1000; }

// 将一个棋子转化为对应边的棋子
inline Chess toSideChess(const Chess chess, const Side side) {
  return chess + side;
}

// 是否在自己的半边，提供越界检查
inline bool isMyHalf(const Position pos, const Side side) {
  if (side == RED) {
    return IN_RED_HALF[pos];
  } else {
    return IN_BLACK_HALF[pos];
  }
}

// 是否在对手的半边，提供越界检查
inline bool isOppHalf(const Position pos, const Side side) {
  if (side == RED) {
    return IN_BLACK_HALF[pos];
  } else {
    return IN_RED_HALF[pos];
  }
}

// 获取棋盘上的将
Position PositionInfo::getKING(const Side side) {
  if (side == RED) {
    for (Position pos : RED_KING_POSITION) {
      if (this->mChessBoard[pos] == RED_KING) {
        return pos;
      }
    }
  } else {
    for (Position pos : BLACK_KING_POSITION) {
      if (this->mChessBoard[pos] == BLACK_KING) {
        return pos;
      }
    }
  }
  return {};
}

// 判断该位置为否被将军,side为走棋方
bool PositionInfo::isChecked(const Side side) {
  // 首先找到棋盘上的将
  Position pos{getKING(side)};
  // 获得对方的sideTag
  Side oppSideTag{getOppSide(side)};

  // 判断是否被兵将军
  for (const auto &delta :
       side == RED ? PROMOTED_RED_PAWN_DELTA : PROMOTED_BLACK_PAWN_DELTA) {
    // 检查对应位置是否在棋盘内,对应的位置上是否有棋子,看一下这个棋子是不是对方的兵
    if (this->mChessBoard[pos + delta] and
        getChessSide(pos + delta) not_eq side and
        this->mChessBoard[pos + delta] == toSideChess(PAWN, oppSideTag)) {
      return true;
    }
  }

  // 判断是否被马将军
  for (Index index{0}; index < 4; ++index) {
    // 先看马腿有没有在棋盘里，并且有没有憋马腿
    // 马腿超出范围或者蹩马腿了就直接下一轮，避免在下一个for循环里面判断两次
    if (IN_BOARD[pos + CHECK_KNIGHT_PIN[index]] and
        isEmpty(pos + CHECK_KNIGHT_PIN[index])) {
      // 遍历每一个马步
      for (const auto &delta : CHECK_KNIGHT_DELTA[index]) {
        // 检查对应位置是否在棋盘内,对应的位置上是否有棋子,是否被蹩马腿,看一下这个棋子是不是对方的马
        if (this->mChessBoard[pos + delta] and
            getChessSide(pos + delta) not_eq side and
            this->mChessBoard[pos + delta] == toSideChess(KNIGHT, oppSideTag)) {
          return true;
        }
      }
    }
  }

  // 判断是否被车(炮)将军，包括将帅对脸
  for (const auto &delta : LINE_CHESS_DELTA) {
    Position cur{static_cast<Position>(pos + delta)};
    // 检查车(将)
    while (IN_BOARD[cur]) {
      if (this->mChessBoard[cur]) {
        if (this->mChessBoard[cur] == toSideChess(ROOK, oppSideTag) or
            this->mChessBoard[cur] == toSideChess(KING, oppSideTag)) {
          return true;
        }
        cur += delta;
        break;
      }
      cur += delta;
    }
    // 检查炮
    while (IN_BOARD[cur]) {
      if (this->mChessBoard[cur]) {
        if (this->mChessBoard[cur] == toSideChess(CANNON, oppSideTag)) {
          return true;
        }
        break;
      }
      cur += delta;
    }
  }
  return false;
}

// 判断一个走法是否是合法的走法
bool PositionInfo::isLegalMove(const Move move, const Side side) {
  // 定义要用到的变量
  Position src{getSrc(move)}, dest{getDest(move)};

  // 如果起始的位置不是自己的棋子或者终点的位置是自己的棋子就返回假
  if (isEmpty(src) or getChessSide(src) not_eq side or
      (this->mChessBoard[dest] and getChessSide(dest) == side)) {
    return false;
  }

  // 定义要用到的变量
  Delta delta;
  Chess chess{static_cast<Chess>(this->mChessBoard[src] - side)};
  // 根据情况判断走法是否合法
  switch (chess) {
    // 是否在九宫内，是否符合将的步长
  case KING:
    return IN_SQRT[dest] and isKingSpan(move);
    // 是否在九宫内，是否符合士的步长
  case ADVISOR:
    return IN_SQRT[dest] and isAdvisorSpan(move);
    // 是否在同一边，是否符合象的步长，是否塞象眼
  case BISHOP:
    return isSameHalf(move) and isBishopSpan(move) and
           isEmpty(getBishopPinPos(move));
    // 是否符合马的步长，是否憋马腿
  case KNIGHT:
    return getKnightPinPos(move) not_eq src and isEmpty(getKnightPinPos(move));
  case ROOK:
  case CANNON:
    // 判断是否在同一行
    if (isSameRow(move)) {
      if (src > dest) {
        delta = -1;
      } else {
        delta = 1;
      }
      // 是否在同一列
    } else if (isSameColumn(move)) {
      if (src > dest) {
        delta = -16;
      } else {
        delta = 16;
      }
    } else {
      return false;
    }
    // 往前搜索第一个棋子
    src += delta;
    while (src not_eq dest and isEmpty(src)) {
      src += delta;
    }
    if (src == dest) {
      // _:空 X:目标棋子 O:中间棋子
      // 到头了，看看最终位置是否为空，也就是这些情况: 车------->_ | 炮------->_
      // 如果不为空，看看走的棋子是否为车（车吃子），也就是这种情况: 车------->X
      return isEmpty(dest) or chess == ROOK;
    } else if (this->mChessBoard[dest] and chess == CANNON) {
      // 没有到头，但是目标位置有对方的棋子，并且我们走的棋子是炮就继续判断
      src += delta;
      // 往前找第二个棋子
      while (src not_eq dest and isEmpty(src)) {
        src += delta;
      }
      // 如果到头了，看看尽头是不是棋子，也就是这种情况: 炮---O--->X
      return src == dest;
    }
    // 其他的所有不符合的情况，形如下:
    // 车---O--->_ | 车---O--->X
    // 炮---O--->_ | 炮--O-O-->X
    return false;
  case PAWN:
    // 如果过河了，并且是左右行走
    if (isOppHalf(src, side) and (dest - src == 1 or dest - src == -1)) {
      return true;
    }
    // 如果没过河只能向前走，如果过河了就补上向前走的一步
    return dest == getPawnForwardPos(src, side);
  default:
    return false;
  }
}

// 检查重复走法
inline RepeatFlag PositionInfo::getRepeatFlag() {
  // mySide代表的是是否是调用本函数的那一方(下称"我方")
  // 因为一调用搜索就马上调用了本函数，我方没有走棋
  // 所以在检查重复走法时，历史走法表中最后一项保存的是对方的最后一步
  // 所以这个变量的初始值为假，代表这一步不是我方，因为走法从后往前遍历
  bool mySide{false};
  // 我方是否将军，对方是否将军
  bool myCheck{true}, oppCheck{true};
  // 指向历史走法表的最后一项，往前遍历
  MoveItem *move = this->mHistoryMove + this->mHistorySize - 1;
  // 必须保证步法有效，也就是没有到头部哨兵或者空步裁剪处
  // 如果遇到空步裁剪就不往下检测了，因为空步无法算作有效步
  // 并且要求不是吃子步，因为吃子就打破长将了
  while (move->mMove not_eq INVALID_MOVE and not move->mVictim) {
    if (mySide) {
      // 如果是我方，更新我方将军信息
      myCheck &= move->mCheck;
      // 如果检测到局面与当前局面重复就返回状态码
      if (move->mZobrist == this->mZobrist) {
        return 1 + (myCheck ? 2 : 0) + (oppCheck ? 4 : 0);
      }
    } else {
      // 如果是对方，更新对方的将军信息
      oppCheck &= move->mCheck;
    }
    // 更新选边信息
    mySide = not mySide;
    // move指向前一个走法
    --move;
  }
  // 没有重复局面
  return false;
}

// 返回和棋的分值
inline Score PositionInfo::getDrawScore() {
  // 无论如何都要使得和棋对于第一层的那一方来说是不利的，是负分
  // DISTANCE & 1 的作用是确定现在在那一层
  // 说明evaluate的那一层和第一层是同一方
  // 同一方返回负值，不同方返回正值，这样正值上到第一层就会变成负值
  return this->mDistance & 1 ? -DRAW_SCORE : DRAW_SCORE;
}

// 从重复检查状态码中提取分数
inline Score PositionInfo::scoreRepeatFlag(const RepeatFlag repeatFlag) {
  // 我方长将返回负分，对方长将返回正分
  Score score = (repeatFlag & 2 ? BAN_SCORE_LOSS + this->mDistance : 0) +
                (repeatFlag & 4 ? BAN_SCORE_MATE - this->mDistance : 0);
  if (score == 0) {
    // 如果双方都长将或者双方都没有长将但是有重复局面
    return getDrawScore();
  } else {
    // 有一方长将
    return score;
  }
}

// 计算红方走了某个位置后的分数
inline void PositionInfo::calcRedMove(const Move move) {
  Position src{getSrc(move)}, dest{getDest(move)};
  Chess srcChess{this->mChessBoard[src]}, destChess{this->mChessBoard[dest]};
  // 位置分计算
  this->mRedScore += RED_VALUE[srcChess][dest] - RED_VALUE[srcChess][src];
  // Zobrist计算
  this->mZobrist ^= CHESS_ZOBRIST[srcChess][src];
  this->mZobrist ^= CHESS_ZOBRIST[srcChess][dest];
  if (this->mChessBoard[dest]) {
    this->mBlackScore -= RED_VALUE[destChess - BLACK][flipPosition(dest)];
    this->mZobrist ^= CHESS_ZOBRIST[destChess][dest];
  }
}

// 计算黑方走了某个位置后的分数
inline void PositionInfo::calcBlackMove(const Move move) {
  Position src{getSrc(move)}, dest{getDest(move)};
  Chess srcChess{this->mChessBoard[src]}, destChess{this->mChessBoard[dest]};
  // 位置分计算
  this->mBlackScore += RED_VALUE[srcChess - BLACK][flipPosition(dest)] -
                       RED_VALUE[srcChess - BLACK][flipPosition(src)];
  // Zobrist计算
  this->mZobrist ^= CHESS_ZOBRIST[srcChess][src];
  this->mZobrist ^= CHESS_ZOBRIST[srcChess][dest];
  // 吃子要减掉对应的分数
  if (this->mChessBoard[dest]) {
    this->mRedScore -= RED_VALUE[destChess][dest];
    this->mZobrist ^= CHESS_ZOBRIST[destChess][dest];
  }
}

// 计算红方还原了某一步棋后的分数
inline void PositionInfo::calcRedUnMove(const Move move, const Chess victim) {
  Position src{getSrc(move)}, dest{getDest(move)};
  Chess destChess{this->mChessBoard[dest]};
  // 位置分计算
  this->mRedScore += RED_VALUE[destChess][src] - RED_VALUE[destChess][dest];
  // Zobrist计算
  this->mZobrist ^= CHESS_ZOBRIST[destChess][dest];
  this->mZobrist ^= CHESS_ZOBRIST[destChess][src];
  // 把吃掉的子的分数加回去
  if (victim not_eq EMPTY) {
    this->mBlackScore += RED_VALUE[victim - BLACK][flipPosition(dest)];
    this->mZobrist ^= CHESS_ZOBRIST[victim][dest];
  }
}

// 计算黑方还原了某一步棋后的分数
inline void PositionInfo::calcBlackUnMove(const Move move, const Chess victim) {
  Position src{getSrc(move)}, dest{getDest(move)};
  Chess destChess{this->mChessBoard[dest]};
  // 位置分计算
  this->mBlackScore += RED_VALUE[destChess - BLACK][flipPosition(src)] -
                       RED_VALUE[destChess - BLACK][flipPosition(dest)];
  // Zobrist计算
  this->mZobrist ^= CHESS_ZOBRIST[destChess][dest];
  this->mZobrist ^= CHESS_ZOBRIST[destChess][src];
  // 把吃掉的子的分数加回去
  if (victim not_eq EMPTY) {
    this->mRedScore += RED_VALUE[victim][dest];
    this->mZobrist ^= CHESS_ZOBRIST[victim][dest];
  }
}

// 计算走一步的分数和Zobrist
inline void PositionInfo::calcMove(const Move move, const Side side) {
  // 肯定换边了
  this->mZobrist ^= SIDE_ZOBRIST;
  if (side == RED) {
    // 如果是红方走棋
    calcRedMove(move);
  } else {
    // 如果是黑方走棋
    calcBlackMove(move);
  }
}

// 计算撤销走一步的分数和Zobrist
inline void PositionInfo::calcUnMove(const Move move, const Chess victim,
                                     const Side side) {
  // 肯定换边了
  this->mZobrist ^= SIDE_ZOBRIST;
  if (side == RED) {
    // 如果是红方走棋
    calcRedUnMove(move, victim);
  } else {
    // 如果是黑方走棋
    calcBlackUnMove(move, victim);
  }
}

inline Score PositionInfo::evaluate(const Side side) {
  /*
    为何此处要额外加上一个ADVANCED_SCORE先行棋分？因为执行该函数时本身是轮到该层玩家走棋，
    但是因为各种原因只能搜索到这里了，该层玩家没有走棋而直接返回了这个局面下自己的评分!
    实际上这样对他的评价是不够正确的，所以加上一个补偿分数，代表下一步是该玩家先行，使得评价更公正一些!
  */
  if (side == RED) {
    return this->mRedScore - this->mBlackScore + ADVANCED_SCORE;
  } else {
    return this->mBlackScore - this->mRedScore + ADVANCED_SCORE;
  }
}

// 搜索置换表
Score PositionInfo::probeHash(Score alpha, Score beta, Depth depth,
                              Move &hashMove) {
  // 杀棋的标志，如果杀棋了就不用满足深度条件
  bool isMate{false};
  // 提取置换表项，注意到这里不取引用，因为下面要对分数进行修改
  HASH_TABLE_LOCK.lockForRead();
  HashItem hashItem = HASH_TABLE[this->mZobrist & HASH_MASK];
  HASH_TABLE_LOCK.unlock();
  // 校验高位zobrist是否对应得上
  if (this->mZobrist not_eq hashItem.mZobrist) {
    hashMove = INVALID_MOVE;
    return LOSS_SCORE;
  }
  // 将走法保存下来
  hashMove = hashItem.mMove;
  if (hashItem.mScore > WIN_SCORE) {
    if (hashItem.mScore < BAN_SCORE_MATE) {
      // 有可能会导致搜索的不稳定性，因为这个长将的分数可能来自于另外一条不同的搜索分支
      return LOSS_SCORE;
    }
    // 真的赢了
    isMate = true;
    // 给分数添加最短层数信息
    hashItem.mScore -= this->mDistance;
  } else if (hashItem.mScore < LOST_SCORE) {
    if (hashItem.mScore > BAN_SCORE_LOSS) {
      // 理由同上
      return LOSS_SCORE;
    }
    // 真的输了
    isMate = true;
    // 给分数添加最短层数信息
    hashItem.mScore += this->mDistance;
  }
  // 判断该走法是否满足深度条件，即结果比当前的搜索层数同层或者更深
  // 如果赢了就不用满足深度条件
  if (hashItem.mDepth >= depth || isMate) {
    if (hashItem.mFlag == HASH_BETA) {
      /*
       * 如果是beta走法，说明在同层或高层中搜索该局面走该走法时发生了beta截断
       * 那么查看一下在当前的beta下走该走法是否也可以发生截断，既score >= beta
       * 即是否超出alpha-beta的边界(alpha, beta)(here)
       * 如果摸到或超过当前beta边界即可截断
       * 如果没有超过beta，那么不能直接返回score
       * 因为既然同层或高层发生了beta截断，那么就有可能没有搜索完该局面下的所有走法，分数不一定具备参考性
       * 但是这个走法能直接返回，因为有助于更快速的缩小当前的alpha-beta范围
       */
      if (hashItem.mScore >= beta) {
        return hashItem.mScore;
      } else {
        return LOSS_SCORE;
      }
    } else if (hashItem.mFlag == HASH_ALPHA) {
      /*
       * 如果是alpha走法，说明在同层或者上层的搜索中遍历了局面中的所有走法
       * 并且得到的最好的走法是alpha走法，分数无法超过那层的alpha
       * 那么查看一下在当前的alpha下是否也无法超过当前的alpha，既score <= alpha
       * 即是否超出alpha-beta的边界(here)(alpha, beta)
       * 如果摸到或小于当前alpha即可截断
       * 如果大于alpha，那么不能直接返回score
       * 因为如果它在alpha-beta范围内，那么它就不是alpha走法，值得搜索一下
       * 返回该走法有助于更快速的缩小当前的alpha-beta范围
       * 如果它甚至超过了beta，那么这个走法就可以发生beta截断
       */
      if (hashItem.mScore <= alpha) {
        return hashItem.mScore;
      } else {
        return LOSS_SCORE;
      }
    }
    /*
     * 在同层或者上层的搜索中遍历了局面中的所有走法，找到了pv走法，所以就是这个分数了！直接返回即可。
     */
    return hashItem.mScore;
  }
  // 不满足深度条件并且不是杀棋
  return LOSS_SCORE;
}

// 保存到置换表
void PositionInfo::recordHash(HashFlag hashFlag, Score score, Depth depth,
                              Move move) {
  // 获取置换表项
  HashItem &hashItem = HASH_TABLE[this->mZobrist & HASH_MASK];
  // 查看置换表中的项是否比当前项更加准确
  if (hashItem.mDepth > depth) {
    return;
  }
  // 不然就保存置换表标志和深度
  hashItem.mFlag = hashFlag;
  hashItem.mDepth = depth;
  if (score > WIN_SCORE) {
    if (move == INVALID_MOVE && score <= BAN_SCORE_MATE) {
      // 可能导致搜索的不稳定性，并且没有最佳着法，立刻退出
      return;
    }
    // 否则就记录分数，消除分数的最短层数信息
    hashItem.mScore = score + this->mDistance;
  } else if (score < LOST_SCORE) {
    if (move == INVALID_MOVE && score >= BAN_SCORE_LOSS) {
      // 同上
      return;
    }
    hashItem.mScore = score - this->mDistance;
  } else {
    // 不是杀棋时直接记录分数
    hashItem.mScore = score;
  }
  // 记录走法
  hashItem.mMove = move;
  // 记录Zobrist
  hashItem.mZobrist = this->mZobrist;
}

// 用于设置历史表、杀手表
inline void PositionInfo::setBestMove(const Move move, const Depth depth) {
  if (this->mKiller[this->mDistance][0] not_eq move) {
    this->mKiller[this->mDistance][1] = this->mKiller[this->mDistance][0];
    this->mKiller[this->mDistance][0] = move;
  }
  this->mHistory[move] += depth * depth;
}

// 走法生成
Count PositionInfo::moveGen(MoveList &moves, const Side side,
                            const MoveFlag capture) {
  Count totalMoves{0};
  // 遍历棋盘
  for (Position start{51}; start < 204; start += 16) {
    for (Position pos{start}; pos < start + 9; ++pos) {
      // 找到自己的棋子
      if (this->mChessBoard[pos] and getChessSide(pos) == side) {
        switch (this->mChessBoard[pos] - side) {
        // 如果是将
        case KING:
          for (const auto &delta : KING_DELTA) {
            // 对于每一个步长，查看上面有没有自己的棋子
            if (IN_SQRT[pos + delta] and
                (capture ? isOppChess(pos + delta, side)
                         : isEmptyOrOppChess(pos + delta, side))) {
              // 没有棋子或者不是自己的棋子
              moves[totalMoves++] = toMove(pos, pos + delta);
            }
          }
          break;

        // 如果是士
        case ADVISOR:
          for (const auto &delta : ADVISOR_DELTA) {
            // 对于每一个步长，查看上面有没有自己的棋子
            if (IN_SQRT[pos + delta] and
                (capture ? isOppChess(pos + delta, side)
                         : isEmptyOrOppChess(pos + delta, side))) {
              // 没有棋子或者不是自己的棋子
              moves[totalMoves++] = toMove(pos, pos + delta);
            }
          }
          break;

        // 如果是象
        case BISHOP:
          for (Index index{0}; index < 4; ++index) {
            Delta delta = BISHOP_DELTA[index];
            Delta pin = BISHOP_PIN[index];
            // 对于每一个步长，先看是否在自己的半边内，有没有塞住象眼，再查看上面有没有自己的棋子
            if (isMyHalf(pos + delta, side) and isEmpty(pos + pin) and
                (capture ? isOppChess(pos + delta, side)
                         : isEmptyOrOppChess(pos + delta, side))) {
              // 没有棋子或者不是自己的棋子
              moves[totalMoves++] = toMove(pos, pos + delta);
            }
          }
          break;

        // 如果是马
        case KNIGHT:
          for (Index index{0}; index < 4; ++index) {
            // 先看马腿有没有在棋盘里，并且有没有憋马腿
            // 马腿超出范围或者蹩马腿了就直接下一轮，避免在下一个for循环里面判断两次
            if (IN_BOARD[pos + KNIGHT_PIN[index]] and
                isEmpty(pos + KNIGHT_PIN[index])) {
              for (const auto &delta : KNIGHT_DELTA[index]) {
                // 对于每一个步长，先看是否在棋盘内，再查看上面有没有自己的棋子
                if (IN_BOARD[pos + delta] and
                    (capture ? isOppChess(pos + delta, side)
                             : isEmptyOrOppChess(pos + delta, side))) {
                  // 没有棋子或者不是自己的棋子
                  moves[totalMoves++] = toMove(pos, pos + delta);
                }
              }
            }
          }
          break;

        // 如果是兵
        case PAWN:
          if (isMyHalf(pos, side)) {
            // 如果没过河
            Delta delta{getChessSide(pos) == RED ? RED_PAWN_DELTA
                                                 : BLACK_PAWN_DELTA};
            // 检查所到之处是否在棋盘内，是否被自己的棋子堵住了
            if (IN_BOARD[pos + delta] and
                (capture ? isOppChess(pos + delta, side)
                         : isEmptyOrOppChess(pos + delta, side))) {
              moves[totalMoves++] = toMove(pos, pos + delta);
            }
          } else {
            // 过河了
            for (const auto &delta : getChessSide(pos) == RED
                                         ? PROMOTED_RED_PAWN_DELTA
                                         : PROMOTED_BLACK_PAWN_DELTA) {
              // 检查所到之处是否在棋盘内，是否被自己的棋子堵住了
              if (IN_BOARD[pos + delta] and
                  (capture ? isOppChess(pos + delta, side)
                           : isEmptyOrOppChess(pos + delta, side))) {
                moves[totalMoves++] = toMove(pos, pos + delta);
              }
            }
          }
          break;

        // 如果是车
        case ROOK:
          for (const auto &delta : LINE_CHESS_DELTA) {
            Position cur{static_cast<Position>(pos + delta)};
            // 如果该位置为空
            while (IN_BOARD[cur] and isEmpty(cur)) {
              if (not capture) {
                moves[totalMoves++] = toMove(pos, cur);
              }
              cur += delta;
            }
            // 可能超出棋盘或者碰到了一个棋子
            if (IN_BOARD[cur] and getChessSide(cur) not_eq side) {
              moves[totalMoves++] = toMove(pos, cur);
            }
          }
          break;

        // 如果是炮
        case CANNON:
          for (const auto &delta : LINE_CHESS_DELTA) {
            Position cur{static_cast<Position>(pos + delta)};
            // 如果该位置为空
            while (IN_BOARD[cur] and isEmpty(cur)) {
              if (not capture) {
                moves[totalMoves++] = toMove(pos, cur);
              }
              cur += delta;
            }
            // 碰到了第一个棋子或者超出棋盘
            cur += delta;
            while (IN_BOARD[cur] and isEmpty(cur)) {
              cur += delta;
            }
            // 碰到了第二个棋子或者超出棋盘
            if (IN_BOARD[cur] and getChessSide(cur) not_eq side) {
              moves[totalMoves++] = toMove(pos, cur);
            }
          }
          break;
        }
      }
    }
  }
  return totalMoves;
}

// 撤销走棋
inline void PositionInfo::unMakeMove(const Move move, const Side side) {
  // 与根节点的距离-1
  --this->mDistance;
  Position src = getSrc(move), dest = getDest(move);
  // 获取被吃掉的棋子，并自减历史走法表大小
  Chess victim = this->mHistoryMove[--this->mHistorySize].mVictim;
  // 还原分数和Zobrist
  calcUnMove(move, victim, side);
  this->mChessBoard[src] = this->mChessBoard[dest];
  this->mChessBoard[dest] = victim;
}

// 走棋,返回是否走成功
inline bool PositionInfo::makeMove(const Move move, const Side side) {
  // 与根节点的距离加1
  ++this->mDistance;
  // 获取历史走法表项，并将自增走法历史表的大小
  MoveItem &moveItem = this->mHistoryMove[this->mHistorySize++];
  Position src = getSrc(move), dest = getDest(move);
  // 保存受害者
  moveItem.mVictim = this->mChessBoard[dest];
  // 保存Zobrist
  moveItem.mZobrist = this->mZobrist;
  // 计算分数和Zobrist
  calcMove(move, side);
  // 走棋
  this->mChessBoard[dest] = this->mChessBoard[src];
  this->mChessBoard[src] = EMPTY;
  // 检查自己是否被将军
  if (isChecked(side)) {
    // 如果被将军了就复原
    unMakeMove(move, side);
    return false;
  }
  // 否则就保存本步棋是否将军对方的信息
  moveItem.mCheck = isChecked(getOppSide(side));
  // 保存走法信息
  moveItem.mMove = move;
  return true;
}

// 当前是否可以走空步，即不在残局阶段
inline bool PositionInfo::isNullOk(const Side side) {
  if (side == RED) {
    return this->mRedScore > 400;
  } else {
    return this->mBlackScore > 400;
  }
}

// 走一步空步
inline void PositionInfo::makeNullMove() {
  // 与根节点的距离加1
  ++this->mDistance;
  // 获取历史走法表项，并将自增走法历史表的大小
  MoveItem &moveItem = this->mHistoryMove[this->mHistorySize++];
  // 保存受害者
  moveItem.mVictim = EMPTY;
  // 保存Zobrist
  moveItem.mZobrist = this->mZobrist;
  // 否则就保存本步棋是否将军对方的信息
  moveItem.mCheck = false;
  // 保存走法信息
  moveItem.mMove = INVALID_MOVE;
  // 换边，计算Zobrist
  this->mZobrist ^= SIDE_ZOBRIST;
}

// 撤销走一步空步
inline void PositionInfo::unMakeNullMove() {
  // 与根节点的距离-1
  --this->mDistance;
  // 自减历史走法表大小
  --this->mHistorySize;
  // 还原Zobrist
  this->mZobrist ^= SIDE_ZOBRIST;
}

SearchMachine::SearchMachine(PositionInfo &positionInfo, const Move hashMove,
                             const Side side)
    : mPositionInfo{positionInfo}, mSide{side}, mHash{hashMove} {
  // 获取两个杀手走法
  mKiller1 = this->mPositionInfo.mKiller[this->mPositionInfo.mDistance][0];
  mKiller2 = this->mPositionInfo.mKiller[this->mPositionInfo.mDistance][1];
}

Move SearchMachine::nextMove() {
  switch (mNowPhase) {
  case PHASE_HASH:
    // 指明下一个阶段
    this->mNowPhase = PHASE_KILLER1;
    // 确保这一个置换表走法不是默认走法
    if (this->mHash not_eq INVALID_MOVE) {
      return this->mHash;
    }
    // 否则就下一步
    [[fallthrough]];
  case PHASE_KILLER1:
    // 指明下一个阶段
    this->mNowPhase = PHASE_KILLER2;
    // 确保这一个杀手走法不是默认走法，不是置换表走法，并且要确认是否是合法的步
    if (this->mKiller1 not_eq INVALID_MOVE and
        this->mKiller1 not_eq this->mHash and
        this->mPositionInfo.isLegalMove(this->mKiller1, this->mSide)) {
      return this->mKiller1;
    }
    // 否则就下一步
    [[fallthrough]];
  case PHASE_KILLER2:
    // 指明下一个阶段
    this->mNowPhase = PHASE_GEN_MOVE;
    // 确保这一个杀手走法不是默认走法，不是置换表走法
    if (this->mKiller2 not_eq INVALID_MOVE and
        this->mKiller2 not_eq this->mHash and
        this->mPositionInfo.isLegalMove(this->mKiller2, this->mSide)) {
      return this->mKiller2;
    }
    // 否则就下一步
    [[fallthrough]];
  case PHASE_GEN_MOVE:
    // 指明下一个阶段
    this->mNowPhase = PHASE_ALL_MOVE;
    // 生成所有的走法并使用历史表对其进行排序
    this->mTotalMoves = this->mPositionInfo.moveGen(this->mMoves, this->mSide);
    sort(begin(this->mMoves), begin(this->mMoves) + this->mTotalMoves,
         [&](const Move &rhs, const Move &lhs) {
           return this->mPositionInfo.mHistory[lhs] <
                  this->mPositionInfo.mHistory[rhs];
         });
    // 直接下一步
    [[fallthrough]];
  case PHASE_ALL_MOVE:
    // 遍历走法，逐个检查并返回
    while (this->mNowIndex < this->mTotalMoves) {
      Move move{this->mMoves[mNowIndex++]};
      if (move not_eq this->mHash and move not_eq this->mKiller1 and
          move not_eq this->mKiller2) {
        return move;
      }
    }
    // 如果没有了就直接返回
    [[fallthrough]];
  default:
    return INVALID_MOVE;
  }
}

// 静态搜索
Score PositionInfo::searchQuiescence(Score alpha, const Score beta,
                                     const Side side) {
  // 先检查重复局面，获得重复局面标志
  RepeatFlag repeatFlag{getRepeatFlag()};
  if (repeatFlag) {
    // 如果有重复的情况，直接返回分数
    return scoreRepeatFlag(repeatFlag);
  }
  MoveList moves;
  Count totalMoves;
  Score bestScore{LOSS_SCORE};
  if (this->mHistoryMove[this->mHistorySize - 1].mCheck) {
    // 如果被将军了,生成全部走法
    totalMoves = moveGen(moves, side);
    // 按照历史表排序
    sort(begin(moves), begin(moves) + totalMoves,
         [&](const Move &rhs, const Move &lhs) {
           return this->mHistory[lhs] < this->mHistory[rhs];
         });
  } else {
    // 如果不被将军，先做局面评价
    Score tryScore = evaluate(side);
    if (tryScore > bestScore) {
      bestScore = tryScore;
      if (tryScore >= beta) {
        // Beta截断
        return tryScore;
      }
      if (tryScore > alpha) {
        // 缩小Alpha-Beta边界
        alpha = tryScore;
      }
    }

    // 如果局面评价没有截断，再生成吃子走法
    totalMoves = moveGen(moves, side, CAPTURE);
    // 根据MVVLVA对走法进行排序
    sort(begin(moves), begin(moves) + totalMoves,
         [&](const Move &rhs, const Move &lhs) {
           return getMVVLVA(lhs) < getMVVLVA(rhs);
         });
  }

  // 遍历所有走法
  for (Index index{0}; index < totalMoves; ++index) {
    Move move{moves[index]};
    // 如果被将军了就不搜索这一步
    if (makeMove(move, side)) {
      // 不然就获得评分并更新最好的分数
      Score tryScore{static_cast<Score>(
          -searchQuiescence(-beta, -alpha, getOppSide(side)))};
      // 撤销走棋
      unMakeMove(move, side);
      if (tryScore > bestScore) {
        // 找到最佳走法(但不能确定是Alpha、PV还是Beta走法)
        bestScore = tryScore;
        // 找到一个Beta走法
        if (tryScore >= beta) {
          // Beta截断
          return tryScore;
        }
        // 找到一个PV走法
        if (tryScore > alpha) {
          // 缩小Alpha-Beta边界
          alpha = tryScore;
        }
      }
    }
  }

  // 所有走法都搜索完了，返回最佳值
  if (bestScore == LOSS_SCORE) {
    // 如果是杀棋，就根据杀棋步数给出评价
    return LOSS_SCORE + this->mDistance;
  }

  // 否则就返回最佳值
  return bestScore;
}

// 完全局面搜索
Score PositionInfo::searchFull(Score alpha, const Score beta, const Depth depth,
                               const Side side, const NullFlag nullOk) {
  // 达到深度就返回静态评价，由于空着裁剪，深度可能小于-1
  if (depth <= 0) {
    return searchQuiescence(alpha, beta, side);
  }

  // 先检查重复局面，获得重复局面标志
  RepeatFlag repeatFlag{getRepeatFlag()};
  if (repeatFlag) {
    // 如果有重复的情况，直接返回分数
    return scoreRepeatFlag(repeatFlag);
  }

  // 当前走法
  Move move;
  // 尝试置换表裁剪，并得到置换表走法
  Score tryScore{probeHash(alpha, beta, depth, move)};
  if (tryScore > LOSS_SCORE) {
    // 置换表裁剪成功
    return tryScore;
  }

  // 进行空步裁剪，不能连着走两步空步，被将军时不能走空步
  // 并且在残局中不能走空步，不然会忽略“等招”这一重要策略
  // 根节点的Beta值是"MATE_SCORE"，所以不可能发生空步裁剪)
  if (nullOk and not this->mHistoryMove[this->mHistorySize - 1].mCheck and
      isNullOk(side)) {
    // 走一步空步
    makeNullMove();
    // 获得评分，深度减掉空着裁剪推荐的两层，然后本身走了一步空步，还要再减掉一层
    tryScore =
        -searchFull(-beta, 1 - beta, depth - 3, getOppSide(side), NO_NULL);
    // 撤销空步
    unMakeNullMove();
    if (tryScore >= beta) {
      // 如果足够好就可以发生截断
      return tryScore;
    }
  }

  // 搜索有限状态机
  SearchMachine search{*this, move, side};
  // 最佳走法的标志
  HashFlag bestMoveHashFlag{HASH_ALPHA};
  Score bestScore{LOSS_SCORE};
  Move bestMove{INVALID_MOVE};
  // 遍历所有走法
  while ((move = search.nextMove())) {
    // 如果被将军了就不搜索这一步
    if (makeMove(move, side)) {
      // 不然就获得评分并更新最好的分数
      // 将军延伸，如果将军了对方就多搜几步
      Depth newDepth{static_cast<Depth>(
          this->mHistoryMove[this->mHistorySize - 1].mCheck ? depth
                                                            : depth - 1)};
      // PVS
      if (bestScore == LOSS_SCORE) {
        tryScore = -searchFull(-beta, -alpha, newDepth, getOppSide(side));
      } else {
        tryScore = -searchFull(-alpha - 1, -alpha, newDepth, getOppSide(side));
        if (tryScore > alpha and tryScore < beta) {
          tryScore = -searchFull(-beta, -alpha, newDepth, getOppSide(side));
        }
      }
      // 撤销走棋
      unMakeMove(move, side);
      if (tryScore > bestScore) {
        // 找到最佳走法(但不能确定是Alpha、PV还是Beta走法)
        bestScore = tryScore;
        // 找到一个Beta走法
        if (tryScore >= beta) {
          // 更新走法标志
          bestMoveHashFlag = HASH_BETA;
          // Beta走法要保存到历史表
          bestMove = move;
          // Beta截断
          break;
        }
        // 找到一个PV走法
        if (tryScore > alpha) {
          // 更新走法标志
          bestMoveHashFlag = HASH_PV;
          // PV走法要保存到历史表
          bestMove = move;
          // 缩小Alpha-Beta边界
          alpha = tryScore;
        }
      }
    }
  }

  // 所有走法都搜索完了，把最佳走法(不能是Alpha走法)保存到历史表，返回最佳值
  if (bestScore == LOSS_SCORE) {
    // 如果是杀棋，就根据杀棋步数给出评价
    return LOSS_SCORE + this->mDistance;
  }

  // 记录到置换表
  HASH_TABLE_LOCK.lockForWrite();
  recordHash(bestMoveHashFlag, bestScore, depth, bestMove);
  HASH_TABLE_LOCK.unlock();
  if (bestMove not_eq INVALID_MOVE) {
    // 如果不是Alpha走法，就将最佳走法保存到历史表、杀手表、置换表
    setBestMove(bestMove, depth);
  }
  return bestScore;
}

// 根节点的搜索
Score PositionInfo::searchRoot(const Depth depth) {
  // 当前走法
  Move move;
  // 搜索有限状态机
  SearchMachine search{*this, INVALID_MOVE, COMPUTER_SIDE};
  Score bestScore{LOSS_SCORE};
  // 遍历所有走法
  while ((move = search.nextMove())) {
    // 如果被将军了就不搜索这一步
    if (makeMove(move, COMPUTER_SIDE)) {
      // 不然就获得评分并更新最好的分数
      Score tryScore;
      // 将军延伸，如果将军了对方就多搜几步
      Depth newDepth{static_cast<Depth>(
          this->mHistoryMove[this->mHistorySize - 1].mCheck ? depth
                                                            : depth - 1)};
      // PVS
      if (bestScore == LOSS_SCORE) {
        tryScore = -searchFull(LOSS_SCORE, MATE_SCORE, newDepth,
                               getOppSide(COMPUTER_SIDE), NO_NULL);
      } else {
        tryScore = -searchFull(-bestScore - 1, -bestScore, newDepth,
                               getOppSide(COMPUTER_SIDE));
        if (tryScore > bestScore) {
          tryScore = -searchFull(LOSS_SCORE, -bestScore, newDepth,
                                 getOppSide(COMPUTER_SIDE), NO_NULL);
        }
      }
      // 撤销走棋
      unMakeMove(move, COMPUTER_SIDE);
      if (tryScore > bestScore) {
        // 找到最佳走法
        bestScore = tryScore;
        this->mBestMove = move;
      }
    }
  }
  // 记录到置换表
  HASH_TABLE_LOCK.lockForWrite();
  recordHash(HASH_PV, bestScore, depth, this->mBestMove);
  HASH_TABLE_LOCK.unlock();
  setBestMove(this->mBestMove, depth);
  return bestScore;
}

string PositionInfo::fenGen() {
  string fen{""};
  char spaceBetween{'0'};
  // 遍历棋盘
  for (Position start{51}; start < 204; start += 16) {
    for (Position pos{start}; pos < start + 9; ++pos) {
      switch (this->mChessBoard[pos]) {
        // 空的
      case EMPTY:
        ++spaceBetween;
        break;

      default:
        if (spaceBetween != '0') {
          fen += spaceBetween;
          spaceBetween = '0';
        }
        fen += CHESS_FEN[this->mChessBoard[pos]];
      }
    }
    // 一行最后的空格
    if (spaceBetween != '0') {
      fen += spaceBetween;
      spaceBetween = '0';
    }
    fen += '/';
  }
  // 移除最后一个多加的/
  fen.erase(--end(fen));
  fen += COMPUTER_SIDE == RED ? " w" : " b";
  return fen;
}

tuple<QString, Step> PositionInfo::searchBook() {
  HttpRequest httpReq{"112.73.74.24", 80};
  string res =
      httpReq.HttpGet("/chessdb.php?action=queryall&board=" + fenGen());
  //未联网或获取失败
  if (res.empty()) {
    return {QString::fromLocal8Bit("象棋引擎"), {0, 0, 0, 0}};
  }
  //分割取走法
  vector<string> splitRes = httpReq.split(res, "\n");
  vector<string> parse = httpReq.split(*--splitRes.end(), ":");
  //云库无对应招法
  if (parse.at(0) != "move") {
    return {QString::fromLocal8Bit("象棋引擎"), {0, 0, 0, 0}};
  } else {
    //走棋
    uint16_t nowRow = 9 - (parse.at(1).at(1) - '0');
    uint16_t nowCol = parse.at(1).at(0) - 'a';
    uint16_t destRow = 9 - (parse.at(1).at(3) - '0');
    uint16_t destCol = parse.at(1).at(2) - 'a';
    return {QString::fromLocal8Bit("云库出步"),
            {nowRow, nowCol, destRow, destCol}};
  }
}

// 搜索的入口
Score searchMain() {
  // 重置信息
  resetCache(POSITION_INFO);
  // 迭代加深，重置深度
  CURRENT_DEPTH = 1;
  Score bestScore{0};
  // 多线程搜索，每个线程一个PositionInfo，只有置换表是共享的
  // 为什么是MAX_CONCURRENT - 1?
  // 因为本身开启其他线程去搜索的线程也要参与搜索，占用一个核心
  QVector<PositionInfo> threadPositionInfos(MAX_CONCURRENT - 1);
  // 初始化线程局面
  for (auto &threadPositionInfo : threadPositionInfos) {
    threadPositionInfo.setThreadPositionInfo(POSITION_INFO);
  }
  // 时间控制
  auto startTimeStamp = clock();
  forever {
    // 多线程搜索
    QVector<QFuture<void>> tasks;
    tasks.reserve(MAX_CONCURRENT - 1);
    for (auto &threadPositionInfo : threadPositionInfos) {
      tasks.emplaceBack(QtConcurrent::run(
          [&] { threadPositionInfo.searchRoot(CURRENT_DEPTH); }));
    }
    // 自己也搜索
    bestScore = POSITION_INFO.searchRoot(CURRENT_DEPTH);
    // 等待线程结束
    for (auto &task : tasks) {
      task.waitForFinished();
    }
    ++CURRENT_DEPTH;
    // 如果赢了或者输了就不用再往下搜索了
    if (bestScore < LOST_SCORE or bestScore > WIN_SCORE) {
      break;
    }
    // 超过时间就停止搜索
    if (clock() - startTimeStamp > SEARCH_TIME) {
      break;
    }
  }
  // 走棋
  POSITION_INFO.makeMove(POSITION_INFO.mBestMove, COMPUTER_SIDE);
  return bestScore;
}

// 用于初始化工作
inline void init() {
  // 初始化棋盘
  POSITION_INFO.resetBoard();
  // 初始化CHESS_FEN
  CHESS_FEN[ROOK] = 'R';
  CHESS_FEN[KNIGHT] = 'N';
  CHESS_FEN[CANNON] = 'C';
  CHESS_FEN[PAWN] = 'P';
  CHESS_FEN[BISHOP] = 'B';
  CHESS_FEN[ADVISOR] = 'A';
  CHESS_FEN[KING] = 'K';
  for (Chess chess{9}; chess < 16; ++chess) {
    CHESS_FEN[chess] = tolower(CHESS_FEN[chess - BLACK]);
  }
  // 电脑选边恢复
  COMPUTER_SIDE = BLACK;
  // 初始化Zobrist表
  // 梅森螺旋随机数引擎
  mt19937 eng(time(NULL));
  // 整型平均分布
  uniform_int_distribution<ZobristValue> uniDist;
  // 每一个棋子编号 1 - 7  9 - 15
  for (Chess chess{1}; chess < 16; ++chess) {
    if (chess == 8) {
      continue;
    }
    // 每一个位置 51 - 203
    for (Position pos{51}; pos < 204; ++pos) {
      CHESS_ZOBRIST[chess][pos] = uniDist(eng);
    }
  }
  // 初始化选边随机值
  SIDE_ZOBRIST = uniDist(eng);
}
