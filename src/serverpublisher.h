#pragma once

#include <QBindable>
#include <QObject>

class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

/**
 * @brief Represents the ServerPublisher of the server. Sends current server information to the serverlist.
 */
class ServerPublisher : public QObject
{
  Q_OBJECT

public:
  explicit ServerPublisher(int port, const QBindable<int> &playerCount, QObject *parent = nullptr);
  virtual ~ServerPublisher(){};

public Q_SLOTS:
  void publishServer();

private:
  QNetworkAccessManager *m_manager;
  QTimer *timeout_timer;
  QBindable<int> m_player_count;
  int m_port;

private Q_SLOTS:
  void finished(QNetworkReply *reply);
};
