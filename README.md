# 中国象棋

#### 介绍
大学C++项目，中国象棋Qt界面与AI象棋引擎\
引擎算法基于超出边界（Fail-Soft）的AlphaBeta剪枝\
支持历史表启发，杀手启发，吃子启发\
支持置换表裁剪、不带验证的空着裁剪(残局不启用)、主要变例搜索、Lazy-SMP多线程搜索\
支持将军延伸和重复局面检测\
联网的情况下支持ChessDB提供的开局库、对局库和残局库，大约可提升引擎200ELO左右

#### 引擎棋力（在联网情况、四核机器下）

平均状态下1秒可上8层，3秒上9层，10秒可到10层\
目前足以应对一般的纯人，但由于搜索速度和评分函数知识上的缺陷，暂不足以应对任何其他象棋软件(免费与商业)。

#### 天天象棋测试（在联网情况、四核机器下，调成普通难度，一步至少3秒，大约上到10层）
可战胜业8纯人，得出本软件ELO大约为2000左右\
天天象棋人机对战可以战胜精英级别电脑（天天象棋分析12层）\
由此可得本软件大致与新版天天象棋分析13层相当\
实战测试结果最高等级如下(该账号仅用于测试软件棋力，由于达到业余9-1后，再往后的测试需要实名认证，鉴于已经达到了测试的目的，所以该账号现已注销)：
![评测最高等级](https://images.gitee.com/uploads/images/2021/0823/185211_45f94b91_7628839.jpeg "QQ图片20210823185009.jpg")

#### 软件架构
Qt、C++

#### 开发环境
Qt 6.2.0 + mingw 8.1.0 x64 with posix thread model

#### 使用说明
打开可执行即可运行程序

#### 参与贡献
PikaCat

#### 代码参考
象棋小巫师：https://www.xqbase.com/computer/stepbystep1.htm

云开局库、残局库：https://www.chessdb.cn/query/

想要做出更好的引擎，请参考：https://github.com/xqbase/eleeye/tree/master/eleeye