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
  // ִ�����еĴ��ڳ�ʼ������
  void initDialog();
  // ִ�����е������йصĳ�ʼ������
  void initChess();
  // �������
  void playerMakeMove(const Step &step);
  // ��������
  void computerMove();
  // �ж�һ�������Ƿ�����������
  inline bool isChess(const QObject *object);
  // �ж��Ƿ������
  inline bool canMove(const Move move);
  // ��һ��10x9��ʾ�����߷�ת��Ϊ256��ʾ��
  inline Move mapTo256(const Step &step);
  // ��һ��256��ʾ�����߷�ת��Ϊ10x9��ʾ��
  inline Step mapToStep(const Move move);
  // ���ð�ť״̬���ɵ��/���ɵ��
  inline void setButtonDisabled(const bool disable);
  // ��������״̬
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
  // ������صĳ�Ա
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

  //��Щ�����ӳ����������õĶ���ָ��Ͷ���
  QLabel *mSelected{nullptr};
  QLabel *mTarget{nullptr};
  QPropertyAnimation *mChessMoveAni{
      new QPropertyAnimation(mSelected, "geometry")};
  QPropertyAnimation *mChessEatAni{
      new QPropertyAnimation(mSelected, "geometry")};
  QPropertyAnimation *mComputerMoveAni{
      new QPropertyAnimation(mSelected, "geometry")};
  QPropertyAnimation *mMaskAni{new QPropertyAnimation(this, "geometry")};

  // ����ʱ�����û��Ҷ�
  bool mOnMoving{false};

  // �����Ƿ�ʤ��
  bool mComputerWin{false};

  //������ṩ���������ӵĶ���ָ���ά����
  QVector<QVector<QLabel *>> mLabelPointers;
  bool mIsFliped{false};
};
