# 中国象棋

#### 介绍
+ 个人项目，中国象棋Qt界面与AI象棋引擎
+ 棋盘结构为 **PEXT位棋盘** ，使用CPU中128位寄存器的低90位来存储棋盘，对应C++的数据结构为__m128i
+ 使用了 **POPCNT指令，BMI位操作指令集中的PEXT与TZCNT指令，SSE指令集中的与、或、非、异或、零测试，AVX2指令集** 等指令来进行走法预生成与快速运算，需要相应的CPU支持
+ 引擎算法基于超出边界（Fail-Soft）的AlphaBeta剪枝，使用迭代加深（含内部迭代加深）的搜索方式
+ 在局面评价上使用NNUE（快速可更新的神经网络）对进行局面评估
+ 支持历史表启发，杀手启发，吃子启发，有良好的走法排序器
+ 支持基于SSE的无锁置换表裁剪、带验证的空着裁剪、落后走法衰减、落后走法裁剪、杀棋步数裁剪、无用裁剪、差值裁剪
+ 支持将军延伸和重复局面检测（支持长将检测和部分长捉检测）
+ 支持主要变例搜索、使用OpenMP与QtConcurrent并发库进行Lazy-SMP多线程搜索
+ 联网的情况下支持ChessDB提供的开局库、对局库和残局库，大约可提升引擎200ELO左右

#### 软件架构
+ Qt、C++

#### 开发环境
+ 集成开发环境: Qt最新版
+ 编译器: Qt最新版自带的最新版MinGW 

#### 语言标准
+ C++最新标准，开启GNU最新的语言级别扩展特性

#### 引擎棋力（非NNUE版本，使用云库、CPU:i5-8265U）
+ 足以应对一般的纯人，但由于搜索速度上的缺陷，暂不足以应对其他优秀的象棋软件(如象棋旋风)。
+ 与一般的象棋引擎的对战评测在b站：
    + 对战悟空象棋引擎：https://www.bilibili.com/video/BV1TF41147Do/ 
    + 对战象棋小巫师引擎：https://www.bilibili.com/video/BV1va411h7yo/ 
    + 对战象眼引擎：https://www.bilibili.com/video/BV1Q34y1b7PA/

#### 天天象棋测试（非NNUE版本，使用云库、CPU:i5-8265U）
+ 可战胜业8-3纯人，得出本软件ELO大约为2000左右
+ 天天象棋人机对战可以战胜精英级别电脑（天天象棋分析12层），由此可得本软件大致与新版天天象棋分析13层相当。
+ 实战测试结果最高等级如下(该账号仅用于测试软件棋力，由于达到业余9-1后，再往后的测试需要实名认证，鉴于已经达到了测试的目的，所以该账号现已注销)：
![评测最高等级](https://images.gitee.com/uploads/images/2021/0823/185211_45f94b91_7628839.jpeg "QQ图片20210823185009.jpg")
+ 更多实战测试的内容在：https://www.bilibili.com/video/BV1eR4y1j777

#### JJ象棋测试（非NNUE版本，使用云库、CPU:i5-8265U）
+ 实战测试可战胜特大等级纯人，最高达到荣誉顶级，100盘胜率94%，有1盘掉线，1盘与其他软件作和，4盘输给其他软件，其余与纯人对战都赢了
+ 该账号仅用于测试软件棋力，由于特大等级的小部分人和荣誉顶级的绝大部分人都是软件，由于本软件不具备与其他软件对撕的能力，鉴于已经达到了测试的目的，故不再往后测试
![评测最高等级](https://images.gitee.com/uploads/images/2021/0921/212032_434c1039_7628839.jpeg "Screenshot_2021-09-21-21-16-53-960_cn.jj.chess.mi.jpg")

#### 使用说明
+ 打开可执行即可运行程序

#### 参与贡献
+ PikaCat

#### 未来愿景
+ 这个引擎目前还有很多不完善的地方((＞﹏＜)一大堆捏~)：
    1. 没有发挥出位棋盘该有的速度，相比于数组棋盘提升幅度不是很大，所以对应的程序实现还有很多未被发现的Bug没有解决。
    2. 搜索速度不快，剪枝力度不够大。
    3. 没有UCI协议支持，目前无法使用命令模式将引擎与界面解耦。
    4. 没有引擎ELO测评平台，如CCRL。
    5. 没有测试平台，如fishtest。

+ 建立这个仓库的初心是看到国际象棋Stockfish引擎的开源仓库及其开源社区支持的强大支持，于是想着能不能在国内也建立一个这样的仓库，让更多象棋引擎爱好者参与引擎的改进，更新，提issue，提pull requests，众人拾柴火焰高。就像Stockfish超过商业引擎Komodo一样，有一天我们也能够媲美象棋旋风。
+ 我曾经看到过一句话，我很喜欢：If you love something, set it free. 来自虚幻引擎的官网。这里的free有两种意思，免费与自由。所以如果你喜欢一样东西，想让它变好，就让它免费吧，让它可以被它人自由获取吧！这也是我为什么要开源的原因，这也是我为什么使用WTFPL的原因。
+ 作为一条咸鱼ヾ(•ω•`)o，梦想还是要有的，万一实现了呢？

#### 云开局库、残局库
+ https://www.chessdb.cn/query/

#### 特别感谢
+ 特别感谢ianfab编写的NNUE工具链以及Belzedar94提供的权重文件，让皮卡喵象棋引擎搭上了NNUE的时代快车
+ 特别感谢ianfab耐心解答我的解惑，使得皮卡喵NNUE成为可能。https://github.com/ianfab/Fairy-Stockfish/discussions/491
+ 以下是ianfab提供的NNUE工具链：
    1. 训练数据生成器：https://github.com/ianfab/variant-nnue-tools
    2. NNUE网络训练器：https://github.com/ianfab/variant-nnue-pytorch
+ NNUE的最新参数文件（皮卡喵象棋的nnue文件会与其保持同步更新）：https://fairy-stockfish.github.io/nnue/#current-best-nnue-networks

#### 参考文献
1. 象棋百科全书：https://www.xqbase.com/computer.htm
2. 象棋编程维基百科：https://www.chessprogramming.org/Main_Page
3. Shark象棋引擎论文：http://rportal.lib.ntnu.edu.tw/bitstream/20.500.12235/106625/1/n060147070s01.pdf
4. NNUE神经网络手册：https://github.com/glinscott/nnue-pytorch/blob/master/docs/nnue.md

#### 参考代码
1. 象棋小巫师: https://github.com/xqbase/xqwlight
2. 象眼: https://github.com/xqbase/eleeye
3. 国际象棋位棋盘: https://github.com/maksimKorzh/bbc
4. 佳佳象棋：https://github.com/leedavid/NewGG
5. Fairy-Stockfish：https://github.com/ianfab/Fairy-Stockfish