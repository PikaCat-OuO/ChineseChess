# 中国象棋

#### 介绍
个人项目，中国象棋Qt界面与AI象棋引擎\
棋盘结构为 **PEXT位棋盘** ，使用CPU中128位寄存器的低90位来存储棋盘，对应C++的数据结构为__m128i\
使用了 **POPCNT指令，BMI位操作指令集中的PEXT与TZCNT指令，SSE指令集中的与、或、非、异或、零测试** 等指令来进行走法预生成与快速运算，需要相应的CPU支持\
引擎算法基于超出边界（Fail-Soft）的AlphaBeta剪枝\
支持历史表启发，杀手启发，吃子启发，有良好的走法排序器\
支持置换表裁剪、带验证的空着裁剪、剃刀裁剪、静态评分裁剪、落后着法衰减\
支持将军延伸和重复局面检测（只支持长将检测，目前不支持长捉检测，一将一捉等检测）\
支持主要变例搜索、使用OpenMP与QtConcurrent并发库进行Lazy-SMP多线程搜索\
联网的情况下支持ChessDB提供的开局库、对局库和残局库，大约可提升引擎200ELO左右

#### 引擎棋力（在联网情况、四核机器下，使用简单难度）
足以应对一般的纯人，但由于搜索速度和评分函数知识上的缺陷，暂不足以应对任何其他象棋软件(免费与商业)。

#### 天天象棋测试（在联网情况、四核机器下，使用简单难度）
可战胜业8纯人，得出本软件ELO大约为2000左右\
天天象棋人机对战可以战胜精英级别电脑（天天象棋分析12层）\
由此可得本软件大致与新版天天象棋分析13层相当\
实战测试结果最高等级如下(该账号仅用于测试软件棋力，由于达到业余9-1后，再往后的测试需要实名认证，鉴于已经达到了测试的目的，所以该账号现已注销)：
![评测最高等级](https://images.gitee.com/uploads/images/2021/0823/185211_45f94b91_7628839.jpeg "QQ图片20210823185009.jpg")

#### JJ象棋测试（在联网情况、四核机器下，使用简单难度）
实战测试可战胜特大等级纯人，最高达到荣誉顶级\
100盘胜率94%，有1盘掉线，1盘与其他软件作和，4盘输给其他软件，其余与纯人对战都赢了\
该账号仅用于测试软件棋力，由于特大等级的小部分人和荣誉顶级的绝大部分人都是软件，由于本软件不具备与其他软件对撕的能力，鉴于已经达到了测试的目的，故不再往后测试
![评测最高等级](https://images.gitee.com/uploads/images/2021/0921/212032_434c1039_7628839.jpeg "Screenshot_2021-09-21-21-16-53-960_cn.jj.chess.mi.jpg")

#### 软件架构
Qt、C++

#### 开发环境
Qt 6.2.3 + mingw 11.2.0 x64 with posix thread model

#### 语言标准
C++23 编译时开启 **-std=gnu++2b** 

#### 使用说明
打开可执行即可运行程序

#### 参与贡献
PikaCat

#### 云开局库、残局库
https://www.chessdb.cn/query/

#### 参考文献
象棋百科全书：https://www.xqbase.com/computer.htm\
象棋编程维基百科: https://www.chessprogramming.org/Main_Page

#### 参考代码
象棋小巫师: https://gitee.com/SpiritFinches/xqwizard/tree/master/XQWLIGHT/Win32\
象眼: https://gitee.com/SpiritFinches/xqwizard/tree/master/ELEEYE\
国际象棋位棋盘: https://github.com/maksimKorzh/bbc