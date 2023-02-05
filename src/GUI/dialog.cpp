#include "dialog.h"
#include "ui_dialog.h"

Dialog::Dialog(QWidget *parent) : QDialog(parent), ui(new Ui::Dialog) {
  ui->setupUi(this);
  this->initDialog();
  this->initChess();
  this->m_chessEngine = new PikaChess::ChessEngine;
}

void Dialog::initDialog() {
  //安装标题栏按钮事件监控
  ui->CloseButton->installEventFilter(this);
  ui->MinButton->installEventFilter(this);
  //设置无边框窗口
  setWindowFlag(Qt::FramelessWindowHint);
  //设置窗口透明
  setAttribute(Qt::WA_TranslucentBackground);
  //设置窗口在屏幕正中央显示
  auto desk = QApplication::primaryScreen()->geometry();
  move((desk.width() - this->width()) / 2,
       (desk.height() - this->height()) / 2);
  //窗口启动动画
  QPropertyAnimation *ani = new QPropertyAnimation(this, "windowOpacity");
  ani->setDuration(600);
  ani->setStartValue(0);
  ani->setEndValue(0.94);
  ani->setEasingCurve(QEasingCurve::InOutSine);
  ani->start(QPropertyAnimation::DeleteWhenStopped);
  //设置窗口阴影
  QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect;
  shadow->setOffset(0, 0);
  shadow->setColor(QColor(0, 0, 0, 50));
  shadow->setBlurRadius(10);
  ui->frame->setGraphicsEffect(shadow);
}

void Dialog::initChess() {
  //安装棋盘事件监控
  ui->ChessBoard->installEventFilter(this);
  //安装mask2事件监控
  ui->Mask2->installEventFilter(this);
  //安装所有的棋子事件监控
  ui->RedChe1->installEventFilter(this);
  ui->RedChe2->installEventFilter(this);
  ui->RedMa1->installEventFilter(this);
  ui->RedMa2->installEventFilter(this);
  ui->RedPao1->installEventFilter(this);
  ui->RedPao2->installEventFilter(this);
  ui->RedBing1->installEventFilter(this);
  ui->RedBing2->installEventFilter(this);
  ui->RedBing3->installEventFilter(this);
  ui->RedBing4->installEventFilter(this);
  ui->RedBing5->installEventFilter(this);
  ui->RedShi1->installEventFilter(this);
  ui->RedShi2->installEventFilter(this);
  ui->RedXiang1->installEventFilter(this);
  ui->RedXiang2->installEventFilter(this);
  ui->RedJiang->installEventFilter(this);
  ui->BlackJiang->installEventFilter(this);
  ui->BlackChe1->installEventFilter(this);
  ui->BlackChe2->installEventFilter(this);
  ui->BlackMa1->installEventFilter(this);
  ui->BlackMa2->installEventFilter(this);
  ui->BlackPao1->installEventFilter(this);
  ui->BlackPao2->installEventFilter(this);
  ui->BlackBing1->installEventFilter(this);
  ui->BlackBing2->installEventFilter(this);
  ui->BlackBing3->installEventFilter(this);
  ui->BlackBing4->installEventFilter(this);
  ui->BlackBing5->installEventFilter(this);
  ui->BlackShi1->installEventFilter(this);
  ui->BlackShi2->installEventFilter(this);
  ui->BlackXiang1->installEventFilter(this);
  ui->BlackXiang2->installEventFilter(this);
  //不可视化mask
  ui->Mask1->setVisible(false);
  ui->Mask2->setVisible(false);
  ui->Mask3->setVisible(false);
  //初始化指针数组
  mLabelPointers = {
      {ui->BlackChe1, ui->BlackMa1, ui->BlackXiang1, ui->BlackShi1,
       ui->BlackJiang, ui->BlackShi2, ui->BlackXiang2, ui->BlackMa2,
       ui->BlackChe2},
      {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
       nullptr},
      {nullptr, ui->BlackPao1, nullptr, nullptr, nullptr, nullptr, nullptr,
       ui->BlackPao2, nullptr},
      {ui->BlackBing1, nullptr, ui->BlackBing2, nullptr, ui->BlackBing3,
       nullptr, ui->BlackBing4, nullptr, ui->BlackBing5},
      {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
       nullptr},
      {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
       nullptr},
      {ui->RedBing1, nullptr, ui->RedBing2, nullptr, ui->RedBing3, nullptr,
       ui->RedBing4, nullptr, ui->RedBing5},
      {nullptr, ui->RedPao1, nullptr, nullptr, nullptr, nullptr, nullptr,
       ui->RedPao2, nullptr},
      {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
       nullptr},
      {ui->RedChe1, ui->RedMa1, ui->RedXiang1, ui->RedShi1, ui->RedJiang,
       ui->RedShi2, ui->RedXiang2, ui->RedMa2, ui->RedChe2}};
  //设置走棋动画的部分参数
  mChessMoveAni->setEasingCurve(QEasingCurve::InOutCubic);
  mChessEatAni->setEasingCurve(QEasingCurve::InOutCubic);
  mComputerMoveAni->setEasingCurve(QEasingCurve::InOutCubic);
  mMaskAni->setTargetObject(ui->Mask3);
  mMaskAni->setEasingCurve(QEasingCurve::InOutCubic);
  //注册类型
  qRegisterMetaType<uint16_t>("uint16_t");
  //走棋动画走完要清空已经选中的子并让电脑走子
  connect(mChessMoveAni, &QPropertyAnimation::finished, [&] {
    mSelected = nullptr;
    computerMove();
  });
  //吃子吃完要将被吃的子力设置为不可见
  connect(mChessEatAni, &QPropertyAnimation::finished, [&] {
    mSelected = nullptr;
    mTarget->setVisible(false);
    mTarget = nullptr;
    computerMove();
  });
  //连接电脑计算完成信号和走子槽函数
  connect(this, &Dialog::threadOK, this, &Dialog::makeMove);
  //连接电脑走子后的处理槽函数
  connect(mComputerMoveAni, &QPropertyAnimation::finished, [&] {
    //如果是吃子走法，要将对方的子力设置为不可见
    if (mTarget) {
      mTarget->setVisible(false);
    }
    mSelected = nullptr;
    mTarget = nullptr;
    // 走完了，看电脑是否已经胜利
    if (this->mComputerWin) {
      // 电脑赢了，注意此时isMoving保持为true，这样可以使得棋子不响应玩家的操作
      this->setButtonDisabled(false);
    } else {
      // 否则就正常回复按钮和isMoving的状态
      this->setMoving(false);
    }
  });
}

Dialog::~Dialog() { delete ui; delete this->m_chessEngine; }
//重写关闭事件实现窗口关闭动画
void Dialog::closeEvent(QCloseEvent *event) {
  if (this->mCloseCheck == false) {
    this->mCloseCheck = true;
    event->ignore();
    //窗口关闭动画
    QPropertyAnimation *ani = new QPropertyAnimation(this, "windowOpacity");
    ani->setDuration(600);
    ani->setStartValue(0.94);
    ani->setEndValue(0);
    ani->setEasingCurve(QEasingCurve::InOutSine);
    ani->start(QPropertyAnimation::DeleteWhenStopped);
    connect(ani, &QPropertyAnimation::finished, this, &Dialog::close);
  } else {
    event->accept();
  }
}

//重写鼠标移动事件，实现点击窗口任意处移动窗口的功能
void Dialog::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    this->mOnDialog = true;
    this->mMouseStartPoint = event->globalPosition().toPoint();
    this->mDialogStartPoint = this->frameGeometry().topLeft();
  }
}

void Dialog::mouseMoveEvent(QMouseEvent *event) {
  if (event->buttons() == Qt::LeftButton && this->mOnDialog) {
    QPoint MouseMoveDelta =
        event->globalPosition().toPoint() - this->mMouseStartPoint;
    this->move(this->mDialogStartPoint + MouseMoveDelta);
  } else {
    //单击按住按钮后离开按钮区域，按钮回复原样
    this->mColorClose = QColor(212, 64, 39, 0);
    QString qssClose = QString("#CloseButton{border-image:url(:/Images/"
                               "close.ico);border-radius:5px;background: "
                               "rgba(212,64,39,0);}#CloseButton:pressed{border-"
                               "image:url(:/Images/close_press.ico);}");
    ui->CloseButton->setStyleSheet(qssClose);
    this->mColorMin = QColor(38, 169, 218, 0);
    QString qssMin = QString("#MinButton{border-image:url(:/Images/"
                             "min.ico);border-radius:5px;background: "
                             "rgba(38,169,218,0);}#MinButton:pressed{border-"
                             "image:url(:/Images/min_press.bmp);}");
    ui->MinButton->setStyleSheet(qssMin);
  }
}

void Dialog::mouseReleaseEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    this->mOnDialog = false;
  }
}

//实现按Esc键关闭窗口时也能播放动画
void Dialog::keyPressEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_Escape) {
    this->close();
  }
}

//标题栏按钮动画
bool Dialog::eventFilter(QObject *watched, QEvent *event) {
  //关闭按钮点燃效果
  if (watched == ui->CloseButton) {
    if (event->type() == QEvent::Enter) {
      QPropertyAnimation *ani = new QPropertyAnimation(this, "mColorClose");
      ani->setDuration(100);
      ani->setStartValue(getColorClose());
      ani->setEndValue(QColor(212, 64, 39, 230));
      ani->start(QPropertyAnimation::DeleteWhenStopped);
      return true;
    }
    if (event->type() == QEvent::Leave) {
      QPropertyAnimation *ani = new QPropertyAnimation(this, "mColorClose");
      ani->setDuration(100);
      ani->setStartValue(getColorClose());
      ani->setEndValue(QColor(212, 64, 39, 0));
      ani->start(QPropertyAnimation::DeleteWhenStopped);
      return true;
    }
  }
  //最小化按钮点燃效果
  if (watched == ui->MinButton) {
    if (event->type() == QEvent::Enter) {
      QPropertyAnimation *ani = new QPropertyAnimation(this, "mColorMin");
      ani->setDuration(100);
      ani->setStartValue(getColorMin());
      ani->setEndValue(QColor(38, 169, 218, 230));
      ani->start(QPropertyAnimation::DeleteWhenStopped);
      return true;
    }
    if (event->type() == QEvent::Leave) {
      QPropertyAnimation *ani = new QPropertyAnimation(this, "mColorMin");
      ani->setDuration(100);
      ani->setStartValue(getColorMin());
      ani->setEndValue(QColor(38, 169, 218, 0));
      ani->start(QPropertyAnimation::DeleteWhenStopped);
      return true;
    }
  }

  //将棋子和操作连接起来
  if (!this->mOnMoving and event->type() == QEvent::MouseButtonPress and
      this->isChess(watched)) {
    mTarget = dynamic_cast<QLabel *>(watched);
    if (mSelected != nullptr and mSelected->text() != mTarget->text()) {
      //吃子时要改动指针数组和棋盘
      uint16_t nowRow = (mSelected->y() - ui->ChessBoard->y() - 10) / 80,
               nowCol = (mSelected->x() - ui->ChessBoard->x() - 10) / 80,
               destRow = (mTarget->y() - ui->ChessBoard->y() - 10) / 80,
               destCol = (mTarget->x() - ui->ChessBoard->x() - 10) / 80;
      if (this->mIsFliped) {
        nowRow = 9 - nowRow;
        nowCol = 8 - nowCol;
        destRow = 9 - destRow;
        destCol = 8 - destCol;
      }
      PikaChess::Move move;
      move.setMove(nowRow * 9 + nowCol, destRow * 9 + destCol);
      if (not this->canMove(move)) {
        event->ignore();
        return true;
      }
      // 可以走，先把棋子锁上
      this->setMoving(true);
      // 走子
      this->playerMakeMove({nowRow, nowCol, destRow, destCol});
      //隐藏mask
      ui->Mask1->setVisible(false);
      ui->Mask2->setVisible(false);
      ui->Mask3->setVisible(false);
      //移动mask
      ui->Mask2->move(mSelected->x(), mSelected->y());
      ui->Mask3->move(mSelected->x(), mSelected->y());
      ui->Mask2->setVisible(true);
      ui->Mask3->setVisible(true);
      // Mask3动画
      mMaskAni->setStartValue(QRect(ui->Mask3->x(), ui->Mask3->y(),
                                    ui->Mask3->width(), ui->Mask3->height()));
      mMaskAni->setEndValue(QRect(mTarget->x(), mTarget->y(),
                                  ui->Mask3->width(), ui->Mask3->height()));
      mMaskAni->start();
      //吃子做动画
      mChessEatAni->setTargetObject(mSelected);
      mChessEatAni->setStartValue(QRect(mSelected->x(), mSelected->y(),
                                        mSelected->width(),
                                        mSelected->height()));
      mChessEatAni->setEndValue(QRect(mTarget->x(), mTarget->y(),
                                      mSelected->width(), mSelected->height()));
      mChessEatAni->start();
    } else {
      // 在选棋时不是自己方的不让选
      if (mTarget->text() == (this->m_chessEngine->side() ? "b" : "r")) {
        mSelected = mTarget;
        mSelected->raise();
        if (ui->Mask1->isVisible()) {
          QPropertyAnimation *maskAni =
              new QPropertyAnimation(ui->Mask1, "geometry");
          maskAni->setEasingCurve(QEasingCurve::InOutCubic);
          maskAni->setStartValue(QRect(ui->Mask1->x(), ui->Mask1->y(),
                                       ui->Mask1->width(),
                                       ui->Mask1->height()));
          maskAni->setEndValue(QRect(mSelected->x(), mSelected->y(),
                                     ui->Mask1->width(), ui->Mask1->height()));
          maskAni->start(QPropertyAnimation::DeleteWhenStopped);
        } else {
          ui->Mask1->move(mSelected->x(), mSelected->y());
          ui->Mask1->setVisible(true);
        }
      }
    }
    return true;
  }

  //走棋
  if (!this->mOnMoving and mSelected != nullptr and
      event->type() == QEvent::MouseButtonPress and
      (watched == ui->ChessBoard or watched == ui->Mask2)) {
    //获取棋盘左上角的坐标
    QPoint global = ui->ChessBoard->mapToGlobal(QPoint(0, 0));
    //加上10等于第一个黑车左上角的位置,用该位置来获取要走到哪里去
    global.setX(global.x() + 10);
    global.setY(global.y() + 10);
    //获取要去的位置相对第一个黑车坐标
    QPoint relativePos{QCursor::pos() - global};
    //走棋时要改动指针数组和棋盘
    uint16_t nowRow = (mSelected->y() - ui->ChessBoard->y() - 10) / 80,
             nowCol = (mSelected->x() - ui->ChessBoard->x() - 10) / 80,
             destRow = relativePos.y() / 80, destCol = relativePos.x() / 80;
    if (this->mIsFliped) {
      nowRow = 9 - nowRow;
      nowCol = 8 - nowCol;
      destRow = 9 - destRow;
      destCol = 8 - destCol;
    }
    PikaChess::Move move;
    move.setMove(nowRow * 9 + nowCol, destRow * 9 + destCol);
    if (not this->canMove(move)) {
      event->ignore();
      return true;
    }
    // 可以走，先把棋子锁上
    this->setMoving(true);
    // 走子后不能再改变边
    ui->PlayerSide->setDisabled(true);
    this->playerMakeMove({nowRow, nowCol, destRow, destCol});
    // 计算精确的相对黑车的坐标
    QPoint destPos{relativePos.x() / 80 * 80, relativePos.y() / 80 * 80};
    // 算出相对棋盘第一个黑车位置的相对坐标后
    // 加上第一个黑车相对窗口左上角的坐标就可以得到要去的位置了
    destPos.setX(ui->ChessBoard->x() + 10 + destPos.x());
    destPos.setY(ui->ChessBoard->y() + 10 + destPos.y());
    // 隐藏mask
    ui->Mask1->setVisible(false);
    ui->Mask2->setVisible(false);
    ui->Mask3->setVisible(false);
    // 移动mask
    ui->Mask2->move(mSelected->x(), mSelected->y());
    ui->Mask3->move(mSelected->x(), mSelected->y());
    ui->Mask2->setVisible(true);
    ui->Mask3->setVisible(true);
    // Mask3动画
    mMaskAni->setStartValue(QRect(ui->Mask3->x(), ui->Mask3->y(),
                                  ui->Mask3->width(), ui->Mask3->height()));
    mMaskAni->setEndValue(QRect(destPos.x(), destPos.y(), ui->Mask3->width(),
                                ui->Mask3->height()));
    mMaskAni->start();
    // 做动画
    mChessMoveAni->setTargetObject(mSelected);
    mChessMoveAni->setStartValue(QRect(mSelected->x(), mSelected->y(),
                                       mSelected->width(),
                                       mSelected->height()));
    mChessMoveAni->setEndValue(QRect(destPos.x(), destPos.y(),
                                     mSelected->width(), mSelected->height()));
    mChessMoveAni->start();
    return true;
  }
  return false;
}
// 获取关闭按钮颜色
QColor Dialog::getColorClose() const { return this->mColorClose; }
// 设置关闭按钮颜色
void Dialog::setColorClose(const QColor color) {
  this->mColorClose = color;
  QString qss =
      QString("#CloseButton{border-image:url(:/Images/"
              "close.ico);border-radius:5px;background: rgba(%1, %2, %3, "
              "%4);}#CloseButton:pressed{border-image:url(:/Images/"
              "close_press.ico);}")
          .arg(color.red())
          .arg(color.green())
          .arg(color.blue())
          .arg(color.alpha());
  ui->CloseButton->setStyleSheet(qss);
}
//获得最小化按钮颜色
QColor Dialog::getColorMin() const { return this->mColorMin; }
//设置最小化按钮颜色
void Dialog::setColorMin(const QColor color) {
  this->mColorMin = color;
  QString qss =
      QString(
          "#MinButton{border-image:url(:/Images/"
          "min.ico);border-radius:5px;background: rgba(%1, %2, %3, "
          "%4);}#MinButton:pressed{border-image:url(:/Images/min_press.bmp);}")
          .arg(color.red())
          .arg(color.green())
          .arg(color.blue())
          .arg(color.alpha());
  ui->MinButton->setStyleSheet(qss);
}
//窗口按钮事件
void Dialog::on_CloseButton_clicked() { this->close(); }

void Dialog::on_MinButton_clicked() { this->showMinimized(); }

//选择红黑槽
void Dialog::on_PlayerSide_currentIndexChanged(int index) {
  ui->PlayerSide->setDisabled(true);
  // 玩家执黑方
  if (1 == index) {
    // 如果棋盘没翻转，那么翻转一下
    if (!this->mIsFliped) {
      on_Flip_clicked();
      QEventLoop eventLoop;
      QTimer::singleShot(600, &eventLoop, &QEventLoop::quit);
      eventLoop.exec();
    }
    // 电脑走红子，先行棋
    computerMove();
  }
}

// 电脑难度选择
void Dialog::on_ComputerHard_currentIndexChanged(int index) {
	// 电脑每一步至少搜索多长时间（单位：毫秒）
	this->m_chessEngine->setSearchTime(1000 * (index + 1));
}

// 翻转按钮点击槽函数
void Dialog::on_Flip_clicked() {
  this->setMoving(true);
  // 翻转不需要锁难度按钮
  ui->HardSelectionBox->setDisabled(false);
  for (uint16_t row = 0; row < 10; ++row)
    for (uint16_t col = 0; col < 9; ++col) {
      QLabel *target = mLabelPointers[row][col];
      if (target) {
        QPropertyAnimation *moveAni = new QPropertyAnimation(this, "geometry");
        moveAni->setTargetObject(target);
        moveAni->setEasingCurve(QEasingCurve::InOutCubic);
        moveAni->setDuration(600);
        moveAni->setStartValue(
            QRect(target->x(), target->y(), target->width(), target->height()));
        if (this->mIsFliped) {
          //已经翻转了，现在要翻回去
          moveAni->setEndValue(QRect(30 + 80 * col, 90 + 80 * row,
                                     target->width(), target->height()));
        } else {
          //还没有翻转，现在翻转
          moveAni->setEndValue(QRect(670 - 80 * col, 810 - 80 * row,
                                     target->width(), target->height()));
        }
        moveAni->start(QPropertyAnimation::DeleteWhenStopped);
      }
    }
  QPropertyAnimation *moveAni1 = new QPropertyAnimation(this, "geometry");
  QPropertyAnimation *moveAni2 = new QPropertyAnimation(this, "geometry");
  QPropertyAnimation *moveAni3 = new QPropertyAnimation(this, "geometry");
  moveAni1->setTargetObject(ui->Mask1);
  moveAni1->setEasingCurve(QEasingCurve::InOutCubic);
  moveAni1->setDuration(600);
  moveAni1->setStartValue(QRect(ui->Mask1->x(), ui->Mask1->y(),
                                ui->Mask1->width(), ui->Mask1->height()));
  moveAni2->setTargetObject(ui->Mask2);
  moveAni2->setEasingCurve(QEasingCurve::InOutCubic);
  moveAni2->setDuration(600);
  moveAni2->setStartValue(QRect(ui->Mask2->x(), ui->Mask2->y(),
                                ui->Mask2->width(), ui->Mask2->height()));
  moveAni3->setTargetObject(ui->Mask3);
  moveAni3->setEasingCurve(QEasingCurve::InOutCubic);
  moveAni3->setDuration(600);
  moveAni3->setStartValue(QRect(ui->Mask3->x(), ui->Mask3->y(),
                                ui->Mask3->width(), ui->Mask3->height()));
  if (this->mIsFliped) {
    //已经翻转了，现在要翻回去
    moveAni1->setEndValue(QRect(700 - ui->Mask1->x(), 900 - ui->Mask1->y(),
                                ui->Mask1->width(), ui->Mask1->height()));
    moveAni2->setEndValue(QRect(700 - ui->Mask2->x(), 900 - ui->Mask2->y(),
                                ui->Mask2->width(), ui->Mask2->height()));
    moveAni3->setEndValue(QRect(700 - ui->Mask3->x(), 900 - ui->Mask3->y(),
                                ui->Mask3->width(), ui->Mask3->height()));
  } else {
    //还没有翻转，现在翻转
    moveAni1->setEndValue(QRect(700 - ui->Mask1->x(), 900 - ui->Mask1->y(),
                                ui->Mask1->width(), ui->Mask1->height()));
    moveAni2->setEndValue(QRect(700 - ui->Mask2->x(), 900 - ui->Mask2->y(),
                                ui->Mask2->width(), ui->Mask2->height()));
    moveAni3->setEndValue(QRect(700 - ui->Mask3->x(), 900 - ui->Mask3->y(),
                                ui->Mask3->width(), ui->Mask3->height()));
  }
  connect(moveAni3, &QPropertyAnimation::finished,
          [&] { this->setMoving(false); });
  moveAni1->start(QPropertyAnimation::DeleteWhenStopped);
  moveAni2->start(QPropertyAnimation::DeleteWhenStopped);
  moveAni3->start(QPropertyAnimation::DeleteWhenStopped);
  this->mIsFliped = !this->mIsFliped;
}

// 重置棋盘槽函数
void Dialog::on_Reset_clicked() {
  this->setMoving(true);
  // 重置不需要锁难度
  ui->HardSelectionBox->setDisabled(false);
  // 重置局面信息
  this->m_chessEngine->reset();
  // 重置电脑胜利标志
  this->mComputerWin = false;
  // 重启边选择器
  ui->PlayerSide->setCurrentIndex(0);
  ui->PlayerSide->setDisabled(false);
  // 重置云开局库标志
  ui->ComputerScore->setText("云库出步");
  ui->ComputerScore->setStyleSheet("font:40px;color:green;");
  ui->ComputerScoreBox->setTitle("中国象棋云库");
  // 重置isFliped标志
  this->mIsFliped = false;
  // 重置选中标志
  this->mSelected = nullptr;
  // 设置mask为不可见
  ui->Mask1->setVisible(false);
  ui->Mask2->setVisible(false);
  ui->Mask3->setVisible(false);
  // 重置指针数组
  mLabelPointers = {
      {ui->BlackChe1, ui->BlackMa1, ui->BlackXiang1, ui->BlackShi1,
       ui->BlackJiang, ui->BlackShi2, ui->BlackXiang2, ui->BlackMa2,
       ui->BlackChe2},
      {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
       nullptr},
      {nullptr, ui->BlackPao1, nullptr, nullptr, nullptr, nullptr, nullptr,
       ui->BlackPao2, nullptr},
      {ui->BlackBing1, nullptr, ui->BlackBing2, nullptr, ui->BlackBing3,
       nullptr, ui->BlackBing4, nullptr, ui->BlackBing5},
      {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
       nullptr},
      {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
       nullptr},
      {ui->RedBing1, nullptr, ui->RedBing2, nullptr, ui->RedBing3, nullptr,
       ui->RedBing4, nullptr, ui->RedBing5},
      {nullptr, ui->RedPao1, nullptr, nullptr, nullptr, nullptr, nullptr,
       ui->RedPao2, nullptr},
      {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
       nullptr},
      {ui->RedChe1, ui->RedMa1, ui->RedXiang1, ui->RedShi1, ui->RedJiang,
       ui->RedShi2, ui->RedXiang2, ui->RedMa2, ui->RedChe2}};
  QVector<QPropertyAnimation *> tempVector;
  for (uint16_t row = 0; row < 10; ++row) {
    for (uint16_t col = 0; col < 9; ++col) {
      QLabel *target = mLabelPointers[row][col];
      if (target) {
        target->setVisible(true);
        QPropertyAnimation *moveAni = new QPropertyAnimation(this, "geometry");
        moveAni->setTargetObject(target);
        moveAni->setEasingCurve(QEasingCurve::InOutCubic);
        moveAni->setDuration(600);
        moveAni->setStartValue(
            QRect(target->x(), target->y(), target->width(), target->height()));
        moveAni->setEndValue(QRect(30 + 80 * col, 90 + 80 * row,
                                   target->width(), target->height()));
        moveAni->start(QPropertyAnimation::DeleteWhenStopped);
        tempVector.emplaceBack(moveAni);
      }
    }
  }
  connect(tempVector.last(), &QPropertyAnimation::finished,
          [&] { this->setMoving(false); });
}

void Dialog::makeMove(Step step) {
  auto [nowRow, nowCol, destRow, destCol] = step;
  //隐藏所有mask
  ui->Mask1->setVisible(false);
  ui->Mask2->setVisible(false);
  ui->Mask3->setVisible(false);
  mLabelPointers[nowRow][nowCol]->raise();
  mTarget = mLabelPointers[destRow][destCol];
  mSelected = mLabelPointers[nowRow][nowCol];
  //移动mask
  ui->Mask2->move(mSelected->x(), mSelected->y());
  ui->Mask3->move(mSelected->x(), mSelected->y());
  ui->Mask2->setVisible(true);
  ui->Mask3->setVisible(true);
  // Mask3动画
  mMaskAni->setStartValue(QRect(ui->Mask3->x(), ui->Mask3->y(),
                                ui->Mask3->width(), ui->Mask3->height()));
  //做动画
  mComputerMoveAni->setTargetObject(mSelected);
  mComputerMoveAni->setStartValue(QRect(
      mSelected->x(), mSelected->y(), mSelected->width(), mSelected->height()));
  if (this->mIsFliped) {
    mComputerMoveAni->setEndValue(
        QRect(ui->ChessBoard->x() + 10 + (8 - destCol) * 80,
              ui->ChessBoard->y() + 10 + (9 - destRow) * 80, mSelected->width(),
              mSelected->height()));
    mMaskAni->setEndValue(QRect(ui->ChessBoard->x() + 10 + (8 - destCol) * 80,
                                ui->ChessBoard->y() + 10 + (9 - destRow) * 80,
                                ui->Mask3->width(), ui->Mask3->height()));
  } else {
    mComputerMoveAni->setEndValue(QRect(ui->ChessBoard->x() + 10 + destCol * 80,
                                        ui->ChessBoard->y() + 10 + destRow * 80,
                                        mSelected->width(),
                                        mSelected->height()));
    mMaskAni->setEndValue(QRect(ui->ChessBoard->x() + 10 + destCol * 80,
                                ui->ChessBoard->y() + 10 + destRow * 80,
                                ui->Mask3->width(), ui->Mask3->height()));
  }
  mMaskAni->start();
  mComputerMoveAni->start();
  mLabelPointers[destRow][destCol] = mLabelPointers[nowRow][nowCol];
  mLabelPointers[nowRow][nowCol] = nullptr;
}

void Dialog::computerMove() {
  using namespace PikaChess;
  QFuture future{QtConcurrent::run([&] {
    // 先搜索云开局库
    const auto &[bookOK, step] = this->searchBook();
    if (bookOK == "云库出步") {
      ui->ComputerScoreBox->setTitle("中国象棋云库");
      ui->ComputerScore->setStyleSheet("font:40px;color:green;");
      ui->ComputerScore->setText(bookOK);
      // 先尝试走一步
      const auto &[fromRow, fromCol, toRow, toCol] = step;
      Move move;
      move.setMove(fromRow * 9 + fromCol, toRow * 9 + toCol);
      this->m_chessEngine->makeMove(move);
      // 如果走云端走法会产生重复局面，只有对方长打才采纳云端走法
      auto repeatScore { this->m_chessEngine->getRepeatScore() };
      if (not repeatScore.has_value() or repeatScore.value() == BAN_SCORE_LOSS) {
        emit threadOK(step);
        return;
      }
      // 该步会导致自己长打，不能走，撤回
      this->m_chessEngine->unMakeMove();
    }
    // 云开局库无对应走法或者开局库走法导致我方长打，由引擎出步
    this->m_chessEngine->search();
    qint16 score { this->m_chessEngine->bestScore() };
    // 显示MetaInfo
    ui->ComputerScoreBox->setTitle("电脑局面分");
    // 根据情况设置字体颜色
    if (score > 0) {
      ui->ComputerScore->setStyleSheet("font:40px;color:green;");
    } else if (score < 0) {
      ui->ComputerScore->setStyleSheet("font:40px;color:red;");
    } else {
      ui->ComputerScore->setStyleSheet("font:40px;");
    }
    // 根据分数写提示
    if (score == MATE_SCORE - 1) {
      // 提示电脑胜利
      ui->ComputerScore->setText("电脑获胜");
      // 设置电脑胜利标志
      this->mComputerWin = true;
    } else if (score == LOSS_SCORE) {
      // 提示玩家胜利
      ui->ComputerScore->setText("玩家获胜");
      // 玩家获胜，电脑无法走棋，直接解锁按钮并返回
      // 注意此时isMoving保持为true，这样可以使得棋子不响应玩家的操作
      this->setButtonDisabled(false);
      return;
    } else if (score > BAN_SCORE_MATE) {
      // 如果电脑快赢了
      ui->ComputerScore->setText(QString::number((MATE_SCORE - score - 1) / 2) + "步获胜");
    } else if (score < BAN_SCORE_LOSS) {
      // 如果电脑快输了
      ui->ComputerScore->setText(QString::number((score - LOSS_SCORE) / 2) + "步落败");
    } else if ((score > BAN_SCORE_LOSS and score < LOST_SCORE) or
               (score > WIN_SCORE and score < BAN_SCORE_MATE)) {
      // 如果产生了长打局面
      // 显示MetaInfo
      ui->ComputerScoreBox->setTitle("局面状态");
      ui->ComputerScore->setText("长打局面");
    } else {
      ui->ComputerScore->setText("[" + QString::number(this->m_chessEngine->currentDepth())
                                 + "层]" + QString::number(score));
    }
    Move bestMove { this->m_chessEngine->bestMove() };
    emit threadOK(std::make_tuple(bestMove.from() / 9, bestMove.from() % 9,
                                  bestMove.to() / 9, bestMove.to() % 9));
  })};
}

inline bool Dialog::isChess(const QObject *object) {
  return object == ui->RedChe1 or object == ui->RedChe2 or
         object == ui->RedMa1 or object == ui->RedMa2 or
         object == ui->RedPao1 or object == ui->RedPao2 or
         object == ui->RedBing1 or object == ui->RedBing2 or
         object == ui->RedBing3 or object == ui->RedBing4 or
         object == ui->RedBing5 or object == ui->RedXiang1 or
         object == ui->RedXiang2 or object == ui->RedShi1 or
         object == ui->RedShi2 or object == ui->RedJiang or
         object == ui->BlackJiang or object == ui->BlackShi1 or
         object == ui->BlackShi2 or object == ui->BlackXiang1 or
         object == ui->BlackXiang2 or object == ui->BlackBing1 or
         object == ui->BlackBing2 or object == ui->BlackBing3 or
         object == ui->BlackBing4 or object == ui->BlackBing5 or
         object == ui->BlackPao1 or object == ui->BlackPao2 or
         object == ui->BlackMa1 or object == ui->BlackMa2 or
         object == ui->BlackChe1 or object == ui->BlackChe2;
}

inline bool Dialog::canMove(PikaChess::Move move) {
  bool ret { this->m_chessEngine->makeMove(move) };
  if (ret) this->m_chessEngine->unMakeMove();
  return ret;
}

inline void Dialog::playerMakeMove(const Step &step) {
  auto &[nowRow, nowCol, destRow, destCol] = step;
  PikaChess::Move move;
  move.setMove(nowRow * 9 + nowCol, destRow * 9 + destCol);
  this->m_chessEngine->makeMove(move);
  // 标签也要跟着走
  this->mLabelPointers[destRow][destCol] = this->mLabelPointers[nowRow][nowCol];
  this->mLabelPointers[nowRow][nowCol] = nullptr;
}

inline void Dialog::setButtonDisabled(const bool disabled) {
  if (disabled) {
    // 如果GUI正在播放动画或电脑正在思考，界面三个按钮禁止点击
    ui->HardSelectionBox->setDisabled(true);
    ui->Reset->setDisabled(true);
    ui->Flip->setDisabled(true);
  } else {
    // 否则恢复界面三个按钮的点击
    ui->HardSelectionBox->setDisabled(false);
    ui->Reset->setDisabled(false);
    ui->Flip->setDisabled(false);
  }
}

inline void Dialog::setMoving(const bool isMoving) {
  this->mOnMoving = isMoving;
  this->setButtonDisabled(isMoving);
}

// 查找云开局库
inline std::tuple<QString, Step> Dialog::searchBook() {
  QNetworkAccessManager cloudBook;
  cloudBook.setTransferTimeout(500);
  // 搜索开局库
  QNetworkReply *cloudReply = cloudBook.get(
      QNetworkRequest{"http://www.chessdb.cn/chessdb.php?action=queryall&board=" +
                      this->m_chessEngine->fen()}
  );
  // 等待请求完成
  QEventLoop event;
  connect(cloudReply, &QNetworkReply::finished, &event, &QEventLoop::quit);
  event.exec();
  QList<QByteArray> cloudResult{cloudReply->readAll().split(',').at(0).split(':')};
  //未联网或获取失败
  if (cloudResult.at(0).isEmpty()) {
    return {"象棋引擎", {0, 0, 0, 0}};
  }
  //分割取走法
  QString cloudTag = cloudResult.at(0);
  //云库无对应招法
  if (cloudTag != "move") {
    return {"象棋引擎", {0, 0, 0, 0}};
  } else {
    QString cloudMove = cloudResult.at(1);
    //走棋
    uint16_t nowRow = 9 - (cloudMove.at(1).unicode() - '0');
    uint16_t nowCol = cloudMove.at(0).unicode() - 'a';
    uint16_t destRow = 9 - (cloudMove.at(3).unicode() - '0');
    uint16_t destCol = cloudMove.at(2).unicode() - 'a';
    return {"云库出步", {nowRow, nowCol, destRow, destCol}};
  }
}
