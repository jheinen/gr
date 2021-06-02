#ifndef _GKSSERVER_H_
#define _GKSSERVER_H_

#include <list>
#include <QTcpServer>
#include <QTcpSocket>
#include <qstring.h>

#include "gkswidget.h"


struct SocketFunction
{
  enum Enum
  {
    unknown = 0,
    create_window = 1,
    draw = 2,
    is_alive = 3,
    close_window = 4,
    is_running = 5,
    inq_ws_state = 6
  };
};


class GKSConnection : public QObject
{
  Q_OBJECT

public:
  GKSConnection(QTcpSocket *socket);
  virtual ~GKSConnection();
  void newWidget();

public slots:
  void readClient();
  void destroyedWidget();
  void disconnectedSocket();

signals:
  void data(char *);
  void close(GKSConnection &connection);
  void requestApplicationShutdown(GKSConnection &connection);

private:
  static unsigned int index;
  static const int window_shift;
  QTcpSocket *socket;
  GKSWidget *widget;
  char *dl;
  unsigned int dl_size;
  SocketFunction::Enum socket_function;
};


class GKSServer : public QTcpServer
{
  Q_OBJECT

public:
  GKSServer(QObject *parent = 0);
  virtual ~GKSServer();

public slots:
  void connectSocket();
  void closeConnection(GKSConnection &connection);

private:
  static const unsigned int port;
  std::list<const GKSConnection *> connections;
};

#endif
