#include "server.h"

#include "acl_roles_handler.h"
#include "aoclient.h"
#include "area_data.h"
#include "command_extension.h"
#include "config_manager.h"
#include "db_manager.h"
#include "discord.h"
#include "ipidutils.h"
#include "logger/u_logger.h"
#include "music_manager.h"
#include "network/packet/character_packets.h"
#include "network/packet/handshake_packets.h"
#include "network/packet/moderation_packets.h"
#include "network/packet/system_packets.h"
#include "network/packetprotocol.h"
#include "serverpublisher.h"

Server::Server(QObject *parent)
    : QObject(parent)
{
  timer = new QTimer(this);

  m_database = new DBManager;

  acl_roles_handler = new ACLRolesHandler(this);
  acl_roles_handler->loadFile("config/acl_roles.ini");

  command_extension_collection = new CommandExtensionCollection;
  command_extension_collection->setCommandNameWhitelist(AOClient::COMMANDS.keys());
  command_extension_collection->loadFile("config/command_extensions.ini");

  // We create it, even if its not used later on.
  m_discord = new Discord(this);

  logger = new ULogger(this);

  m_player_count.setBinding([this]() -> int { return playerCount(); });

  connect(this, &Server::logConnectionAttempt, logger, &ULogger::logConnectionAttempt);
}

Server::~Server()
{
  m_listener->deleteLater();
  m_discord->deleteLater();
  acl_roles_handler->deleteLater();
  m_database->deleteLater();
}

void Server::start(int port)
{
  QString bind_ip = ConfigManager::bindIP();
  QHostAddress bind_addr;
  if (bind_ip == "all")
  {
    bind_addr = QHostAddress::Any;
  }
  else
  {
    bind_addr = QHostAddress(bind_ip);
  }
  if (bind_addr.protocol() != QAbstractSocket::IPv4Protocol && bind_addr.protocol() != QAbstractSocket::IPv6Protocol && bind_addr != QHostAddress::Any)
  {
    qDebug() << bind_ip << "is an invalid IP address to listen on! Server not starting, check your config.";
  }

  m_listener = new QWebSocketServer("Akashi", QWebSocketServer::NonSecureMode, this);
  if (!m_listener->listen(bind_addr, port))
  {
    qDebug() << "Server error:" << m_listener->errorString();
  }
  else
  {
    connect(m_listener, &QWebSocketServer::newConnection, this, &Server::clientConnected);
    qInfo() << "Server listening on" << m_listener->serverPort();
  }

  // Checks if any Discord webhooks are enabled.
  handleDiscordIntegration();

  // Construct modern advertiser if enabled in config
  server_publisher = new ServerPublisher(m_listener->serverPort(), QBindable<int>(&m_player_count), this);

  // Get characters from config file
  m_characters = ConfigManager::charlist();

  // Get backgrounds from config file
  m_backgrounds = ConfigManager::backgrounds();

  // Build our music manager.

  MusicList l_musiclist = ConfigManager::musiclist();
  music_manager = new MusicManager(this, ConfigManager::cdnList(), l_musiclist, ConfigManager::ordered_songs(), this);

  // Get musiclist from config file
  m_music_list = music_manager->rootMusiclist();

  // Assembles the area list
  m_area_names = ConfigManager::sanitizedAreaNames();
  for (int i = 0; i < m_area_names.length(); i++)
  {
    QString area_name = QString::number(i) + ":" + m_area_names[i];
    AreaData *l_area = new AreaData(this, area_name, i, music_manager);
    m_areas.insert(i, l_area);
    connect(l_area, &AreaData::userJoinedArea, music_manager, &MusicManager::userJoinedArea);
    music_manager->registerArea(i);
  }

  // Loads the command help information. This is not stored inside the server.
  ConfigManager::loadCommandHelp();

  // Get IP bans
  m_ipban_list = ConfigManager::iprangeBans();

  // Rate-Limiter for IC-Chat
  m_message_floodguard_timer = new QTimer(this);
  m_message_floodguard_timer->setSingleShot(true);
  connect(m_message_floodguard_timer, &QTimer::timeout, this, &Server::allowMessage);

  for (int i = ConfigManager::maxPlayers() - 1; i >= 0; i--)
  {
    m_id_stack.push(i);
  }
}

QList<AOClient *> Server::clientList() const
{
  return m_client_list;
}

QList<AOClient *> Server::clientListByPredicate(const std::function<bool(AOClient *)> &predicate) const
{
  QList<AOClient *> list;
  for (AOClient *client : qAsConst(m_client_list))
  {
    if (predicate(client))
    {
      list.append(client);
    }
  }
  return list;
}

QList<AOClient *> Server::clientListByIpid(const QString &ipid) const
{
  return clientListByPredicate([&](AOClient *client) { return client->getIpid() == ipid; });
}

QList<AOClient *> Server::clientListByHwid(const QString &hwid) const
{
  return clientListByPredicate([&](AOClient *client) { return client->getHwid() == hwid; });
}

bool Server::hasClient(kal::PlayerId id) const
{
  return m_client_map.contains(id);
}

AOClient *Server::client(kal::PlayerId id) const
{
  return m_client_map.value(id);
}

int Server::playerCount() const
{
  return m_client_list.length();
}

QStringList Server::getCharacters()
{
  return m_characters;
}

int Server::getCharacterCount()
{
  return m_characters.length();
}

QString Server::getCharacterById(int f_chr_id)
{
  QString l_chr;

  if (f_chr_id >= 0 && f_chr_id < m_characters.length())
  {
    l_chr = m_characters.at(f_chr_id);
  }

  return l_chr;
}

void Server::updateCharsTaken(AreaData *area)
{
  QList<bool> chars_taken;
  for (const QString &cur_char : qAsConst(m_characters))
  {
    chars_taken.append(area->charactersTaken().contains(getCharID(cur_char)));
  }

  kal::CharacterSelectStatePacket response_cc;
  for (AOClient *client : qAsConst(m_client_list))
  {
    if (client->areaId() == area->index())
    {
      if (!client->m_is_charcursed)
      {
        client->shipPacket(response_cc);
      }
      else
      {
        QList<bool> chars_taken_cursed = getCursedCharsTaken(client, chars_taken);
        kal::CharacterSelectStatePacket response_cc_cursed;
        response_cc_cursed.available = chars_taken_cursed;
        client->shipPacket(response_cc_cursed);
      }
    }
  }
}

void Server::broadcast(const kal::Packet &packet, kal::AreaId id)
{
  QList<int> l_client_ids = m_areas.value(id)->joinedIDs();
  for (const int l_client_id : qAsConst(l_client_ids))
  {
    client(l_client_id)->shipPacket(packet);
  }
}

void Server::broadcast(const kal::Packet &packet)
{
  for (AOClient *client : qAsConst(m_client_list))
  {
    client->shipPacket(packet);
  }
}

void Server::broadcast(const kal::Packet &packet, TARGET_TYPE target)
{
  for (AOClient *client : qAsConst(m_client_list))
  {
    switch (target)
    {
    case TARGET_TYPE::MODCHAT:
      if (client->checkPermission(ACLRole::MODCHAT))
      {
        client->shipPacket(packet);
      }
      break;
    case TARGET_TYPE::ADVERT:
      if (client->m_advert_enabled)
      {
        client->shipPacket(packet);
      }
      break;
    default:
      break;
    }
  }
}

void Server::broadcast(const kal::Packet &packet, const kal::Packet &other_packet, TARGET_TYPE target)
{
  switch (target)
  {
  default:
    break;

  case TARGET_TYPE::AUTHENTICATED:
    for (AOClient *client : qAsConst(m_client_list))
    {
      if (client->isAuthenticated())
      {
        client->shipPacket(other_packet);
      }
      else
      {
        client->shipPacket(packet);
      }
    }
    break;
  }
}

void Server::unicast(const kal::Packet &f_packet, int f_client_id)
{
  AOClient *l_client = client(f_client_id);
  if (l_client != nullptr)
  { // This should never happen, but safety first.
    l_client->shipPacket(f_packet);
    return;
  }
}

kal::CharacterId Server::getCharID(QString char_name)
{
  for (int i = 0; i < m_characters.length(); i++)
  {
    if (m_characters[i].toLower() == char_name.toLower())
    {
      return kal::CharacterId(i);
    }
  }

  return kal::NoCharacterId; // character does not exist
}

bool Server::isAddressBanned(QHostAddress f_remote_IP)
{
  bool l_match_found = false;
  for (const QString &l_ipban : qAsConst(m_ipban_list))
  {
    if (f_remote_IP.isInSubnet(QHostAddress::parseSubnet(l_ipban)))
    {
      l_match_found = true;
      break;
    }
  }
  return l_match_found;
}

QList<AreaData *> Server::areaList()
{
  return m_areas;
}

int Server::getAreaCount()
{
  return m_areas.length();
}

bool Server::hasArea(kal::AreaId id) const
{
  return id >= 0 && id < m_areas.length();
}

AreaData *Server::area(int f_area_id)
{
  AreaData *l_area = nullptr;

  if (f_area_id >= 0 && f_area_id < m_areas.length())
  {
    l_area = m_areas.at(f_area_id);
  }

  return l_area;
}

QQueue<QString> Server::getAreaBuffer(const QString &f_areaName)
{
  return logger->buffer(f_areaName);
}

QStringList Server::getAreaNames()
{
  return m_area_names;
}

QString Server::getAreaName(int f_area_id)
{
  QString l_name;

  if (f_area_id >= 0 && f_area_id < m_area_names.length())
  {
    l_name = m_area_names.at(f_area_id);
  }

  return l_name;
}

QStringList Server::getMusicList()
{
  return m_music_list;
}

QStringList Server::getBackgrounds()
{
  return m_backgrounds;
}

DBManager *Server::getDatabaseManager()
{
  return m_database;
}

ACLRolesHandler *Server::getACLRolesHandler()
{
  return acl_roles_handler;
}

CommandExtensionCollection *Server::getCommandExtensionCollection()
{
  return command_extension_collection;
}

QList<bool> Server::getCursedCharsTaken(AOClient *client, QList<bool> chars_taken)
{
  QList<bool> chars_taken_cursed;
  for (int i = 0; i < chars_taken.length(); i++)
  {
    if (!client->m_charcurse_list.contains(i))
    {
      chars_taken_cursed.append(false);
    }
    else
    {
      chars_taken_cursed.append(chars_taken.value(i));
    }
  }
  return chars_taken_cursed;
}

bool Server::isMessageAllowed() const
{
  return m_can_send_ic_messages;
}

void Server::startMessageFloodguard(int f_duration)
{
  m_can_send_ic_messages = false;
  m_message_floodguard_timer->start(f_duration);
}

void Server::reloadSettings()
{
  ConfigManager::reloadSettings();
  Q_EMIT reloadRequest(ConfigManager::serverName(), ConfigManager::serverDescription());
  Q_EMIT updateHTTPConfiguration();
  handleDiscordIntegration();
  logger->loadLogtext();
  m_ipban_list = ConfigManager::iprangeBans();
  acl_roles_handler->loadFile("config/acl_roles.ini");
  command_extension_collection->loadFile("config/command_extensions.ini");
}

void Server::clientConnected()
{
  QWebSocket *websocket = m_listener->nextPendingConnection();

  QHostAddress address = websocket->peerAddress();
  if (address.protocol() == QAbstractSocket::IPv6Protocol)
  {
    bool ok;
    QHostAddress resolved_address = QHostAddress(address.toIPv4Address(&ok));
    if (ok)
    {
      address = resolved_address;
    }
    else
    {
      websocket->close(QWebSocketProtocol::CloseCodeProtocolError, "IPv6 is not supported.");
      websocket->deleteLater();
      return;
    }
  }

  kal::SocketClient *socket = new kal::SocketClient(websocket);
  if (m_id_stack.empty())
  {
    socket->createAndShipPacket<kal::ModerationNotice>("Server is full.");
    socket->deleteLater();
    return;
  }

  if (isAddressBanned(address))
  {
    socket->createAndShipPacket<kal::ModerationNotice>("Security network has blocked your access.");
    socket->deleteLater();
    return;
  }

  int connections = 0;
  for (AOClient *client : qAsConst(m_client_list))
  {
    if (client->m_remote_ip.isEqual(address))
    {
      ++connections;
    }
  }

  if (connections >= ConfigManager::multiClientLimit() && !address.isLoopback())
  {
    socket->createAndShipPacket<kal::ModerationNotice>("You have reached the maximum number of connections.");
    socket->deleteLater();
    return;
  }

  const QString ipid = calculate_ipid(address);
  if (auto [banned, info] = m_database->isIPBanned(ipid); banned)
  {
    kal::ModerationNotice packet;
    packet.reason = info.reason;
    packet.duration = info.duration;
    packet.id = info.id;
    socket->shipPacket(packet);
    socket->deleteLater();
    return;
  }

  kal::PlayerId player_id = m_id_stack.pop();
  AOClient *client = new AOClient(this, socket, address, ipid, player_id, music_manager);
  m_client_map.insert(player_id, client);
  m_client_list.append(client);
  m_player_state_observer.registerClient(client);

  auto handle_packet = [socket, client]() {
    while (socket->hasPendingPacket())
    {
      kal::Packet packet = socket->nextPendingPacket();
      client->handlePacket(packet);
    }
  };
  connect(socket, &kal::SocketClient::pendingPacketAvailable, client, handle_packet);

  connect(client, &AOClient::logIC, logger, &ULogger::logIC);
  connect(client, &AOClient::logOOC, logger, &ULogger::logOOC);
  connect(client, &AOClient::logLogin, logger, &ULogger::logLogin);
  connect(client, &AOClient::logCMD, logger, &ULogger::logCMD);
  connect(client, &AOClient::logBan, logger, &ULogger::logBan);
  connect(client, &AOClient::logKick, logger, &ULogger::logKick);
  connect(client, &AOClient::logModcall, logger, &ULogger::logModcall);
  connect(client, &AOClient::sessionFinished, this, &Server::removeClient, Qt::QueuedConnection);

  handle_packet();
}

void Server::handleDiscordIntegration()
{
  // Prevent double connecting by preemtively disconnecting them.
  disconnect(this, nullptr, m_discord, nullptr);

  if (ConfigManager::discordWebhookEnabled())
  {
    if (ConfigManager::discordModcallWebhookEnabled())
    {
      connect(this, &Server::modcallWebhookRequest, m_discord, &Discord::onModcallWebhookRequested);
    }

    if (ConfigManager::discordBanWebhookEnabled())
    {
      connect(this, &Server::banWebhookRequest, m_discord, &Discord::onBanWebhookRequested);
    }
  }
  return;
}

void Server::removeClient()
{
  AOClient *client = qobject_cast<AOClient *>(sender());
  if (client == nullptr)
  {
    qCritical() << Q_FUNC_INFO << "called without a sender!";
    return;
  }
  qDebug() << "Removing client" << client->playerId() << "from the server.";

  const kal::PlayerId id = client->playerId();
  m_player_state_observer.unregisterClient(client);
  m_client_list.removeAll(client);
  m_client_map.remove(id);
  m_id_stack.push(id);
}

void Server::allowMessage()
{
  m_can_send_ic_messages = true;
}
