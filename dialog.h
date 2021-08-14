#pragma once

#include <QDialog>
#include <QGraphicsDropShadowEffect>
#include <QKeyEvent>
#include <QLabel>
#include <QtConcurrent/QtConcurrent>

using Position = uint8_t;
using Move = uint16_t;
using Step = std::tuple<Position, Position, Position, Position>;

QT_BEGIN_NAMESPACE
namespace Ui {
class Dialog;
}
QT_END_NAMESPACE

class Dialog : public QDialog {
  Q_OBJECT
  Q_PROPERTY(QColor mColorClose READ getColorClose WRITE setColorClose)
  Q_PROPERTY(QColor mColorMin READ getColorMin WRITE setColorMin)

public:
  Dialog(QWidget *parent = nullptr);
  ~Dialog();
  // 执行所有的窗口初始化动作
  void initDialog();
  // 执行所有的棋盘有关的初始化动作
  void initChess();
  // 玩家走棋
  void playerMakeMove(const Step &step);
  // 电脑走棋
  void computerMove();
  // 判断一个对象是否是棋子类型
  inline bool isChess(const QObject *object);
  // 判断是否可以走
  inline bool canMove(const Move move);
  // 将一个10x9表示法的走法转换为256表示法
  inline Move mapTo256(const Step &step);
  // 将一个256表示法的走法转换为10x9表示法
  inline Step mapToStep(const Move move);
  // 设置按钮状态，可点击/不可点击
  inline void setButtonDisabled(const bool disable);
  // 设置走棋状态
  inline void setMoving(const bool isMoving);

protected:
  void closeEvent(QCloseEvent *);
  void mousePressEvent(QMouseEvent *event);
  void mouseMoveEvent(QMouseEvent *event);
  void mouseReleaseEvent(QMouseEvent *event);
  void keyPressEvent(QKeyEvent *event);
  bool eventFilter(QObject *watched, QEvent *event);

signals:
  void threadOK(Step);

private slots:
  void on_CloseButton_clicked();

  void on_MinButton_clicked();

  void makeMove(Step step);

  void on_PlayerSide_currentIndexChanged(int index);

  void on_Flip_clicked();

  void on_Reset_clicked();

  void on_ComputerHard_currentIndexChanged(int index);

private:
  // 窗口相关的成员
  Ui::Dialog *ui;
  bool mCloseCheck{false};
  bool mOnDialog{false};
  QPoint mMouseStartPoint;
  QPoint mDialogStartPoint;
  QColor getColorClose() const;
  void setColorClose(const QColor color);
  QColor mColorClose{QColor(212, 64, 39, 0)};
  QColor getColorMin() const;
  void setColorMin(const QColor color);
  QColor mColorMin{QColor(38, 169, 218, 0)};

  //这些是走子吃子所必须获得的对象指针和动画
  QLabel *mSelected{nullptr};
  QLabel *mTarget{nullptr};
  QPropertyAnimation *mChessMoveAni{
      new QPropertyAnimation(mSelected, "geometry")};
  QPropertyAnimation *mChessEatAni{
      new QPropertyAnimation(mSelected, "geometry")};
  QPropertyAnimation *mComputerMoveAni{
      new QPropertyAnimation(mSelected, "geometry")};
  QPropertyAnimation *mMaskAni{new QPropertyAnimation(this, "geometry")};

  // 走棋时不给用户乱动
  bool mOnMoving{false};

  // 电脑是否胜利
  bool mComputerWin{false};

  //这个是提供给电脑走子的对象指针二维数组
  QVector<QVector<QLabel *>> mLabelPointers;
  bool mIsFliped{false};
};
