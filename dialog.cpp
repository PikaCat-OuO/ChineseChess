#include "dialog.h"
#include "pikachess.hpp"
#include "ui_dialog.h"

Dialog::Dialog(QWidget *parent) : QDialog(parent), ui(new Ui::Dialog) {
  ui->setupUi(this);
  this->initDialog();
  this->initChess();
}

void Dialog::initDialog() {
  //��װ��������ť�¼����
  ui->CloseButton->installEventFilter(this);
  ui->MinButton->installEventFilter(this);
  //�����ޱ߿򴰿�
  setWindowFlag(Qt::FramelessWindowHint);
  //���ô���͸��
  setAttribute(Qt::WA_TranslucentBackground);
  //���ô�������Ļ��������ʾ
  auto desk = QApplication::primaryScreen()->geometry();
  move((desk.width() - this->width()) / 2,
       (desk.height() - this->height()) / 2);
  //������������
  QPropertyAnimation *ani = new QPropertyAnimation(this, "windowOpacity");
  ani->setDuration(600);
  ani->setStartValue(0);
  ani->setEndValue(0.94);
  ani->setEasingCurve(QEasingCurve::InOutSine);
  ani->start(QPropertyAnimation::DeleteWhenStopped);
  //���ô�����Ӱ
  QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect;
  shadow->setOffset(0, 0);
  shadow->setColor(QColor(0, 0, 0, 50));
  shadow->setBlurRadius(10);
  ui->frame->setGraphicsEffect(shadow);
}

void Dialog::initChess() {
  //��װ�����¼����
  ui->ChessBoard->installEventFilter(this);
  //��װmask2�¼����
  ui->Mask2->installEventFilter(this);
  //��װ���е������¼����
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
  //�����ӻ�mask
  ui->Mask1->setVisible(false);
  ui->Mask2->setVisible(false);
  ui->Mask3->setVisible(false);
  //��ʼ��ָ������
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
  //�������嶯���Ĳ��ֲ���
  mChessMoveAni->setEasingCurve(QEasingCurve::InOutCubic);
  mChessEatAni->setEasingCurve(QEasingCurve::InOutCubic);
  mComputerMoveAni->setEasingCurve(QEasingCurve::InOutCubic);
  mMaskAni->setTargetObject(ui->Mask3);
  mMaskAni->setEasingCurve(QEasingCurve::InOutCubic);
  //ע������
  qRegisterMetaType<uint16_t>("uint16_t");
  //���嶯������Ҫ����Ѿ�ѡ�е��Ӳ��õ�������
  connect(mChessMoveAni, &QPropertyAnimation::finished, [&] {
    mSelected = nullptr;
    computerMove();
  });
  //���ӳ���Ҫ�����Ե���������Ϊ���ɼ�
  connect(mChessEatAni, &QPropertyAnimation::finished, [&] {
    mSelected = nullptr;
    mTarget->setVisible(false);
    mTarget = nullptr;
    computerMove();
  });
  //���ӵ��Լ�������źź����Ӳۺ���
  connect(this, &Dialog::threadOK, this, &Dialog::makeMove);
  //���ӵ������Ӻ�Ĵ���ۺ���
  connect(mComputerMoveAni, &QPropertyAnimation::finished, [&] {
    //����ǳ����߷���Ҫ���Է�����������Ϊ���ɼ�
    if (mTarget) {
      mTarget->setVisible(false);
    }
    mSelected = nullptr;
    mTarget = nullptr;
    // �����ˣ��������Ƿ��Ѿ�ʤ��
    if (this->mComputerWin) {
      // ����Ӯ�ˣ�ע���ʱisMoving����Ϊtrue����������ʹ�����Ӳ���Ӧ��ҵĲ���
      this->setButtonDisabled(false);
    } else {
      // ����������ظ���ť��isMoving��״̬
      this->setMoving(false);
    }
  });
  // ��ʼ������
  init();
}

Dialog::~Dialog() { delete ui; }
//��д�ر��¼�ʵ�ִ��ڹرն���
void Dialog::closeEvent(QCloseEvent *event) {
  if (this->mCloseCheck == false) {
    this->mCloseCheck = true;
    event->ignore();
    //���ڹرն���
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

//��д����ƶ��¼���ʵ�ֵ���������⴦�ƶ����ڵĹ���
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
    //������ס��ť���뿪��ť���򣬰�ť�ظ�ԭ��
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

//ʵ�ְ�Esc���رմ���ʱҲ�ܲ��Ŷ���
void Dialog::keyPressEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_Escape) {
    this->close();
  }
}

//��������ť����
bool Dialog::eventFilter(QObject *watched, QEvent *event) {
  //�رհ�ť��ȼЧ��
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
  //��С����ť��ȼЧ��
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

  //�����ӺͲ�����������
  if (!this->mOnMoving and event->type() == QEvent::MouseButtonPress and
      this->isChess(watched)) {
    mTarget = dynamic_cast<QLabel *>(watched);
    if (mSelected != nullptr and mSelected->text() != mTarget->text()) {
      //����ʱҪ�Ķ�ָ�����������
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
      if (not this->canMove(mapTo256({nowRow, nowCol, destRow, destCol}))) {
        event->ignore();
        return true;
      }
      // �����ߣ��Ȱ���������
      this->setMoving(true);
      // ����
      this->playerMakeMove({nowRow, nowCol, destRow, destCol});
      //����mask
      ui->Mask1->setVisible(false);
      ui->Mask2->setVisible(false);
      ui->Mask3->setVisible(false);
      //�ƶ�mask
      ui->Mask2->move(mSelected->x(), mSelected->y());
      ui->Mask3->move(mSelected->x(), mSelected->y());
      ui->Mask2->setVisible(true);
      ui->Mask3->setVisible(true);
      // Mask3����
      mMaskAni->setStartValue(QRect(ui->Mask3->x(), ui->Mask3->y(),
                                    ui->Mask3->width(), ui->Mask3->height()));
      mMaskAni->setEndValue(QRect(mTarget->x(), mTarget->y(),
                                  ui->Mask3->width(), ui->Mask3->height()));
      mMaskAni->start();
      //����������
      mChessEatAni->setTargetObject(mSelected);
      mChessEatAni->setStartValue(QRect(mSelected->x(), mSelected->y(),
                                        mSelected->width(),
                                        mSelected->height()));
      mChessEatAni->setEndValue(QRect(mTarget->x(), mTarget->y(),
                                      mSelected->width(), mSelected->height()));
      mChessEatAni->start();
    } else {
      //��ѡ��ʱ�����Լ����Ĳ���ѡ
      if (mTarget->text() == (COMPUTER_SIDE ? "r" : "b")) {
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

  //����
  if (!this->mOnMoving and mSelected != nullptr and
      event->type() == QEvent::MouseButtonPress and
      (watched == ui->ChessBoard or watched == ui->Mask2)) {
    //��ȡ�������Ͻǵ�����
    QPoint global = ui->ChessBoard->mapToGlobal(QPoint(0, 0));
    //����10���ڵ�һ���ڳ����Ͻǵ�λ��,�ø�λ������ȡҪ�ߵ�����ȥ
    global.setX(global.x() + 10);
    global.setY(global.y() + 10);
    //��ȡҪȥ��λ����Ե�һ���ڳ�����
    QPoint relativePos{QCursor::pos() - global};
    //����ʱҪ�Ķ�ָ�����������
    uint16_t nowRow = (mSelected->y() - ui->ChessBoard->y() - 10) / 80,
             nowCol = (mSelected->x() - ui->ChessBoard->x() - 10) / 80,
             destRow = relativePos.y() / 80, destCol = relativePos.x() / 80;
    if (this->mIsFliped) {
      nowRow = 9 - nowRow;
      nowCol = 8 - nowCol;
      destRow = 9 - destRow;
      destCol = 8 - destCol;
    }
    if (not this->canMove(mapTo256({nowRow, nowCol, destRow, destCol}))) {
      event->ignore();
      return true;
    }
    // �����ߣ��Ȱ���������
    this->setMoving(true);
    // ���Ӻ����ٸı��
    ui->PlayerSide->setDisabled(true);
    this->playerMakeMove({nowRow, nowCol, destRow, destCol});
    // ���㾫ȷ����Ժڳ�������
    QPoint destPos{relativePos.x() / 80 * 80, relativePos.y() / 80 * 80};
    // ���������̵�һ���ڳ�λ�õ���������
    // ���ϵ�һ���ڳ���Դ������Ͻǵ�����Ϳ��Եõ�Ҫȥ��λ����
    destPos.setX(ui->ChessBoard->x() + 10 + destPos.x());
    destPos.setY(ui->ChessBoard->y() + 10 + destPos.y());
    // ����mask
    ui->Mask1->setVisible(false);
    ui->Mask2->setVisible(false);
    ui->Mask3->setVisible(false);
    // �ƶ�mask
    ui->Mask2->move(mSelected->x(), mSelected->y());
    ui->Mask3->move(mSelected->x(), mSelected->y());
    ui->Mask2->setVisible(true);
    ui->Mask3->setVisible(true);
    // Mask3����
    mMaskAni->setStartValue(QRect(ui->Mask3->x(), ui->Mask3->y(),
                                  ui->Mask3->width(), ui->Mask3->height()));
    mMaskAni->setEndValue(QRect(destPos.x(), destPos.y(), ui->Mask3->width(),
                                ui->Mask3->height()));
    mMaskAni->start();
    // ������
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
// ��ȡ�رհ�ť��ɫ
QColor Dialog::getColorClose() const { return this->mColorClose; }
// ���ùرհ�ť��ɫ
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
//�����С����ť��ɫ
QColor Dialog::getColorMin() const { return this->mColorMin; }
//������С����ť��ɫ
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
//���ڰ�ť�¼�
void Dialog::on_CloseButton_clicked() { this->close(); }

void Dialog::on_MinButton_clicked() { this->showMinimized(); }

//ѡ���ڲ�
void Dialog::on_PlayerSide_currentIndexChanged(int index) {
  ui->PlayerSide->setDisabled(true);
  COMPUTER_SIDE = index ? RED : BLACK;
  //���Ժ���
  if (COMPUTER_SIDE == RED) {
    //�������û��ת����ô��תһ��
    if (!this->mIsFliped) {
      on_Flip_clicked();
      QEventLoop eventLoop;
      QTimer::singleShot(600, &eventLoop, &QEventLoop::quit);
      eventLoop.exec();
    }
    //�����ߺ��ӣ�������
    computerMove();
  }
}

// �����Ѷ�ѡ��
void Dialog::on_ComputerHard_currentIndexChanged(int index) {
  // ����ÿһ�����������೤ʱ�䣨��λ�����룩
  switch (index) {
  case 0:
    SEARCH_TIME = 1000;
    break;
  case 1:
    SEARCH_TIME = 3000;
    break;
  default:
    SEARCH_TIME = 6000;
  }
}

// ��ת��ť����ۺ���
void Dialog::on_Flip_clicked() {
  this->setMoving(true);
  // ��ת����Ҫ���ѶȰ�ť
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
          //�Ѿ���ת�ˣ�����Ҫ����ȥ
          moveAni->setEndValue(QRect(30 + 80 * col, 90 + 80 * row,
                                     target->width(), target->height()));
        } else {
          //��û�з�ת�����ڷ�ת
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
    //�Ѿ���ת�ˣ�����Ҫ����ȥ
    moveAni1->setEndValue(QRect(700 - ui->Mask1->x(), 900 - ui->Mask1->y(),
                                ui->Mask1->width(), ui->Mask1->height()));
    moveAni2->setEndValue(QRect(700 - ui->Mask2->x(), 900 - ui->Mask2->y(),
                                ui->Mask2->width(), ui->Mask2->height()));
    moveAni3->setEndValue(QRect(700 - ui->Mask3->x(), 900 - ui->Mask3->y(),
                                ui->Mask3->width(), ui->Mask3->height()));
  } else {
    //��û�з�ת�����ڷ�ת
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

// �������̲ۺ���
void Dialog::on_Reset_clicked() {
  this->setMoving(true);
  // ���ò���Ҫ���Ѷ�
  ui->HardSelectionBox->setDisabled(false);
  // ���þ�����Ϣ
  ::init();
  // ���õ���ʤ����־
  this->mComputerWin = false;
  // ������ѡ����
  ui->PlayerSide->setCurrentIndex(0);
  ui->PlayerSide->setDisabled(false);
  // �����ƿ��ֿ��־
  ui->ComputerScore->setText(QString::fromLocal8Bit("�ƿ����"));
  ui->ComputerScore->setStyleSheet("font:40px;color:green;");
  ui->ComputerScoreBox->setTitle(QString::fromLocal8Bit("�й������ƿ�"));
  // ����isFliped��־
  this->mIsFliped = false;
  // ����maskΪ���ɼ�
  ui->Mask1->setVisible(false);
  ui->Mask2->setVisible(false);
  ui->Mask3->setVisible(false);
  // ����ָ������
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
  //��������mask
  ui->Mask1->setVisible(false);
  ui->Mask2->setVisible(false);
  ui->Mask3->setVisible(false);
  mLabelPointers[nowRow][nowCol]->raise();
  mTarget = mLabelPointers[destRow][destCol];
  mSelected = mLabelPointers[nowRow][nowCol];
  //�ƶ�mask
  ui->Mask2->move(mSelected->x(), mSelected->y());
  ui->Mask3->move(mSelected->x(), mSelected->y());
  ui->Mask2->setVisible(true);
  ui->Mask3->setVisible(true);
  // Mask3����
  mMaskAni->setStartValue(QRect(ui->Mask3->x(), ui->Mask3->y(),
                                ui->Mask3->width(), ui->Mask3->height()));
  //������
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
  QFuture future{QtConcurrent::run([&] {
    const auto &[bookOK, step] = POSITION_INFO.searchBook();
    if (bookOK == QString::fromLocal8Bit("�ƿ����")) {
      ui->ComputerScoreBox->setTitle(QString::fromLocal8Bit("�й������ƿ�"));
      ui->ComputerScore->setStyleSheet("font:40px;color:green;");
      ui->ComputerScore->setText(bookOK);
      POSITION_INFO.makeMove(mapTo256(step), COMPUTER_SIDE);
      emit threadOK(step);
    } else {
      Score score = searchMain();
      // ��ʾMetaInfo
      ui->ComputerScoreBox->setTitle(QString::fromLocal8Bit("���Ծ����"));
      // �����������������ɫ
      if (score > 0) {
        ui->ComputerScore->setStyleSheet("font:40px;color:green;");
      } else if (score < 0) {
        ui->ComputerScore->setStyleSheet("font:40px;color:red;");
      } else {
        ui->ComputerScore->setStyleSheet("font:40px;");
      }
      // ���ݷ���д��ʾ
      if (score == MATE_SCORE - 1) {
        // ��ʾ����ʤ��
        ui->ComputerScore->setText(QString::fromLocal8Bit("���Ի�ʤ"));
        // ���õ���ʤ����־
        this->mComputerWin = true;
      } else if (score == LOSS_SCORE) {
        // ��ʾ���ʤ��
        ui->ComputerScore->setText(QString::fromLocal8Bit("��һ�ʤ"));
        // ��һ�ʤ�������޷����壬ֱ�ӽ�����ť������
        // ע���ʱisMoving����Ϊtrue����������ʹ�����Ӳ���Ӧ��ҵĲ���
        this->setButtonDisabled(false);
        return;
      } else if (score > BAN_SCORE_MATE) {
        // ������Կ�Ӯ��
        ui->ComputerScore->setText(
            QString::number((MATE_SCORE - score - 1) / 2) +
            QString::fromLocal8Bit("����ʤ"));
      } else if (score < BAN_SCORE_LOSS) {
        // ������Կ�����
        ui->ComputerScore->setText(QString::number((score - LOSS_SCORE) / 2) +
                                   QString::fromLocal8Bit("�����"));
      } else {
        ui->ComputerScore->setText(
            QString::fromLocal8Bit("[") + QString::number(CURRENT_DEPTH - 1) +
            QString::fromLocal8Bit("��]") + QString::number(score));
      }
      emit threadOK(mapToStep(POSITION_INFO.mBestMove));
    }
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

inline bool Dialog::canMove(const Move move) {
  bool canMove{false};
  // ������ǺϷ��Ĳ����Ͳ�����
  if (POSITION_INFO.isLegalMove(move, getOppSide(COMPUTER_SIDE))) {
    // ������һ�£����Ƿ񱻽���
    canMove = POSITION_INFO.makeMove(move, getOppSide(COMPUTER_SIDE));
    if (canMove) {
      // ������߾ͻ�ԭ�������ߵĻ��Ѿ���makeMove�������滹ԭ�ˣ�û��Ҫ�ٻ�ԭ
      POSITION_INFO.unMakeMove(move, getOppSide(COMPUTER_SIDE));
    }
  }
  return canMove;
}

inline void Dialog::playerMakeMove(const Step &step) {
  auto &[nowRow, nowCol, destRow, destCol] = step;
  POSITION_INFO.makeMove(this->mapTo256(step), getOppSide(COMPUTER_SIDE));
  // ��ǩҲҪ������
  this->mLabelPointers[destRow][destCol] = this->mLabelPointers[nowRow][nowCol];
  this->mLabelPointers[nowRow][nowCol] = nullptr;
}

inline Move Dialog::mapTo256(const Step &step) {
  const auto &[fromX, fromY, toX, toY] = step;
  // ��������λ��
  return toMove(51 + fromX * 16 + fromY, 51 + toX * 16 + toY);
}

inline Step Dialog::mapToStep(const Move move) {
  return {getSrc(move) / 16 - 3, getSrc(move) % 16 - 3, getDest(move) / 16 - 3,
          getDest(move) % 16 - 3};
}

inline void Dialog::setButtonDisabled(const bool disabled) {
  if (disabled) {
    // ���GUI���ڲ��Ŷ������������˼��������������ť��ֹ���
    ui->HardSelectionBox->setDisabled(true);
    ui->Reset->setDisabled(true);
    ui->Flip->setDisabled(true);
  } else {
    // ����ָ�����������ť�ĵ��
    ui->HardSelectionBox->setDisabled(false);
    ui->Reset->setDisabled(false);
    ui->Flip->setDisabled(false);
  }
}

inline void Dialog::setMoving(const bool isMoving) {
  this->mOnMoving = isMoving;
  this->setButtonDisabled(isMoving);
}
