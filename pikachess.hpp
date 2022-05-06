#include "httprequest.h"
#include <QReadWriteLock>
#include <QString>
#include <QtConcurrent/QtConcurrent>
// ���������������������俩~
#include <bits/stdc++.h>
using namespace std;

/*
 * ������ȡһЩ������ı�����!
 * MetaType����
 * Ϊʲô��ʹ��stl�е������أ����߲����������г��ԣ�����������ķ��ڷ���!
 */
// ��Ϸѡ�����
using Side = uint8_t;

// �������
using Index = uint8_t;
using Count = uint8_t;

// �������
using Chess = uint8_t;
using ChessBoard = Chess[256];

// λ�á��߷����
using Position = uint8_t;
using Move = uint16_t;
using Delta = int8_t;
using SpanBoard = Delta[512];
using Step = std::tuple<Position, Position, Position, Position>;

// �������
// ÿһ������������߷��б�����128���������ܲ�������128���߷�
using MoveList = Move[128];
using Score = int16_t;
using Depth = int8_t;
using MoveFlag = bool;
using NullFlag = bool;
using Clock = clock_t;

// fen���
using FenMap = unordered_map<Chess, char>;
// ����״̬���Ľ׶�
using Phase = uint8_t;

// ��ʷ�����
// ��ʷ�����
using HistoryScore = uint64_t;
// ��ʷ��һ���߷������ֵΪ: (203 << 8) | 202 = 52170
using History = HistoryScore[52171];

// ɱ���߷������
using Killer = Move[128][2];

// Zobrist���
// ���̵�zobristֵ����λ��������λУ��
using ZobristValue = uint64_t;
// ������ÿһ��λ�õ�zobristֵ
using Zobrist = ZobristValue[16][256];

// ��ʷ�߷����
// �ظ������־
using RepeatFlag = uint8_t;
// ÿһ���Ľṹ
struct MoveItem {
  // �߸��߷�ǰ��Ӧ��Zobrist�����ڼ���ظ�
  ZobristValue mZobrist;
  // �߷�
  Move mMove;
  // ���汻�Ե����ӣ�Ҳ���Ա�־��һ���ǲ��ǳ����߷�����Ϊ�յ�ֵΪ0
  Chess mVictim;
  // �Ƿ��ǽ�����
  bool mCheck;
};
// ��ʷ�߷���
using HistoryMove = MoveItem[256];

// �û������
using HashMask = uint64_t;
using HashFlag = uint8_t;
// �û����С
constexpr size_t HASH_SIZE = 1 << 20;
// ȡ�û�����ʱ������
HashMask HASH_MASK{HASH_SIZE - 1};
// �����û������flag
// ALPHA�ڵ���û�����
constexpr HashFlag HASH_ALPHA{0};
// BETA�ڵ���û�����
constexpr HashFlag HASH_BETA{1};
// PV�ڵ���û�����
constexpr HashFlag HASH_PV{2};
struct HashItem {
  // �߸��߷�ǰ��Ӧ��Zobrist������У��
  ZobristValue mZobrist;
  // �߷�
  Move mMove;
  // ���߷���Ӧ�ķ���
  Score mScore;
  // ��¼����ʱ���������
  Depth mDepth;
  // ���߷���Ӧ�����ͣ�ALPHA��PV��BETA��
  HashFlag mFlag;
};
// �û���
using HashTable = HashItem[HASH_SIZE];

// ����λ����Ϣ�����ڶ��߳�����
// ����move�ĳ�ʼֵ
Move INVALID_MOVE{0};
// ��������λ����Ϣ
struct PositionInfo {
  // ���̾�����Ϣ
  ChessBoard mChessBoard{};
  // ���ھ�����ڵ�����
  Depth mDistance{0};
  // ��ʷ�����ڼ���
  History mHistory{};
  // ɱ�ֱ���¼beta�ضϵ��߷�
  Killer mKiller{};
  // ��ʷ�߷���Ŀǰ���
  size_t mHistorySize{1};
  // ��ʷ�߷���
  HistoryMove mHistoryMove{};
  // �������ڵ�Zobristֵ
  ZobristValue mZobrist{0};
  // ��������߷�
  Move mBestMove{INVALID_MOVE};
  // �췽�ķ���
  Score mRedScore{888};
  // �ڷ��ķ���
  Score mBlackScore{888};
  // ���캯��
  PositionInfo();
  // ��ʼ������
  void resetBoard();
  // ���߳�ʱʹ�ã����������PositionInfo������������
  void setThreadPositionInfo(const PositionInfo &positionInfo);
  // �жϸ������Ǻ췽���ӻ��Ǻڷ�����, ���ܴ���գ��������췽��������
  inline Side getChessSide(const Position pos);
  // ��λ���Ƿ��ǿյ�
  inline bool isEmpty(const Position pos);
  // ��λ���Ƿ��ǶԷ�������
  inline bool isOppChess(const Position pos, const Side side);
  // ��λ���Ƿ�Ϊ�ջ�Ϊ�Է�������
  inline bool isEmptyOrOppChess(const Position pos, const Side side);
  // ��ȡ�����ϵĽ�
  Position getKING(const Side side);
  // �жϸ�λ��Ϊ�񱻽���,sideΪ���巽
  bool isChecked(const Side side);
  // �ж�һ���߷��Ƿ��ǺϷ����߷�
  bool isLegalMove(const Move move, const Side side);
  // ����ظ��߷�
  inline RepeatFlag getRepeatFlag();
  // ���غ���ķ�ֵ
  inline Score getDrawScore();
  // ���ظ����״̬������ȡ����
  inline Score scoreRepeatFlag(const RepeatFlag repeatFlag);
  // ����췽����ĳ��λ�ú�ķ���
  inline void calcRedMove(const Move move);
  // ����ڷ�����ĳ��λ�ú�ķ���
  inline void calcBlackMove(const Move move);
  // ����췽��ԭ��ĳһ�����ķ���
  inline void calcRedUnMove(const Move move, const Chess victim);
  // ����ڷ���ԭ��ĳһ�����ķ���
  inline void calcBlackUnMove(const Move move, const Chess victim);
  // ������һ���ķ�����Zobrist
  inline void calcMove(const Move move, const Side side);
  // ���㳷����һ���ķ�����Zobrist
  inline void calcUnMove(const Move move, const Chess victim, const Side side);
  // ���ֺ���
  inline Score evaluate(const Side side);
  // �����û���
  Score probeHash(Score alpha, Score beta, Depth depth, Move &hashMove);
  // ���浽�û���
  void recordHash(HashFlag hashFlag, Score score, Depth depth, Move move);
  // ����������ʷ��ɱ�ֱ�
  inline void setBestMove(const Move move, const Depth depth);
  // �߷�����
  Count moveGen(MoveList &moves, const Side side,
                const MoveFlag capture = false);
  // ��������
  inline void unMakeMove(const Move move, const Side side);
  // ����,�����Ƿ��߳ɹ�
  inline bool makeMove(const Move move, const Side side);
  // ��ǰ�Ƿ�����߿ղ��������ڲоֽ׶�
  inline bool isNullOk(const Side side);
  // ��һ���ղ�
  inline void makeNullMove();
  // ������һ���ղ�
  inline void unMakeNullMove();
  // ��̬����
  Score searchQuiescence(Score alpha, const Score beta, const Side side);
  // ��ȫ��������
  Score searchFull(Score alpha, const Score beta, const Depth depth,
                   const Side side, const NullFlag nullOk = true);
  // ���ڵ������
  Score searchRoot(const Depth depth);
  // fen����
  string fenGen();
  // �����ƿ�
  tuple<QString, Step> searchBook();
};

// ������������״̬���Ľ׶�
constexpr Phase PHASE_HASH{0};
constexpr Phase PHASE_KILLER1{1};
constexpr Phase PHASE_KILLER2{2};
constexpr Phase PHASE_GEN_MOVE{3};
constexpr Phase PHASE_ALL_MOVE{4};
// ����������״̬��
struct SearchMachine {
  // λ����Ϣ
  PositionInfo &mPositionInfo;
  // �����ڵڼ����׶Σ���ʼֵΪ��ϣ���߷���
  Phase mNowPhase{PHASE_HASH};
  // ѡ�߱�־
  Side mSide;
  // �������ڱ����ڼ����߷�
  Index mNowIndex{0};
  // �ܹ��ж������߷�
  Count mTotalMoves{0};
  // �����߷����б�
  MoveList mMoves{};
  // �û����߷���ɱ���߷�1��2
  Move mHash, mKiller1, mKiller2;
  // ���캯��
  SearchMachine(PositionInfo &positionInfo, const Move hashMove,
                const Side side);
  // �����߷��ĺ���
  Move nextMove();
};

/* ���������ݶ��� */

// �����������ͺͱ߱�ʾ�������ڱ���ʿ��
constexpr Side RED{0}, BLACK{8};
constexpr Chess EMPTY{0};
constexpr Chess ROOK{1}, KNIGHT{2}, CANNON{3};
constexpr Chess PAWN{4}, BISHOP{5}, ADVISOR{6};
constexpr Chess KING{7};
constexpr Chess RED_KING{7};
constexpr Chess BLACK_KING{15};

// �������̣������й�����ĳ�ʼ�ڷ���~
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

//������λ�÷���
constexpr Score RED_VALUE[8][256]{
    {// ��
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
    {// ��
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
    {// ��
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
    {// ��
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
    {// ��
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
    {// ��
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
    {// ʿ
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
    {// ��
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

// �Ƿ���������
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

// �Ƿ��ھŹ���
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

// �Ƿ��ں��Ӱ��
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

// �Ƿ��ں��Ӱ��
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

// �жϲ����Ƿ�����ض��߷������飬1=����2=ʿ��3=��
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

// ���ݲ����ж����Ƿ����ȵ�����
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
// ÿ�������ļ�ֵ�������ڱ���ʿ��
constexpr Score MVVLVA[16] = {0, 4, 3, 3, 2, 1, 1, 5, 0, 4, 3, 3, 2, 1, 1, 5};

// ������������fen��map
FenMap CHESS_FEN;

// �����߷���־
constexpr MoveFlag CAPTURE{true};

// ���Ųü���־
constexpr NullFlag NO_NULL{false};

// ���Ĳ���, ��������
constexpr Delta KING_DELTA[4]{-16, 16, -1, 1};

// ʿ�Ĳ�����б��
constexpr Delta ADVISOR_DELTA[4]{-17, -15, 15, 17};

// ��Ĳ���������б��
constexpr Delta BISHOP_DELTA[4]{-34, -30, 30, 34};

// ���۵�λ�ã���ʿ�Ĳ�����ͬ
constexpr const Delta (&BISHOP_PIN)[4]{ADVISOR_DELTA};

// ��(��),�Լ��ɽ��Ĳ���
constexpr const Delta (&LINE_CHESS_DELTA)[4]{KING_DELTA};

// ��Ĳ����������߷�
constexpr Delta KNIGHT_DELTA[4][2]{
    {-31, -33}, // ������ -16 ��Ӧ
    {31, 33},   // ������ 16 ��Ӧ
    {14, -18},  // ������ -1 ��Ӧ
    {-14, 18},  // ������ 1 ��Ӧ
};

// ���ȵ�λ�ã��뽫�Ĳ�����ͬ
constexpr const Delta (&KNIGHT_PIN)[4]{KING_DELTA};

// ���Ӻ���Ĳ���
constexpr Delta PROMOTED_RED_PAWN_DELTA[3]{-16, -1, 1};

// ���Ӻڱ��Ĳ���
constexpr Delta PROMOTED_BLACK_PAWN_DELTA[3]{16, -1, 1};

// û���ӵĺ��
constexpr Delta RED_PAWN_DELTA{-16};

// û���ӵĺڱ�
constexpr Delta BLACK_PAWN_DELTA{16};

// ������ʱ��Ĳ����������߷�
constexpr Delta CHECK_KNIGHT_DELTA[4][2]{
    {-18, -33}, // �ͱ�����ʱ���� -17 ��Ӧ
    {-31, -14}, // �ͱ�����ʱ���� -15 ��Ӧ
    {14, 31},   // �ͱ�����ʱ���� 15 ��Ӧ
    {18, 33},   // �ͱ�����ʱ���� 17 ��Ӧ
};

// ������ʱ���ȵ�λ��,��ʿ�Ĳ�����ͬ
const Delta (&CHECK_KNIGHT_PIN)[4]{ADVISOR_DELTA};

// ���Ժ콫���ܳ��ֵ�λ��
constexpr Position RED_KING_POSITION[9]{166, 167, 168, 182, 183,
                                        184, 198, 199, 200};

// ���Ժڽ����ܳ��ֵ�λ��
constexpr Position BLACK_KING_POSITION[9]{54, 55, 56, 70, 71, 72, 86, 87, 88};

// ��ָʾ����ָʾ�Ǻ췽���Ǻڷ�
Side COMPUTER_SIDE{BLACK};

// ������ÿһ��λ�õ�Zobristֵ��
Zobrist CHESS_ZOBRIST;

// ѡ�ߵ�Zobristֵ����������ʱ��Ҫ������ֵ
ZobristValue SIDE_ZOBRIST;

// �û���
HashTable HASH_TABLE;

// �û�����
QReadWriteLock HASH_TABLE_LOCK;

// ��������ʱ��
Clock SEARCH_TIME{1000};

// ����������ķ���
constexpr Score ADVANCED_SCORE{3};

// ��������ķ���
constexpr Score LOSS_SCORE{-1000};

// �������ķ���
constexpr Score DRAW_SCORE{-20};

// ����Ӯ��ķ���
constexpr Score MATE_SCORE{1000};

// �����и��ķ�ֵ�����ڸ�ֵ����д���û���
constexpr Score BAN_SCORE_MATE{950};

// �����и��ķ�ֵ�����ڸ�ֵ����д���û���
constexpr Score BAN_SCORE_LOSS{-950};

// ������Ӯ��ķ�ֵ���ޣ�������ֵ��˵���Ѿ�������ɱ����
constexpr Score WIN_SCORE{900};

// ����������ķ�ֵ���ޣ�������ֵ��˵���Ѿ�������ɱ����
constexpr Score LOST_SCORE{-900};

// ��󲢷������ִ�cpuһ��Ϊ���߳�cpu������Ҫ����2������������
Count MAX_CONCURRENT{static_cast<Count>(thread::hardware_concurrency() / 2)};

// �������������
Depth CURRENT_DEPTH{0};

// ����ȫ���߳̾���
PositionInfo POSITION_INFO{};

// ���ڲ���߷���ȡ��8λ
inline Position getSrc(const Move move) { return move >> 8; }

// ���ڲ���߷���ȡ��8λ
inline Position getDest(const Move move) { return move & 0xFF; }

// ���ںϲ��߷�
inline Move toMove(const Position from, const Position to) {
  return from << 8 | to;
}

// ���ڻ�ȡһ���߷���MVVLVAֵ����ֵԽ��˵��Խֵ
inline Score getMVVLVA(const Move move) {
  return (MVVLVA[getDest(move)] << 3) - MVVLVA[getSrc(move)];
}

// �߷��Ƿ����˧(��)�Ĳ���
inline bool isKingSpan(const Move move) {
  return LEGAL_SPAN[256 + getDest(move) - getSrc(move)] == 1;
}

// �߷��Ƿ������(ʿ)�Ĳ���
inline bool isAdvisorSpan(const Move move) {
  return LEGAL_SPAN[256 + getDest(move) - getSrc(move)] == 2;
}

// �߷��Ƿ������(��)�Ĳ���
inline bool isBishopSpan(const Move move) {
  return LEGAL_SPAN[256 + getDest(move) - getSrc(move)] == 3;
}

// ���ر���ǰ��һ����λ��
inline Position getPawnForwardPos(const Position pos, const Side side) {
  if (side == RED) {
    // ����Ǻ췽������ǰ��
    return pos + RED_PAWN_DELTA;
  } else {
    // ����Ǻڷ�������ǰ��
    return pos + BLACK_PAWN_DELTA;
  }
}

// ���۵�λ��
inline Position getBishopPinPos(const Move move) {
  return (getSrc(move) + getDest(move)) >> 1;
}

// ���ȵ�λ��
inline Position getKnightPinPos(const Move move) {
  return getSrc(move) +
         LEGAL_KNIGHT_PIN_SPAN[256 + getDest(move) - getSrc(move)];
}

// �Ƿ���ͬһ��
inline bool isSameHalf(const Move move) {
  return ((getSrc(move) ^ getDest(move)) & 0x80) == 0;
}

// �Ƿ���ͬһ��
inline bool isSameRow(const Move move) {
  return ((getSrc(move) ^ getDest(move)) & 0xF0) == 0;
}

// �Ƿ���ͬһ��
inline bool isSameColumn(const Move move) {
  return ((getSrc(move) ^ getDest(move)) & 0x0F) == 0;
}

// ���ڼƷֵķ�ת
inline Position flipPosition(const Position pos) { return 254 - pos; }

// �����û����������Ϣ
inline void resetCache(PositionInfo &positionInfo) {
  // ���������Ϣ
  positionInfo.mDistance = 0;
  // �����û���
  memset(HASH_TABLE, 0, sizeof(HASH_TABLE));
}

// ������Ϣ�Ĺ��캯��
PositionInfo::PositionInfo() {
  // ��ʼ������
  memcpy(this->mChessBoard, INIT_BOARD, 256);
  //��ձ�
  memset(this->mHistory, 0, sizeof(this->mHistory));
  memset(this->mKiller, 0, sizeof(this->mKiller));
  // ��ʼ�����̵�Zobrist��ÿһ��λ�� 51 - 203
  for (Position pos{51}; pos < 204; ++pos) {
    // ��������Ӿ���������ӵ�Zobristֵ
    if (this->mChessBoard[pos]) {
      this->mZobrist ^= CHESS_ZOBRIST[this->mChessBoard[pos]][pos];
    }
  }
}

// ��������
void PositionInfo::resetBoard() {
  // ��ʼ������
  memcpy(this->mChessBoard, INIT_BOARD, 256);
  // ��ʼ��������ڵ�������Ϣ
  this->mDistance = 0;
  // ��ձ�
  memset(this->mHistory, 0, sizeof(this->mHistory));
  memset(this->mKiller, 0, sizeof(this->mKiller));
  // ��ʷ���С��0
  this->mHistorySize = 0;
  // �����ʷ�߷���
  memset(this->mHistoryMove, 0, sizeof(this->mHistoryMove));
  // ���Zobrist
  this->mZobrist = 0;
  // ��ʼ�����̵�Zobrist��ÿһ��λ�� 51 - 203
  for (Position pos{51}; pos < 204; ++pos) {
    // ��������Ӿ���������ӵ�Zobristֵ
    if (this->mChessBoard[pos]) {
      this->mZobrist ^= CHESS_ZOBRIST[this->mChessBoard[pos]][pos];
    }
  }
  // ���BestMove
  this->mBestMove = INVALID_MOVE;
  // ��ʼ������
  this->mRedScore = this->mBlackScore = 888;
}

// ������Ϣ���ƣ����̹߳ؼ�
void PositionInfo::setThreadPositionInfo(const PositionInfo &positionInfo) {
  // ��������
  memcpy(this->mChessBoard, positionInfo.mChessBoard,
         sizeof(this->mChessBoard));
  // ������ʷ�߷�
  memcpy(this->mHistoryMove, positionInfo.mHistoryMove,
         sizeof(this->mHistoryMove));
  // ��������
  this->mRedScore = positionInfo.mRedScore;
  this->mBlackScore = positionInfo.mBlackScore;
  // ����Zobrist
  this->mZobrist = positionInfo.mZobrist;
  // ������С
  this->mHistorySize = positionInfo.mHistorySize;
}

// �жϸ������Ǻ췽���ӻ��Ǻڷ�����, ���ܴ���գ��������췽��������
inline Side PositionInfo::getChessSide(const Position pos) {
  return this->mChessBoard[pos] & 0b1000;
};

// ��λ���Ƿ��ǿյ�
inline bool PositionInfo::isEmpty(const Position pos) {
  return not this->mChessBoard[pos];
}

// ��λ���Ƿ��ǶԷ�������
inline bool PositionInfo::isOppChess(const Position pos, const Side side) {
  return this->mChessBoard[pos] and getChessSide(pos) not_eq side;
}

// ��λ���Ƿ�Ϊ�ջ�Ϊ�Է�������
inline bool PositionInfo::isEmptyOrOppChess(const Position pos,
                                            const Side side) {
  return isEmpty(pos) or getChessSide(pos) not_eq side;
}

// ��ö��ֵ�ѡ��
inline Side getOppSide(const Side side) { return side ^ 0b1000; }

// ��һ������ת��Ϊ��Ӧ�ߵ�����
inline Chess toSideChess(const Chess chess, const Side side) {
  return chess + side;
}

// �Ƿ����Լ��İ�ߣ��ṩԽ����
inline bool isMyHalf(const Position pos, const Side side) {
  if (side == RED) {
    return IN_RED_HALF[pos];
  } else {
    return IN_BLACK_HALF[pos];
  }
}

// �Ƿ��ڶ��ֵİ�ߣ��ṩԽ����
inline bool isOppHalf(const Position pos, const Side side) {
  if (side == RED) {
    return IN_BLACK_HALF[pos];
  } else {
    return IN_RED_HALF[pos];
  }
}

// ��ȡ�����ϵĽ�
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

// �жϸ�λ��Ϊ�񱻽���,sideΪ���巽
bool PositionInfo::isChecked(const Side side) {
  // �����ҵ������ϵĽ�
  Position pos{getKING(side)};
  // ��öԷ���sideTag
  Side oppSideTag{getOppSide(side)};

  // �ж��Ƿ񱻱�����
  for (const auto &delta :
       side == RED ? PROMOTED_RED_PAWN_DELTA : PROMOTED_BLACK_PAWN_DELTA) {
    // ����Ӧλ���Ƿ���������,��Ӧ��λ�����Ƿ�������,��һ����������ǲ��ǶԷ��ı�
    if (this->mChessBoard[pos + delta] and
        getChessSide(pos + delta) not_eq side and
        this->mChessBoard[pos + delta] == toSideChess(PAWN, oppSideTag)) {
      return true;
    }
  }

  // �ж��Ƿ�����
  for (Index index{0}; index < 4; ++index) {
    // �ȿ�������û���������������û�б�����
    // ���ȳ�����Χ�����������˾�ֱ����һ�֣���������һ��forѭ�������ж�����
    if (IN_BOARD[pos + CHECK_KNIGHT_PIN[index]] and
        isEmpty(pos + CHECK_KNIGHT_PIN[index])) {
      // ����ÿһ����
      for (const auto &delta : CHECK_KNIGHT_DELTA[index]) {
        // ����Ӧλ���Ƿ���������,��Ӧ��λ�����Ƿ�������,�Ƿ�������,��һ����������ǲ��ǶԷ�����
        if (this->mChessBoard[pos + delta] and
            getChessSide(pos + delta) not_eq side and
            this->mChessBoard[pos + delta] == toSideChess(KNIGHT, oppSideTag)) {
          return true;
        }
      }
    }
  }

  // �ж��Ƿ񱻳�(��)������������˧����
  for (const auto &delta : LINE_CHESS_DELTA) {
    Position cur{static_cast<Position>(pos + delta)};
    // ��鳵(��)
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
    // �����
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

// �ж�һ���߷��Ƿ��ǺϷ����߷�
bool PositionInfo::isLegalMove(const Move move, const Side side) {
  // ����Ҫ�õ��ı���
  Position src{getSrc(move)}, dest{getDest(move)};

  // �����ʼ��λ�ò����Լ������ӻ����յ��λ�����Լ������Ӿͷ��ؼ�
  if (isEmpty(src) or getChessSide(src) not_eq side or
      (this->mChessBoard[dest] and getChessSide(dest) == side)) {
    return false;
  }

  // ����Ҫ�õ��ı���
  Delta delta;
  Chess chess{static_cast<Chess>(this->mChessBoard[src] - side)};
  // ��������ж��߷��Ƿ�Ϸ�
  switch (chess) {
    // �Ƿ��ھŹ��ڣ��Ƿ���Ͻ��Ĳ���
  case KING:
    return IN_SQRT[dest] and isKingSpan(move);
    // �Ƿ��ھŹ��ڣ��Ƿ����ʿ�Ĳ���
  case ADVISOR:
    return IN_SQRT[dest] and isAdvisorSpan(move);
    // �Ƿ���ͬһ�ߣ��Ƿ������Ĳ������Ƿ�������
  case BISHOP:
    return isSameHalf(move) and isBishopSpan(move) and
           isEmpty(getBishopPinPos(move));
    // �Ƿ������Ĳ������Ƿ������
  case KNIGHT:
    return getKnightPinPos(move) not_eq src and isEmpty(getKnightPinPos(move));
  case ROOK:
  case CANNON:
    // �ж��Ƿ���ͬһ��
    if (isSameRow(move)) {
      if (src > dest) {
        delta = -1;
      } else {
        delta = 1;
      }
      // �Ƿ���ͬһ��
    } else if (isSameColumn(move)) {
      if (src > dest) {
        delta = -16;
      } else {
        delta = 16;
      }
    } else {
      return false;
    }
    // ��ǰ������һ������
    src += delta;
    while (src not_eq dest and isEmpty(src)) {
      src += delta;
    }
    if (src == dest) {
      // _:�� X:Ŀ������ O:�м�����
      // ��ͷ�ˣ���������λ���Ƿ�Ϊ�գ�Ҳ������Щ���: ��------->_ | ��------->_
      // �����Ϊ�գ������ߵ������Ƿ�Ϊ���������ӣ���Ҳ�����������: ��------->X
      return isEmpty(dest) or chess == ROOK;
    } else if (this->mChessBoard[dest] and chess == CANNON) {
      // û�е�ͷ������Ŀ��λ���жԷ������ӣ����������ߵ��������ھͼ����ж�
      src += delta;
      // ��ǰ�ҵڶ�������
      while (src not_eq dest and isEmpty(src)) {
        src += delta;
      }
      // �����ͷ�ˣ�������ͷ�ǲ������ӣ�Ҳ�����������: ��---O--->X
      return src == dest;
    }
    // ���������в����ϵ������������:
    // ��---O--->_ | ��---O--->X
    // ��---O--->_ | ��--O-O-->X
    return false;
  case PAWN:
    // ��������ˣ���������������
    if (isOppHalf(src, side) and (dest - src == 1 or dest - src == -1)) {
      return true;
    }
    // ���û����ֻ����ǰ�ߣ���������˾Ͳ�����ǰ�ߵ�һ��
    return dest == getPawnForwardPos(src, side);
  default:
    return false;
  }
}

// ����ظ��߷�
inline RepeatFlag PositionInfo::getRepeatFlag() {
  // mySide��������Ƿ��ǵ��ñ���������һ��(�³�"�ҷ�")
  // ��Ϊһ�������������ϵ����˱��������ҷ�û������
  // �����ڼ���ظ��߷�ʱ����ʷ�߷��������һ�����ǶԷ������һ��
  // ������������ĳ�ʼֵΪ�٣�������һ�������ҷ�����Ϊ�߷��Ӻ���ǰ����
  bool mySide{false};
  // �ҷ��Ƿ񽫾����Է��Ƿ񽫾�
  bool myCheck{true}, oppCheck{true};
  // ָ����ʷ�߷�������һ���ǰ����
  MoveItem *move = this->mHistoryMove + this->mHistorySize - 1;
  // ���뱣֤������Ч��Ҳ����û�е�ͷ���ڱ����߿ղ��ü���
  // ��������ղ��ü��Ͳ����¼���ˣ���Ϊ�ղ��޷�������Ч��
  // ����Ҫ���ǳ��Ӳ�����Ϊ���Ӿʹ��Ƴ�����
  while (move->mMove not_eq INVALID_MOVE and not move->mVictim) {
    if (mySide) {
      // ������ҷ��������ҷ�������Ϣ
      myCheck &= move->mCheck;
      // �����⵽�����뵱ǰ�����ظ��ͷ���״̬��
      if (move->mZobrist == this->mZobrist) {
        return 1 + (myCheck ? 2 : 0) + (oppCheck ? 4 : 0);
      }
    } else {
      // ����ǶԷ������¶Է��Ľ�����Ϣ
      oppCheck &= move->mCheck;
    }
    // ����ѡ����Ϣ
    mySide = not mySide;
    // moveָ��ǰһ���߷�
    --move;
  }
  // û���ظ�����
  return false;
}

// ���غ���ķ�ֵ
inline Score PositionInfo::getDrawScore() {
  // ������ζ�Ҫʹ�ú�����ڵ�һ�����һ����˵�ǲ����ģ��Ǹ���
  // DISTANCE & 1 ��������ȷ����������һ��
  // ˵��evaluate����һ��͵�һ����ͬһ��
  // ͬһ�����ظ�ֵ����ͬ��������ֵ��������ֵ�ϵ���һ��ͻ��ɸ�ֵ
  return this->mDistance & 1 ? -DRAW_SCORE : DRAW_SCORE;
}

// ���ظ����״̬������ȡ����
inline Score PositionInfo::scoreRepeatFlag(const RepeatFlag repeatFlag) {
  // �ҷ��������ظ��֣��Է�������������
  Score score = (repeatFlag & 2 ? BAN_SCORE_LOSS + this->mDistance : 0) +
                (repeatFlag & 4 ? BAN_SCORE_MATE - this->mDistance : 0);
  if (score == 0) {
    // ���˫������������˫����û�г����������ظ�����
    return getDrawScore();
  } else {
    // ��һ������
    return score;
  }
}

// ����췽����ĳ��λ�ú�ķ���
inline void PositionInfo::calcRedMove(const Move move) {
  Position src{getSrc(move)}, dest{getDest(move)};
  Chess srcChess{this->mChessBoard[src]}, destChess{this->mChessBoard[dest]};
  // λ�÷ּ���
  this->mRedScore += RED_VALUE[srcChess][dest] - RED_VALUE[srcChess][src];
  // Zobrist����
  this->mZobrist ^= CHESS_ZOBRIST[srcChess][src];
  this->mZobrist ^= CHESS_ZOBRIST[srcChess][dest];
  if (this->mChessBoard[dest]) {
    this->mBlackScore -= RED_VALUE[destChess - BLACK][flipPosition(dest)];
    this->mZobrist ^= CHESS_ZOBRIST[destChess][dest];
  }
}

// ����ڷ�����ĳ��λ�ú�ķ���
inline void PositionInfo::calcBlackMove(const Move move) {
  Position src{getSrc(move)}, dest{getDest(move)};
  Chess srcChess{this->mChessBoard[src]}, destChess{this->mChessBoard[dest]};
  // λ�÷ּ���
  this->mBlackScore += RED_VALUE[srcChess - BLACK][flipPosition(dest)] -
                       RED_VALUE[srcChess - BLACK][flipPosition(src)];
  // Zobrist����
  this->mZobrist ^= CHESS_ZOBRIST[srcChess][src];
  this->mZobrist ^= CHESS_ZOBRIST[srcChess][dest];
  // ����Ҫ������Ӧ�ķ���
  if (this->mChessBoard[dest]) {
    this->mRedScore -= RED_VALUE[destChess][dest];
    this->mZobrist ^= CHESS_ZOBRIST[destChess][dest];
  }
}

// ����췽��ԭ��ĳһ�����ķ���
inline void PositionInfo::calcRedUnMove(const Move move, const Chess victim) {
  Position src{getSrc(move)}, dest{getDest(move)};
  Chess destChess{this->mChessBoard[dest]};
  // λ�÷ּ���
  this->mRedScore += RED_VALUE[destChess][src] - RED_VALUE[destChess][dest];
  // Zobrist����
  this->mZobrist ^= CHESS_ZOBRIST[destChess][dest];
  this->mZobrist ^= CHESS_ZOBRIST[destChess][src];
  // �ѳԵ����ӵķ����ӻ�ȥ
  if (victim not_eq EMPTY) {
    this->mBlackScore += RED_VALUE[victim - BLACK][flipPosition(dest)];
    this->mZobrist ^= CHESS_ZOBRIST[victim][dest];
  }
}

// ����ڷ���ԭ��ĳһ�����ķ���
inline void PositionInfo::calcBlackUnMove(const Move move, const Chess victim) {
  Position src{getSrc(move)}, dest{getDest(move)};
  Chess destChess{this->mChessBoard[dest]};
  // λ�÷ּ���
  this->mBlackScore += RED_VALUE[destChess - BLACK][flipPosition(src)] -
                       RED_VALUE[destChess - BLACK][flipPosition(dest)];
  // Zobrist����
  this->mZobrist ^= CHESS_ZOBRIST[destChess][dest];
  this->mZobrist ^= CHESS_ZOBRIST[destChess][src];
  // �ѳԵ����ӵķ����ӻ�ȥ
  if (victim not_eq EMPTY) {
    this->mRedScore += RED_VALUE[victim][dest];
    this->mZobrist ^= CHESS_ZOBRIST[victim][dest];
  }
}

// ������һ���ķ�����Zobrist
inline void PositionInfo::calcMove(const Move move, const Side side) {
  // �϶�������
  this->mZobrist ^= SIDE_ZOBRIST;
  if (side == RED) {
    // ����Ǻ췽����
    calcRedMove(move);
  } else {
    // ����Ǻڷ�����
    calcBlackMove(move);
  }
}

// ���㳷����һ���ķ�����Zobrist
inline void PositionInfo::calcUnMove(const Move move, const Chess victim,
                                     const Side side) {
  // �϶�������
  this->mZobrist ^= SIDE_ZOBRIST;
  if (side == RED) {
    // ����Ǻ췽����
    calcRedUnMove(move, victim);
  } else {
    // ����Ǻڷ�����
    calcBlackUnMove(move, victim);
  }
}

inline Score PositionInfo::evaluate(const Side side) {
  /*
    Ϊ�δ˴�Ҫ�������һ��ADVANCED_SCORE������֣���Ϊִ�иú���ʱ�������ֵ��ò�������壬
    ������Ϊ����ԭ��ֻ�������������ˣ��ò����û�������ֱ�ӷ���������������Լ�������!
    ʵ�������������������ǲ�����ȷ�ģ����Լ���һ������������������һ���Ǹ�������У�ʹ�����۸�����һЩ!
  */
  if (side == RED) {
    return this->mRedScore - this->mBlackScore + ADVANCED_SCORE;
  } else {
    return this->mBlackScore - this->mRedScore + ADVANCED_SCORE;
  }
}

// �����û���
Score PositionInfo::probeHash(Score alpha, Score beta, Depth depth,
                              Move &hashMove) {
  // ɱ��ı�־�����ɱ���˾Ͳ��������������
  bool isMate{false};
  // ��ȡ�û����ע�⵽���ﲻȡ���ã���Ϊ����Ҫ�Է��������޸�
  HASH_TABLE_LOCK.lockForRead();
  HashItem hashItem = HASH_TABLE[this->mZobrist & HASH_MASK];
  HASH_TABLE_LOCK.unlock();
  // У���λzobrist�Ƿ��Ӧ����
  if (this->mZobrist not_eq hashItem.mZobrist) {
    hashMove = INVALID_MOVE;
    return LOSS_SCORE;
  }
  // ���߷���������
  hashMove = hashItem.mMove;
  if (hashItem.mScore > WIN_SCORE) {
    if (hashItem.mScore < BAN_SCORE_MATE) {
      // �п��ܻᵼ�������Ĳ��ȶ��ԣ���Ϊ��������ķ�����������������һ����ͬ��������֧
      return LOSS_SCORE;
    }
    // ���Ӯ��
    isMate = true;
    // �����������̲�����Ϣ
    hashItem.mScore -= this->mDistance;
  } else if (hashItem.mScore < LOST_SCORE) {
    if (hashItem.mScore > BAN_SCORE_LOSS) {
      // ����ͬ��
      return LOSS_SCORE;
    }
    // �������
    isMate = true;
    // �����������̲�����Ϣ
    hashItem.mScore += this->mDistance;
  }
  // �жϸ��߷��Ƿ��������������������ȵ�ǰ����������ͬ����߸���
  // ���Ӯ�˾Ͳ��������������
  if (hashItem.mDepth >= depth || isMate) {
    if (hashItem.mFlag == HASH_BETA) {
      /*
       * �����beta�߷���˵����ͬ���߲��������þ����߸��߷�ʱ������beta�ض�
       * ��ô�鿴һ���ڵ�ǰ��beta���߸��߷��Ƿ�Ҳ���Է����ضϣ���score >= beta
       * ���Ƿ񳬳�alpha-beta�ı߽�(alpha, beta)(here)
       * ��������򳬹���ǰbeta�߽缴�ɽض�
       * ���û�г���beta����ô����ֱ�ӷ���score
       * ��Ϊ��Ȼͬ���߲㷢����beta�ضϣ���ô���п���û��������þ����µ������߷���������һ���߱��ο���
       * ��������߷���ֱ�ӷ��أ���Ϊ�����ڸ����ٵ���С��ǰ��alpha-beta��Χ
       */
      if (hashItem.mScore >= beta) {
        return hashItem.mScore;
      } else {
        return LOSS_SCORE;
      }
    } else if (hashItem.mFlag == HASH_ALPHA) {
      /*
       * �����alpha�߷���˵����ͬ������ϲ�������б����˾����е������߷�
       * ���ҵõ�����õ��߷���alpha�߷��������޷������ǲ��alpha
       * ��ô�鿴һ���ڵ�ǰ��alpha���Ƿ�Ҳ�޷�������ǰ��alpha����score <= alpha
       * ���Ƿ񳬳�alpha-beta�ı߽�(here)(alpha, beta)
       * ���������С�ڵ�ǰalpha���ɽض�
       * �������alpha����ô����ֱ�ӷ���score
       * ��Ϊ�������alpha-beta��Χ�ڣ���ô���Ͳ���alpha�߷���ֵ������һ��
       * ���ظ��߷������ڸ����ٵ���С��ǰ��alpha-beta��Χ
       * ���������������beta����ô����߷��Ϳ��Է���beta�ض�
       */
      if (hashItem.mScore <= alpha) {
        return hashItem.mScore;
      } else {
        return LOSS_SCORE;
      }
    }
    /*
     * ��ͬ������ϲ�������б����˾����е������߷����ҵ���pv�߷������Ծ�����������ˣ�ֱ�ӷ��ؼ��ɡ�
     */
    return hashItem.mScore;
  }
  // ����������������Ҳ���ɱ��
  return LOSS_SCORE;
}

// ���浽�û���
void PositionInfo::recordHash(HashFlag hashFlag, Score score, Depth depth,
                              Move move) {
  // ��ȡ�û�����
  HashItem &hashItem = HASH_TABLE[this->mZobrist & HASH_MASK];
  // �鿴�û����е����Ƿ�ȵ�ǰ�����׼ȷ
  if (hashItem.mDepth > depth) {
    return;
  }
  // ��Ȼ�ͱ����û����־�����
  hashItem.mFlag = hashFlag;
  hashItem.mDepth = depth;
  if (score > WIN_SCORE) {
    if (move == INVALID_MOVE && score <= BAN_SCORE_MATE) {
      // ���ܵ��������Ĳ��ȶ��ԣ�����û������ŷ��������˳�
      return;
    }
    // ����ͼ�¼������������������̲�����Ϣ
    hashItem.mScore = score + this->mDistance;
  } else if (score < LOST_SCORE) {
    if (move == INVALID_MOVE && score >= BAN_SCORE_LOSS) {
      // ͬ��
      return;
    }
    hashItem.mScore = score - this->mDistance;
  } else {
    // ����ɱ��ʱֱ�Ӽ�¼����
    hashItem.mScore = score;
  }
  // ��¼�߷�
  hashItem.mMove = move;
  // ��¼Zobrist
  hashItem.mZobrist = this->mZobrist;
}

// ����������ʷ��ɱ�ֱ�
inline void PositionInfo::setBestMove(const Move move, const Depth depth) {
  if (this->mKiller[this->mDistance][0] not_eq move) {
    this->mKiller[this->mDistance][1] = this->mKiller[this->mDistance][0];
    this->mKiller[this->mDistance][0] = move;
  }
  this->mHistory[move] += depth * depth;
}

// �߷�����
Count PositionInfo::moveGen(MoveList &moves, const Side side,
                            const MoveFlag capture) {
  Count totalMoves{0};
  // ��������
  for (Position start{51}; start < 204; start += 16) {
    for (Position pos{start}; pos < start + 9; ++pos) {
      // �ҵ��Լ�������
      if (this->mChessBoard[pos] and getChessSide(pos) == side) {
        switch (this->mChessBoard[pos] - side) {
        // ����ǽ�
        case KING:
          for (const auto &delta : KING_DELTA) {
            // ����ÿһ���������鿴������û���Լ�������
            if (IN_SQRT[pos + delta] and
                (capture ? isOppChess(pos + delta, side)
                         : isEmptyOrOppChess(pos + delta, side))) {
              // û�����ӻ��߲����Լ�������
              moves[totalMoves++] = toMove(pos, pos + delta);
            }
          }
          break;

        // �����ʿ
        case ADVISOR:
          for (const auto &delta : ADVISOR_DELTA) {
            // ����ÿһ���������鿴������û���Լ�������
            if (IN_SQRT[pos + delta] and
                (capture ? isOppChess(pos + delta, side)
                         : isEmptyOrOppChess(pos + delta, side))) {
              // û�����ӻ��߲����Լ�������
              moves[totalMoves++] = toMove(pos, pos + delta);
            }
          }
          break;

        // �������
        case BISHOP:
          for (Index index{0}; index < 4; ++index) {
            Delta delta = BISHOP_DELTA[index];
            Delta pin = BISHOP_PIN[index];
            // ����ÿһ���������ȿ��Ƿ����Լ��İ���ڣ���û����ס���ۣ��ٲ鿴������û���Լ�������
            if (isMyHalf(pos + delta, side) and isEmpty(pos + pin) and
                (capture ? isOppChess(pos + delta, side)
                         : isEmptyOrOppChess(pos + delta, side))) {
              // û�����ӻ��߲����Լ�������
              moves[totalMoves++] = toMove(pos, pos + delta);
            }
          }
          break;

        // �������
        case KNIGHT:
          for (Index index{0}; index < 4; ++index) {
            // �ȿ�������û���������������û�б�����
            // ���ȳ�����Χ�����������˾�ֱ����һ�֣���������һ��forѭ�������ж�����
            if (IN_BOARD[pos + KNIGHT_PIN[index]] and
                isEmpty(pos + KNIGHT_PIN[index])) {
              for (const auto &delta : KNIGHT_DELTA[index]) {
                // ����ÿһ���������ȿ��Ƿ��������ڣ��ٲ鿴������û���Լ�������
                if (IN_BOARD[pos + delta] and
                    (capture ? isOppChess(pos + delta, side)
                             : isEmptyOrOppChess(pos + delta, side))) {
                  // û�����ӻ��߲����Լ�������
                  moves[totalMoves++] = toMove(pos, pos + delta);
                }
              }
            }
          }
          break;

        // ����Ǳ�
        case PAWN:
          if (isMyHalf(pos, side)) {
            // ���û����
            Delta delta{getChessSide(pos) == RED ? RED_PAWN_DELTA
                                                 : BLACK_PAWN_DELTA};
            // �������֮���Ƿ��������ڣ��Ƿ��Լ������Ӷ�ס��
            if (IN_BOARD[pos + delta] and
                (capture ? isOppChess(pos + delta, side)
                         : isEmptyOrOppChess(pos + delta, side))) {
              moves[totalMoves++] = toMove(pos, pos + delta);
            }
          } else {
            // ������
            for (const auto &delta : getChessSide(pos) == RED
                                         ? PROMOTED_RED_PAWN_DELTA
                                         : PROMOTED_BLACK_PAWN_DELTA) {
              // �������֮���Ƿ��������ڣ��Ƿ��Լ������Ӷ�ס��
              if (IN_BOARD[pos + delta] and
                  (capture ? isOppChess(pos + delta, side)
                           : isEmptyOrOppChess(pos + delta, side))) {
                moves[totalMoves++] = toMove(pos, pos + delta);
              }
            }
          }
          break;

        // ����ǳ�
        case ROOK:
          for (const auto &delta : LINE_CHESS_DELTA) {
            Position cur{static_cast<Position>(pos + delta)};
            // �����λ��Ϊ��
            while (IN_BOARD[cur] and isEmpty(cur)) {
              if (not capture) {
                moves[totalMoves++] = toMove(pos, cur);
              }
              cur += delta;
            }
            // ���ܳ������̻���������һ������
            if (IN_BOARD[cur] and getChessSide(cur) not_eq side) {
              moves[totalMoves++] = toMove(pos, cur);
            }
          }
          break;

        // �������
        case CANNON:
          for (const auto &delta : LINE_CHESS_DELTA) {
            Position cur{static_cast<Position>(pos + delta)};
            // �����λ��Ϊ��
            while (IN_BOARD[cur] and isEmpty(cur)) {
              if (not capture) {
                moves[totalMoves++] = toMove(pos, cur);
              }
              cur += delta;
            }
            // �����˵�һ�����ӻ��߳�������
            cur += delta;
            while (IN_BOARD[cur] and isEmpty(cur)) {
              cur += delta;
            }
            // �����˵ڶ������ӻ��߳�������
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

// ��������
inline void PositionInfo::unMakeMove(const Move move, const Side side) {
  // ����ڵ�ľ���-1
  --this->mDistance;
  Position src = getSrc(move), dest = getDest(move);
  // ��ȡ���Ե������ӣ����Լ���ʷ�߷����С
  Chess victim = this->mHistoryMove[--this->mHistorySize].mVictim;
  // ��ԭ������Zobrist
  calcUnMove(move, victim, side);
  this->mChessBoard[src] = this->mChessBoard[dest];
  this->mChessBoard[dest] = victim;
}

// ����,�����Ƿ��߳ɹ�
inline bool PositionInfo::makeMove(const Move move, const Side side) {
  // ����ڵ�ľ����1
  ++this->mDistance;
  // ��ȡ��ʷ�߷�������������߷���ʷ��Ĵ�С
  MoveItem &moveItem = this->mHistoryMove[this->mHistorySize++];
  Position src = getSrc(move), dest = getDest(move);
  // �����ܺ���
  moveItem.mVictim = this->mChessBoard[dest];
  // ����Zobrist
  moveItem.mZobrist = this->mZobrist;
  // ���������Zobrist
  calcMove(move, side);
  // ����
  this->mChessBoard[dest] = this->mChessBoard[src];
  this->mChessBoard[src] = EMPTY;
  // ����Լ��Ƿ񱻽���
  if (isChecked(side)) {
    // ����������˾͸�ԭ
    unMakeMove(move, side);
    return false;
  }
  // ����ͱ��汾�����Ƿ񽫾��Է�����Ϣ
  moveItem.mCheck = isChecked(getOppSide(side));
  // �����߷���Ϣ
  moveItem.mMove = move;
  return true;
}

// ��ǰ�Ƿ�����߿ղ��������ڲоֽ׶�
inline bool PositionInfo::isNullOk(const Side side) {
  if (side == RED) {
    return this->mRedScore > 400;
  } else {
    return this->mBlackScore > 400;
  }
}

// ��һ���ղ�
inline void PositionInfo::makeNullMove() {
  // ����ڵ�ľ����1
  ++this->mDistance;
  // ��ȡ��ʷ�߷�������������߷���ʷ��Ĵ�С
  MoveItem &moveItem = this->mHistoryMove[this->mHistorySize++];
  // �����ܺ���
  moveItem.mVictim = EMPTY;
  // ����Zobrist
  moveItem.mZobrist = this->mZobrist;
  // ����ͱ��汾�����Ƿ񽫾��Է�����Ϣ
  moveItem.mCheck = false;
  // �����߷���Ϣ
  moveItem.mMove = INVALID_MOVE;
  // ���ߣ�����Zobrist
  this->mZobrist ^= SIDE_ZOBRIST;
}

// ������һ���ղ�
inline void PositionInfo::unMakeNullMove() {
  // ����ڵ�ľ���-1
  --this->mDistance;
  // �Լ���ʷ�߷����С
  --this->mHistorySize;
  // ��ԭZobrist
  this->mZobrist ^= SIDE_ZOBRIST;
}

SearchMachine::SearchMachine(PositionInfo &positionInfo, const Move hashMove,
                             const Side side)
    : mPositionInfo{positionInfo}, mSide{side}, mHash{hashMove} {
  // ��ȡ����ɱ���߷�
  mKiller1 = this->mPositionInfo.mKiller[this->mPositionInfo.mDistance][0];
  mKiller2 = this->mPositionInfo.mKiller[this->mPositionInfo.mDistance][1];
}

Move SearchMachine::nextMove() {
  switch (mNowPhase) {
  case PHASE_HASH:
    // ָ����һ���׶�
    this->mNowPhase = PHASE_KILLER1;
    // ȷ����һ���û����߷�����Ĭ���߷�
    if (this->mHash not_eq INVALID_MOVE) {
      return this->mHash;
    }
    // �������һ��
    [[fallthrough]];
  case PHASE_KILLER1:
    // ָ����һ���׶�
    this->mNowPhase = PHASE_KILLER2;
    // ȷ����һ��ɱ���߷�����Ĭ���߷��������û����߷�������Ҫȷ���Ƿ��ǺϷ��Ĳ�
    if (this->mKiller1 not_eq INVALID_MOVE and
        this->mKiller1 not_eq this->mHash and
        this->mPositionInfo.isLegalMove(this->mKiller1, this->mSide)) {
      return this->mKiller1;
    }
    // �������һ��
    [[fallthrough]];
  case PHASE_KILLER2:
    // ָ����һ���׶�
    this->mNowPhase = PHASE_GEN_MOVE;
    // ȷ����һ��ɱ���߷�����Ĭ���߷��������û����߷�
    if (this->mKiller2 not_eq INVALID_MOVE and
        this->mKiller2 not_eq this->mHash and
        this->mPositionInfo.isLegalMove(this->mKiller2, this->mSide)) {
      return this->mKiller2;
    }
    // �������һ��
    [[fallthrough]];
  case PHASE_GEN_MOVE:
    // ָ����һ���׶�
    this->mNowPhase = PHASE_ALL_MOVE;
    // �������е��߷���ʹ����ʷ������������
    this->mTotalMoves = this->mPositionInfo.moveGen(this->mMoves, this->mSide);
    sort(begin(this->mMoves), begin(this->mMoves) + this->mTotalMoves,
         [&](const Move &rhs, const Move &lhs) {
           return this->mPositionInfo.mHistory[lhs] <
                  this->mPositionInfo.mHistory[rhs];
         });
    // ֱ����һ��
    [[fallthrough]];
  case PHASE_ALL_MOVE:
    // �����߷��������鲢����
    while (this->mNowIndex < this->mTotalMoves) {
      Move move{this->mMoves[mNowIndex++]};
      if (move not_eq this->mHash and move not_eq this->mKiller1 and
          move not_eq this->mKiller2) {
        return move;
      }
    }
    // ���û���˾�ֱ�ӷ���
    [[fallthrough]];
  default:
    return INVALID_MOVE;
  }
}

// ��̬����
Score PositionInfo::searchQuiescence(Score alpha, const Score beta,
                                     const Side side) {
  // �ȼ���ظ����棬����ظ������־
  RepeatFlag repeatFlag{getRepeatFlag()};
  if (repeatFlag) {
    // ������ظ��������ֱ�ӷ��ط���
    return scoreRepeatFlag(repeatFlag);
  }
  MoveList moves;
  Count totalMoves;
  Score bestScore{LOSS_SCORE};
  if (this->mHistoryMove[this->mHistorySize - 1].mCheck) {
    // �����������,����ȫ���߷�
    totalMoves = moveGen(moves, side);
    // ������ʷ������
    sort(begin(moves), begin(moves) + totalMoves,
         [&](const Move &rhs, const Move &lhs) {
           return this->mHistory[lhs] < this->mHistory[rhs];
         });
  } else {
    // �������������������������
    Score tryScore = evaluate(side);
    if (tryScore > bestScore) {
      bestScore = tryScore;
      if (tryScore >= beta) {
        // Beta�ض�
        return tryScore;
      }
      if (tryScore > alpha) {
        // ��СAlpha-Beta�߽�
        alpha = tryScore;
      }
    }

    // �����������û�нضϣ������ɳ����߷�
    totalMoves = moveGen(moves, side, CAPTURE);
    // ����MVVLVA���߷���������
    sort(begin(moves), begin(moves) + totalMoves,
         [&](const Move &rhs, const Move &lhs) {
           return getMVVLVA(lhs) < getMVVLVA(rhs);
         });
  }

  // ���������߷�
  for (Index index{0}; index < totalMoves; ++index) {
    Move move{moves[index]};
    // ����������˾Ͳ�������һ��
    if (makeMove(move, side)) {
      // ��Ȼ�ͻ�����ֲ�������õķ���
      Score tryScore{static_cast<Score>(
          -searchQuiescence(-beta, -alpha, getOppSide(side)))};
      // ��������
      unMakeMove(move, side);
      if (tryScore > bestScore) {
        // �ҵ�����߷�(������ȷ����Alpha��PV����Beta�߷�)
        bestScore = tryScore;
        // �ҵ�һ��Beta�߷�
        if (tryScore >= beta) {
          // Beta�ض�
          return tryScore;
        }
        // �ҵ�һ��PV�߷�
        if (tryScore > alpha) {
          // ��СAlpha-Beta�߽�
          alpha = tryScore;
        }
      }
    }
  }

  // �����߷����������ˣ��������ֵ
  if (bestScore == LOSS_SCORE) {
    // �����ɱ�壬�͸���ɱ�岽����������
    return LOSS_SCORE + this->mDistance;
  }

  // ����ͷ������ֵ
  return bestScore;
}

// ��ȫ��������
Score PositionInfo::searchFull(Score alpha, const Score beta, const Depth depth,
                               const Side side, const NullFlag nullOk) {
  // �ﵽ��Ⱦͷ��ؾ�̬���ۣ����ڿ��Ųü�����ȿ���С��-1
  if (depth <= 0) {
    return searchQuiescence(alpha, beta, side);
  }

  // �ȼ���ظ����棬����ظ������־
  RepeatFlag repeatFlag{getRepeatFlag()};
  if (repeatFlag) {
    // ������ظ��������ֱ�ӷ��ط���
    return scoreRepeatFlag(repeatFlag);
  }

  // ��ǰ�߷�
  Move move;
  // �����û���ü������õ��û����߷�
  Score tryScore{probeHash(alpha, beta, depth, move)};
  if (tryScore > LOSS_SCORE) {
    // �û���ü��ɹ�
    return tryScore;
  }

  // ���пղ��ü������������������ղ���������ʱ�����߿ղ�
  // �����ڲо��в����߿ղ�����Ȼ����ԡ����С���һ��Ҫ����
  // ���ڵ��Betaֵ��"MATE_SCORE"�����Բ����ܷ����ղ��ü�)
  if (nullOk and not this->mHistoryMove[this->mHistorySize - 1].mCheck and
      isNullOk(side)) {
    // ��һ���ղ�
    makeNullMove();
    // ������֣���ȼ������Ųü��Ƽ������㣬Ȼ��������һ���ղ�����Ҫ�ټ���һ��
    tryScore =
        -searchFull(-beta, 1 - beta, depth - 3, getOppSide(side), NO_NULL);
    // �����ղ�
    unMakeNullMove();
    if (tryScore >= beta) {
      // ����㹻�þͿ��Է����ض�
      return tryScore;
    }
  }

  // ��������״̬��
  SearchMachine search{*this, move, side};
  // ����߷��ı�־
  HashFlag bestMoveHashFlag{HASH_ALPHA};
  Score bestScore{LOSS_SCORE};
  Move bestMove{INVALID_MOVE};
  // ���������߷�
  while ((move = search.nextMove())) {
    // ����������˾Ͳ�������һ��
    if (makeMove(move, side)) {
      // ��Ȼ�ͻ�����ֲ�������õķ���
      // �������죬��������˶Է��Ͷ��Ѽ���
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
      // ��������
      unMakeMove(move, side);
      if (tryScore > bestScore) {
        // �ҵ�����߷�(������ȷ����Alpha��PV����Beta�߷�)
        bestScore = tryScore;
        // �ҵ�һ��Beta�߷�
        if (tryScore >= beta) {
          // �����߷���־
          bestMoveHashFlag = HASH_BETA;
          // Beta�߷�Ҫ���浽��ʷ��
          bestMove = move;
          // Beta�ض�
          break;
        }
        // �ҵ�һ��PV�߷�
        if (tryScore > alpha) {
          // �����߷���־
          bestMoveHashFlag = HASH_PV;
          // PV�߷�Ҫ���浽��ʷ��
          bestMove = move;
          // ��СAlpha-Beta�߽�
          alpha = tryScore;
        }
      }
    }
  }

  // �����߷����������ˣ�������߷�(������Alpha�߷�)���浽��ʷ���������ֵ
  if (bestScore == LOSS_SCORE) {
    // �����ɱ�壬�͸���ɱ�岽����������
    return LOSS_SCORE + this->mDistance;
  }

  // ��¼���û���
  HASH_TABLE_LOCK.lockForWrite();
  recordHash(bestMoveHashFlag, bestScore, depth, bestMove);
  HASH_TABLE_LOCK.unlock();
  if (bestMove not_eq INVALID_MOVE) {
    // �������Alpha�߷����ͽ�����߷����浽��ʷ��ɱ�ֱ��û���
    setBestMove(bestMove, depth);
  }
  return bestScore;
}

// ���ڵ������
Score PositionInfo::searchRoot(const Depth depth) {
  // ��ǰ�߷�
  Move move;
  // ��������״̬��
  SearchMachine search{*this, INVALID_MOVE, COMPUTER_SIDE};
  Score bestScore{LOSS_SCORE};
  // ���������߷�
  while ((move = search.nextMove())) {
    // ����������˾Ͳ�������һ��
    if (makeMove(move, COMPUTER_SIDE)) {
      // ��Ȼ�ͻ�����ֲ�������õķ���
      Score tryScore;
      // �������죬��������˶Է��Ͷ��Ѽ���
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
      // ��������
      unMakeMove(move, COMPUTER_SIDE);
      if (tryScore > bestScore) {
        // �ҵ�����߷�
        bestScore = tryScore;
        this->mBestMove = move;
      }
    }
  }
  // ��¼���û���
  HASH_TABLE_LOCK.lockForWrite();
  recordHash(HASH_PV, bestScore, depth, this->mBestMove);
  HASH_TABLE_LOCK.unlock();
  setBestMove(this->mBestMove, depth);
  return bestScore;
}

string PositionInfo::fenGen() {
  string fen{""};
  char spaceBetween{'0'};
  // ��������
  for (Position start{51}; start < 204; start += 16) {
    for (Position pos{start}; pos < start + 9; ++pos) {
      switch (this->mChessBoard[pos]) {
        // �յ�
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
    // һ�����Ŀո�
    if (spaceBetween != '0') {
      fen += spaceBetween;
      spaceBetween = '0';
    }
    fen += '/';
  }
  // �Ƴ����һ����ӵ�/
  fen.erase(--end(fen));
  fen += COMPUTER_SIDE == RED ? " w" : " b";
  return fen;
}

tuple<QString, Step> PositionInfo::searchBook() {
  HttpRequest httpReq{"112.73.74.24", 80};
  string res =
      httpReq.HttpGet("/chessdb.php?action=queryall&board=" + fenGen());
  //δ�������ȡʧ��
  if (res.empty()) {
    return {QString::fromLocal8Bit("��������"), {0, 0, 0, 0}};
  }
  //�ָ�ȡ�߷�
  vector<string> splitRes = httpReq.split(res, "\n");
  vector<string> parse = httpReq.split(*--splitRes.end(), ":");
  //�ƿ��޶�Ӧ�з�
  if (parse.at(0) != "move") {
    return {QString::fromLocal8Bit("��������"), {0, 0, 0, 0}};
  } else {
    //����
    uint16_t nowRow = 9 - (parse.at(1).at(1) - '0');
    uint16_t nowCol = parse.at(1).at(0) - 'a';
    uint16_t destRow = 9 - (parse.at(1).at(3) - '0');
    uint16_t destCol = parse.at(1).at(2) - 'a';
    return {QString::fromLocal8Bit("�ƿ����"),
            {nowRow, nowCol, destRow, destCol}};
  }
}

// ���������
Score searchMain() {
  // ������Ϣ
  resetCache(POSITION_INFO);
  // ��������������
  CURRENT_DEPTH = 1;
  Score bestScore{0};
  // ���߳�������ÿ���߳�һ��PositionInfo��ֻ���û����ǹ����
  // Ϊʲô��MAX_CONCURRENT - 1?
  // ��Ϊ�����������߳�ȥ�������߳�ҲҪ����������ռ��һ������
  QVector<PositionInfo> threadPositionInfos(MAX_CONCURRENT - 1);
  // ��ʼ���߳̾���
  for (auto &threadPositionInfo : threadPositionInfos) {
    threadPositionInfo.setThreadPositionInfo(POSITION_INFO);
  }
  // ʱ�����
  auto startTimeStamp = clock();
  forever {
    // ���߳�����
    QVector<QFuture<void>> tasks;
    tasks.reserve(MAX_CONCURRENT - 1);
    for (auto &threadPositionInfo : threadPositionInfos) {
      tasks.emplaceBack(QtConcurrent::run(
          [&] { threadPositionInfo.searchRoot(CURRENT_DEPTH); }));
    }
    // �Լ�Ҳ����
    bestScore = POSITION_INFO.searchRoot(CURRENT_DEPTH);
    // �ȴ��߳̽���
    for (auto &task : tasks) {
      task.waitForFinished();
    }
    ++CURRENT_DEPTH;
    // ���Ӯ�˻������˾Ͳ���������������
    if (bestScore < LOST_SCORE or bestScore > WIN_SCORE) {
      break;
    }
    // ����ʱ���ֹͣ����
    if (clock() - startTimeStamp > SEARCH_TIME) {
      break;
    }
  }
  // ����
  POSITION_INFO.makeMove(POSITION_INFO.mBestMove, COMPUTER_SIDE);
  return bestScore;
}

// ���ڳ�ʼ������
inline void init() {
  // ��ʼ������
  POSITION_INFO.resetBoard();
  // ��ʼ��CHESS_FEN
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
  // ����ѡ�߻ָ�
  COMPUTER_SIDE = BLACK;
  // ��ʼ��Zobrist��
  // ÷ɭ�������������
  mt19937 eng(time(NULL));
  // ����ƽ���ֲ�
  uniform_int_distribution<ZobristValue> uniDist;
  // ÿһ�����ӱ�� 1 - 7  9 - 15
  for (Chess chess{1}; chess < 16; ++chess) {
    if (chess == 8) {
      continue;
    }
    // ÿһ��λ�� 51 - 203
    for (Position pos{51}; pos < 204; ++pos) {
      CHESS_ZOBRIST[chess][pos] = uniDist(eng);
    }
  }
  // ��ʼ��ѡ�����ֵ
  SIDE_ZOBRIST = uniDist(eng);
}
