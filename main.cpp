#include "dialog.h"

#include <QApplication>

int main(int argc, char *argv[])
{
  QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::Floor);
  QApplication a(argc, argv);
  Dialog w;
  w.show();
  return a.exec();
}
