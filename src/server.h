#pragma once

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QMap>
#include <QProperty>
#include <QSettings>
#include <QStack>
#include <QString>
#include <QTimer>
#include <QWebSocket>
#include <QWebSocketServer>

#include "playerstateobserver.h"

class ACLRolesHandler;
class ServerPublisher;
class AOClient;
class AreaData;
class CommandExtensionCollection;
class ConfigManager;
class DBManager;
class Discord;
class MusicManager;
class ULogger;

/**
 * @brief The class that represents the actual server as it is.
 */
class Server : public QObject
{
  Q_OBJECT

public:
  enum class TARGET_TYPE
  {
    AUTHENTICATED,
    MODCHAT,
    ADVERT,
  };
  Q_ENUM(TARGET_TYPE)

  Server(QObject *parent = nullptr);
  ~Server();

  void start(int port);

  QList<AOClient *> clientList() const;
  QList<AOClient *> clientListByPredicate(const std::function<bool(AOClient *)> &predicate) const;

  // helper functions
  QList<AOClient *> clientListByIpid(const QString &ipid) const;
  QList<AOClient *> clientListByHwid(const QString &hwid) const;

  bool hasClient(kal::PlayerId id) const;
  AOClient *client(kal::PlayerId id) const;

  /**
   * @brief Returns the overall player count in the server.
   *
   * @return The overall player count in the server.
   */
  int playerCount() const;

  /**
   * @brief Returns a list of the available characters on the server to use.
   *
   * @return A list of the available characters on the server to use.
   */
  QStringList getCharacters();

  /**
   * @brief Returns the count of available characters on the server to use.
   *
   * @return The count of available characters on the server to use.
   */
  int getCharacterCount();

  /**
   * @brief Get the available character by index.
   *
   * @param f_chr_id The index of the character.
   *
   * @return The character if it exist, otherwise an empty stirng.
   */
  QString getCharacterById(int f_chr_id);

  /**
   * @brief Updates which characters are taken in the given area, and sends out an update packet to
   * all clients present the area.
   *
   * @param area The area in which to update the list of characters.
   */
  void updateCharsTaken(AreaData *area);

  /**
   * @brief Sends a packet to all clients in a given area.
   *
   * @param packet The packet to send to the clients.
   *
   * @param id The index of the area to look for clients in.
   *
   * @note Does nothing if an area by the given index does not exist.
   */
  void broadcast(const kal::Packet &packet, kal::AreaId id);

  /**
   * @brief Sends a packet to all clients in the server.
   *
   * @param packet The packet to send to the clients.
   */
  void broadcast(const kal::Packet &packet);

  /**
   * @brief Sends a packet to a specific usergroup..
   *
   * @param The packet to send to the clients.
   *
   * @param ENUM to determine the targets of the altered packet.
   */
  void broadcast(const kal::Packet &packet, TARGET_TYPE target);

  /**
   * @brief Sends a packet to clients, sends an altered packet to a specific usergroup.
   *
   * @param The packet to send to the clients.
   *
   * @param The altered packet to send to the other clients.
   *
   * @param ENUM to determine the targets of the altered packet.
   */
  void broadcast(const kal::Packet &packet, const kal::Packet &other_packet, enum TARGET_TYPE target);

  /**
   * @brief Sends a packet to a single client.
   *
   * @param The packet send to the client.
   *
   * @param The temporary userID of the client.
   */
  void unicast(const kal::Packet &f_packet, int f_client_id);

  /**
   * @brief Returns the character's character ID (= their index in the character list).
   *
   * @param char_name The 'internal' name for the character whose character ID to look up. This is equivalent to
   * the name of the directory of the character.
   *
   * @return The character ID if a character with that name exists in the character selection list, `-1` if not.
   */
  kal::CharacterId getCharID(QString char_name);

  /**
   * @brief Checks if an IP is in a subnet of the IPBanlist.
   **/
  bool isAddressBanned(QHostAddress f_remote_IP);

  /**
   * @brief Returns the list of areas in the server.
   *
   * @return A list of areas.
   */
  QList<AreaData *> areaList();

  /**
   * @brief Returns the number of areas in the server.
   */
  int getAreaCount();

  /**
   * @brief Returns a pointer to the area associated with the index.
   *
   * @param f_area_id The index of the area.
   *
   * @return A pointer to the area or null.
   */
  bool hasArea(kal::AreaId id) const;
  AreaData *area(int f_area_id);

  /**
   * @brief Getter for an area specific buffer from the logger.
   */
  QQueue<QString> getAreaBuffer(const QString &f_areaName);

  /**
   * @brief The names of the areas on the server.
   *
   * @return A list of names.
   */
  QStringList getAreaNames();

  /**
   * @brief Returns the name of the area associated with the index.
   *
   * @param f_area_id The index of the area.
   *
   * @return The name of the area or empty.
   */
  QString getAreaName(int f_area_id);

  /**
   * @brief Returns the available songs on the server.
   *
   * @return A list of songs.
   */
  QStringList getMusicList();

  /**
   * @brief Returns the available backgrounds on the server.
   *
   * @return A list of backgrounds.
   */
  QStringList getBackgrounds();

  /**
   * @brief Returns a pointer to a database manager.
   *
   * @return A pointer to a database manager.
   */
  DBManager *getDatabaseManager();

  /**
   * @brief Returns a pointer to ACL role handler.
   */
  ACLRolesHandler *getACLRolesHandler();

  /**
   * @brief Returns a pointer to a command extension collection.
   */
  CommandExtensionCollection *getCommandExtensionCollection();

  /**
   * @brief The server-wide global timer.
   */
  QTimer *timer;

  QList<bool> getCursedCharsTaken(AOClient *client, QList<bool> chars_taken);

  /**
   * @brief Returns whatever a game message may be broadcasted or not.
   *
   * @return True if expired; false otherwise.
   */
  bool isMessageAllowed() const;

  /**
   * @brief Starts a global timer that determines whatever a game message may be broadcasted or not.
   *
   * @param f_duration The duration of the message floodguard timer.
   */
  void startMessageFloodguard(int f_duration);

public Q_SLOTS:
  /**
   * @brief Convenience class to call a reload of available configuraiton elements.
   */
  void reloadSettings();

  /**
   * @brief Handles a new connection.
   *
   * @details The function creates an AOClient to represent the user, assigns a user ID to them, and
   * checks if the client is banned.
   */
  void clientConnected();

  /**
   * @brief Method to construct and reconstruct Discord Webhook Integration.
   *
   * @details Constructs or rebuilds Discord Object during server startup and configuration reload.
   */
  void handleDiscordIntegration();

  /**
   * @brief Marks a userID as free and ads it back to the available client id queue.
   */
  void removeClient();

Q_SIGNALS:

  /**
   * @brief Sends the server name and description, emitted by /reload.
   *
   * @param p_name The server name.
   * @param p_desc The server description.
   */
  void reloadRequest(QString p_name, QString p_desc);

  /**
   * @brief Triggers a partial update of the modern advertiser as some information, such as ports
   * can't be updated while the server is running.
   */
  void updateHTTPConfiguration();

  /**
   * @brief Sends a modcall webhook request, emitted by AOClient::pktModcall.
   *
   * @param f_name The character or OOC name of the client who sent the modcall.
   * @param f_area The name of the area the modcall was sent from.
   * @param f_reason The reason the client specified for the modcall.
   * @param f_buffer The area's log buffer.
   */
  void modcallWebhookRequest(const QString &f_name, const QString &f_area, const QString &f_reason, const QQueue<QString> &f_buffer);

  /**
   * @brief Sends a ban webhook request, emitted by AOClient::cmdBan
   * @param f_ipid The IPID of the banned client.
   * @param f_moderator The moderator who issued the ban.
   * @param f_duration The duration of the ban in a human readable format.
   * @param f_reason The reason for the ban.
   * @param f_banID The ID of the issued ban.
   */
  void banWebhookRequest(const QString &f_ipid, const QString &f_moderator, const QString &f_duration, const QString &f_reason, const int &f_banID);

  /**
   * @brief Signal connected to universal logger. Logs a client connection attempt.
   * @param f_ip_address The IP Address of the incoming connection.
   * @param f_ipid The IPID of the incoming connection.
   * @param f_hdid The HDID of the incoming connection.
   */
  void logConnectionAttempt(const QString &f_ip_address, const QString &f_ipid, const QString &f_hwid);

private:
  /**
   * @brief Listens for incoming websocket connections.
   */
  QWebSocketServer *m_listener;

  /**
   * @brief Handles Discord webhooks.
   */
  Discord *m_discord;

  /**
   * @brief Handles HTTP server advertising.
   */
  ServerPublisher *server_publisher;

  /**
   * @brief Handles the universal log framework.
   */
  ULogger *logger;

  /**
   * @brief Handles all musiclists.
   */
  MusicManager *music_manager;

  /**
   * @brief The collection of all currently connected clients.
   */
  QList<AOClient *> m_client_list;

  /**
   * @brief Collection of all clients with their userID as key.
   */
  QHash<kal::PlayerId, AOClient *> m_client_map;
  PlayerStateObserver m_player_state_observer;

  QProperty<int> m_player_count;

  /**
   * @brief Stack of all available IDs for clients. When this is empty the server
   * rejects any new connection attempt.
   */
  QStack<kal::PlayerId> m_id_stack;

  /**
   * @brief The characters available on the server to use.
   */
  QStringList m_characters;

  /**
   * @brief The areas on the server.
   */
  QList<AreaData *> m_areas;

  /**
   * @brief The names of the areas on the server.
   *
   * @details Equivalent to iterating over #areas and getting the area names individually, but grouped together
   * here for faster access.
   */
  QStringList m_area_names;

  /**
   * @brief The available songs on the server.
   *
   * @details Does **not** include the area names, the actual music list packet should be constructed from
   * #area_names and this combined.
   */
  QStringList m_music_list;

  /**
   * @brief The backgrounds on the server that may be used in areas.
   */
  QStringList m_backgrounds;

  /**
   * @brief Collection of all IPs that are banned.
   */
  QStringList m_ipban_list;

  /**
   * @brief Timer until the next IC message can be sent.
   */
  QTimer *m_message_floodguard_timer;

  /**
   * @brief If false, IC messages will be rejected.
   */
  bool m_can_send_ic_messages = true;

  /**
   * @brief The database manager on the server, used to store users' bans and authorisation details.
   */
  DBManager *m_database;

  /**
   * @see ACLRolesHandler
   */
  ACLRolesHandler *acl_roles_handler;

  /**
   * @see CommandExtensionCollection
   */
  CommandExtensionCollection *command_extension_collection;

private Q_SLOTS:
  /**
   * @brief Allow game messages to be broadcasted.
   */
  void allowMessage();
};
