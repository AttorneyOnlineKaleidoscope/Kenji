#include "serverpublisher.h"

#include "config_manager.h"
#include "qnamespace.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>

const int HTTP_OK = 200;
const int WS_REVERSE_PROXY = 80;
const int TIMEOUT = 1000 * 60 * 5;

ServerPublisher::ServerPublisher(int port, const QBindable<int> &playerCount, QObject *parent)
    : QObject(parent)
    , m_manager{new QNetworkAccessManager(this)}
    , timeout_timer(new QTimer(this))
    , m_player_count(playerCount)
    , m_port{port}
{
  connect(m_manager, &QNetworkAccessManager::finished, this, &ServerPublisher::finished);
  connect(timeout_timer, &QTimer::timeout, this, &ServerPublisher::publishServer);

  timeout_timer->setTimerType(Qt::PreciseTimer);
  timeout_timer->setInterval(TIMEOUT);
  timeout_timer->start();
  publishServer();
}

void ServerPublisher::publishServer()
{
  if (!ConfigManager::publishServerEnabled())
  {
    return;
  }

  QUrl serverlist(ConfigManager::serverlistURL());
  if (serverlist.isValid())
  {
    QNetworkRequest request(serverlist);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject serverinfo;
    if (!ConfigManager::serverDomainName().trimmed().isEmpty())
    {
      serverinfo["ip"] = ConfigManager::serverDomainName();
    }
    if (ConfigManager::securePort() != -1)
    {
      serverinfo["wss_port"] = ConfigManager::securePort();
    }
    serverinfo["port"] = 27106;
    serverinfo["ws_port"] = ConfigManager::advertiseWSProxy() ? WS_REVERSE_PROXY : m_port;
    serverinfo["players"] = m_player_count.value();
    serverinfo["name"] = ConfigManager::serverName();
    serverinfo["description"] = ConfigManager::serverDescription();

    m_manager->post(request, QJsonDocument(serverinfo).toJson());
  }
  else
  {
    qWarning() << "Failed to advertise server. Serverlist URL is not valid. URL:" << serverlist.toString();
  }
}

void ServerPublisher::finished(QNetworkReply *reply)
{
  reply->deleteLater();
  if (reply->error())
  {
    qWarning() << "Failed to advertise server. Error:" << reply->errorString() << "URL:" << reply->url().toString();
    return;
  }

  QByteArray data = reply->isReadable() ? reply->readAll() : QByteArray();
  const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  if (status != HTTP_OK)
  {
    QJsonParseError error;
    QJsonDocument document = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError || !document.isObject())
    {
      qWarning() << "Received malformed response from master server. Error:" << error.errorString();
      return;
    }

    QJsonObject body = document.object();
    if (body.contains("errors"))
    {
      qWarning() << "Failed to advertise to the master server due to the following errors:";
      const QJsonArray errors = body["errors"].toArray();
      for (const auto &ref : errors)
      {
        QJsonObject error = ref.toObject();
        qWarning().noquote() << "Error:" << error["type"].toString() << ". Message:" << error["message"].toString();
      }
      return;
    }
  }
  qInfo() << "Sucessfully advertised server to master server.";
}
