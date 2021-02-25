#include <QtGlobal>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QApplication>
#else
#include <QtGui/QApplication>
#endif

#include "gksserver.h"

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);
  GKSServer server;
  return app.exec();
}
