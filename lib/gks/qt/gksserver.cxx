#include <stdio.h>
#include <sstream>

#include <iostream>

#include <QApplication>
#include <QtNetwork>
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QScreen>
#else
#include <QDesktopWidget>
#endif

#include "gksserver.h"


const int GKSConnection::window_shift = 30;
unsigned int GKSConnection::index = 0;
const unsigned int GKSServer::port = 8410;


#if QT_VERSION < QT_VERSION_CHECK(5, 3, 0)
inline QRect operator-(const QRect &lhs, const QMargins &rhs)
{
  return QRect(QPoint(lhs.left() + rhs.left(), lhs.top() + rhs.top()),
               QPoint(lhs.right() - rhs.right(), lhs.bottom() - rhs.bottom()));
}

inline QRect &operator-=(QRect &rect, const QMargins &margins)
{
  rect = rect - margins;
  return rect;
}
#endif


GKSConnection::GKSConnection(QTcpSocket *socket) : socket(socket), widget(NULL), dl(NULL), dl_size(0)
{
  ++index;
  connect(socket, SIGNAL(readyRead()), this, SLOT(readClient()));
  connect(socket, SIGNAL(disconnected()), this, SLOT(disconnectedSocket()));
  // send information about workstation back to client
  struct
  {
    int nbytes;
    double mwidth;
    double mheight;
    int width;
    int height;
    char name[6];
  } workstation_information = {sizeof(workstation_information), 0, 0, 0, 0, "gksqt"};
  GKSWidget::inqdspsize(&workstation_information.mwidth, &workstation_information.mheight,
                        &workstation_information.width, &workstation_information.height);
  socket->write(reinterpret_cast<const char *>(&workstation_information), workstation_information.nbytes);
}

GKSConnection::~GKSConnection()
{
  socket->close();
  delete socket;
  if (widget != NULL)
    {
      widget->close();
    }
}

void GKSConnection::readClient()
{
  while (socket->bytesAvailable() > 0)
    {
      if (dl_size == 0)
        {
          if (socket->bytesAvailable() < (long)sizeof(int)) return;
          socket->read((char *)&dl_size, sizeof(unsigned int));
        }
      /* If `dl_size` is still `0` this is a close request
       * -> send a close request signal which is processed in the GKSServer instance */
      if (dl_size == 0 && widget == NULL)
        {
          emit(requestApplicationShutdown(*this));
        }
      if (socket->bytesAvailable() < dl_size) return;
      dl = new char[dl_size + sizeof(int)];
      socket->read(dl, dl_size);
      // The data buffer must be terminated by a zero integer -> `sizeof(int)` zero bytes
      memset(dl + dl_size, 0, sizeof(int));
      if (widget == NULL)
        {
          newWidget();
        }
      emit(data(dl));
      dl_size = 0;
    }
}

void GKSConnection::destroyedWidget()
{
  widget = NULL;
  emit(close(*this));
}

void GKSConnection::disconnectedSocket()
{
  if (widget != NULL)
    {
      widget->close();
      widget = NULL;
    }
}

void GKSConnection::newWidget()
{
  std::stringstream window_title_stream;
  window_title_stream << "GKS QtTerm";
  if (index > 1)
    {
      window_title_stream << " (" << index << ")";
    }
  widget = new GKSWidget();
  widget->setWindowTitle(window_title_stream.str().c_str());
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
  QRect screen_geometry = QGuiApplication::primaryScreen()->availableGeometry();
#else
  QDesktopWidget *desktop = QApplication::desktop();
  QRect screen_geometry = desktop->screenGeometry(desktop->primaryScreen());
#endif
  QPoint screen_center = screen_geometry.center();
  QRect valid_position_area = screen_geometry - QMargins(0, 0, widget->width(), widget->height());
  if (GKSWidget::frame_decoration_size().isValid())
    {
      valid_position_area -=
          QMargins(0, 0, GKSWidget::frame_decoration_size().width(), GKSWidget::frame_decoration_size().height());
    }
  QPoint widget_position =
      QPoint((screen_center.x() - widget->width() / 2 - valid_position_area.left() + index * window_shift) %
                     valid_position_area.width() +
                 valid_position_area.left(),
             (screen_center.y() - widget->height() / 2 - valid_position_area.top() + index * window_shift) %
                     valid_position_area.height() +
                 valid_position_area.top());
  widget->move(widget_position);
  connect(this, SIGNAL(data(char *)), widget, SLOT(interpret(char *)));

  widget->setAttribute(Qt::WA_QuitOnClose, false);
  widget->setAttribute(Qt::WA_DeleteOnClose);
  connect(widget, SIGNAL(destroyed(QObject *)), this, SLOT(destroyedWidget()));
}

GKSServer::GKSServer(QObject *parent) : QTcpServer(parent)
{
  QString gks_display = QProcessEnvironment::systemEnvironment().value("GKS_DISPLAY");
  QHostAddress host_address = QHostAddress::LocalHost;
  if (!gks_display.isEmpty())
    {
      host_address = QHostAddress(gks_display);
    }
  connect(this, SIGNAL(newConnection()), this, SLOT(connectSocket()));
  if (!listen(host_address, port))
    {
      qWarning("GKSserver: Failed to listen to port %d", port);
      exit(1);
    }
}

GKSServer::~GKSServer()
{
  for (std::list<const GKSConnection *>::iterator it = connections.begin(); it != connections.end(); ++it)
    {
      delete *it;
    }
}

void GKSServer::connectSocket()
{
  QTcpSocket *socket = this->nextPendingConnection();
  GKSConnection *connection = new GKSConnection(socket);
  connect(connection, SIGNAL(close(GKSConnection &)), this, SLOT(closeConnection(GKSConnection &)));
  connect(connection, SIGNAL(requestApplicationShutdown(GKSConnection &)), this,
          SLOT(closeConnection(GKSConnection &)));
  connections.push_back(connection);
}

void GKSServer::closeConnection(GKSConnection &connection)
{
  connections.remove(&connection);
  connection.deleteLater();
  if (connections.empty())
    {
      QApplication::quit();
    }
}
