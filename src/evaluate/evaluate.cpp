#include "chessboard.h"
#include <fstream>

namespace PikaChess {
/** 输入特征转换器，由于参数都在里面，所以使用大页面管理 */
LargePagePtr<FeatureTransformer> featureTransformer;

/** 模型剩余的部分使用对齐页面 */
AlignedPtr<Model> model[LAYER_STACKS];

/** NNUE的文件名和网络描述信息 */
std::string fileName { "xiangqi.nnue" };
std::string netDescription;

/** 将特征转换器和模型的权重和偏差置为空 */
template <typename T>
void ZeroParameters(AlignedPtr<T> &pointer) {
  pointer.reset((T*)_mm_malloc(sizeof(T), alignof(T)));
  std::memset(pointer.get(), 0, sizeof(T));
}

template <typename T>
void ZeroParameters(LargePagePtr<T> &pointer) {
  pointer.reset((T*)(AlignedLargePageAlloc(sizeof(T))));
  std::memset(pointer.get(), 0, sizeof(T));
}

void ZeroParameters() {
  ZeroParameters(featureTransformer);
  for (quint8 i = 0; i < LAYER_STACKS; ++i) ZeroParameters(model[i]);
}

/** 读取NNUE文件的头部信息 */
bool ReadHeader(std::istream &stream) {
  // 首先读取版本信息并校验
  if (ReadInt<quint32>(stream) not_eq VERSION) return false;

  // 接着读取整个NNUE文件的哈希值并校验
  if (ReadInt<quint32>(stream) not_eq HASH_VALUE_FILE) return false;

  // 读取文件描述信息的长度
  quint32 length = ReadInt<quint32>(stream);
  netDescription.resize(length);
  // 读取文件描述信息
  stream.read(const_cast<char *>(netDescription.c_str()), length);

  return not stream.fail();
}

/** 读取某层网络的权重和偏差 */
template <typename T>
bool ReadParameters(std::istream &stream, T &layer) {
  // 首先读取该层的HASH值并校验
  if (not stream or ReadInt<quint32>(stream) not_eq T::getHashValue()) return false;
  // 随之调用该层的读参数方法
  return layer.readParameters(stream);
}

/** 读取所有网络层的权重和偏差 */
bool ReadParameters(std::istream& stream) {
  // 首先读取头部信息
  if (not ReadHeader(stream)) return false;

  // 接着读取特征转换器的权重和偏差
  if (not ReadParameters(stream, *featureTransformer)) return false;

  // 接着读取每一层的权重和偏差
  for (quint8 i = 0; i < LAYER_STACKS; ++i) {
    if (not ReadParameters(stream, *model[i])) return false;
  }

  // 最后检查是否已经读到了文件末尾符号EOF
  return stream and stream.peek() == std::ios::traits_type::eof();
}

/** 从NNUE文件中初始化所有的内容 */
void NNUEInit() {
  ZeroParameters();
  std::ifstream nnueFile { fileName, std::ios::binary };
  if (not ReadParameters(nnueFile)) throw "读取NNUE神经网络参数失败";
}

/** 获得局面评分 */
qint16 Chessboard::score() {
  // 存储中间结果的空间
  alignas(CACHE_LINE_SIZE) quint8 transformedFeatures[FeatureTransformer::BufferSize];
  alignas(CACHE_LINE_SIZE) char buffer[Model::BufferSize];

  /* bucket有点像以前的渐进式评分函数的局面阶段（开局->中局->残局），不同的阶段采用不同的评分模型
   * HalfKAv2有8份小的评分模型，分别对应局面的8个阶段，按照下面的公式计算 */
  const quint8 bucket = (this->m_piece - 1) / 4;
  const auto psqt = featureTransformer->transform(this->getLastMove().m_acc, this->m_side,
                                                  transformedFeatures, bucket);
  const auto output = model[bucket]->propagate(transformedFeatures, buffer);

  return (psqt + output[0]) >> OUTPUT_SCALE_BITS;
}
}
