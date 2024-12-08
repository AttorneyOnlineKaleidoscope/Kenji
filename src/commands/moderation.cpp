#include "aoclient.h"

#include "area_data.h"
#include "command_extension.h"
#include "config_manager.h"
#include "db_manager.h"
#include "network/packet/chat_packets.h"
#include "network/packet/moderation_packets.h"
#include "server.h"

// This file is for commands under the moderation category in aoclient.h
// Be sure to register the command in the header before adding it here!

void AOClient::cmdCommands(int argc, QStringList argv)
{
  Q_UNUSED(argc);
  Q_UNUSED(argv);

  QStringList l_entries;
  l_entries << "Allowed commands:";
  QMap<QString, CommandInfo>::const_iterator i;
  for (i = COMMANDS.constBegin(); i != COMMANDS.constEnd(); ++i)
  {
    const CommandInfo l_command = i.value();
    const CommandExtension l_extension = m_server->getCommandExtensionCollection()->getExtension(i.key());
    const QList<ACLRole::Permission> l_permissions = l_extension.getPermissions(l_command.acl_permissions);
    bool l_has_permission = false;
    for (const ACLRole::Permission i_permission : qAsConst(l_permissions))
    {
      if (checkPermission(i_permission))
      {
        l_has_permission = true;
        break;
      }
    }
    if (!l_has_permission)
    {
      continue;
    }

    QString l_info = "/" + i.key();
    const QStringList l_aliases = l_extension.getAliases();
    if (!l_aliases.isEmpty())
    {
      l_info += " [aka: " + l_aliases.join(", ") + "]";
    }
    l_entries << l_info;
  }
  sendServerMessage(l_entries.join("\n"));
}

void AOClient::cmdHelp(int argc, QStringList argv)
{
  if (argc > 1)
  {
    sendServerMessage("Too many arguments. Please only use the command name.");
    return;
  }

  QString l_command_name = argv[0];
  ConfigManager::help l_command_info = ConfigManager::commandHelp(l_command_name);
  if (l_command_info.usage.isEmpty() || l_command_info.text.isEmpty()) // my picoseconds :(
  {
    sendServerMessage("Unable to find the command " + l_command_name + ".");
  }
  else
  {
    sendServerMessage("==Help==\n" + l_command_info.usage + "\n" + l_command_info.text);
  }
}

void AOClient::cmdMOTD(int argc, QStringList argv)
{
  Q_UNUSED(argc)
  Q_UNUSED(argv)

  sendServerMessage("=== MOTD ===\r\n" + ConfigManager::motd() + "\r\n=============");
}

void AOClient::cmdSetMOTD(int argc, QStringList argv)
{
  Q_UNUSED(argc)

  QString l_MOTD = argv.join(" ");
  ConfigManager::setMotd(l_MOTD);
  sendServerMessage("MOTD has been changed.");
}

void AOClient::cmdAbout(int argc, QStringList argv)
{
  Q_UNUSED(argc);
  Q_UNUSED(argv);

  shipPacket(kal::ChatPacket("The Kenji dev team", "Thank you for using Kenji! Made with love by scatterflower, with help from in1tiate, Salanto, and mangosarentliterature. Kenji " + QCoreApplication::applicationVersion() + ". For documentation and reporting issues, see the source: https://github.com/AttorneyOnlineKaleidoscope/Kenji", true));
}

void AOClient::cmdMods(int argc, QStringList argv)
{
  Q_UNUSED(argc);
  Q_UNUSED(argv);

  QStringList l_entries;
  int l_online_count = 0;
  const QList<AOClient *> l_clients = m_server->clientList();
  for (AOClient *l_client : l_clients)
  {
    if (l_client->m_authenticated)
    {
      l_entries << "---";
      if (ConfigManager::authType() != DataTypes::AuthType::SIMPLE)
      {
        l_entries << "Moderator: " + l_client->m_moderator_name;
        l_entries << "Role:" << l_client->m_acl_role_id;
      }
      l_entries << "OOC name: " + l_client->name();
      l_entries << "ID: " + QString::number(l_client->playerId());
      l_entries << "Area: " + QString::number(l_client->areaId());
      l_entries << "Character: " + l_client->character();
      l_online_count++;
    }
  }
  l_entries << "---";
  l_entries << "Total online: " << QString::number(l_online_count);
  sendServerMessage(l_entries.join("\n"));
}

void AOClient::cmdBan(int argc, QStringList argv)
{
  QString l_args_str = argv[2];
  if (argc > 3)
  {
    for (int i = 3; i < argc; i++)
    {
      l_args_str += " " + argv[i];
    }
  }

  DBManager::BanInfo l_ban;

  long long l_duration_seconds = 0;
  if (argv[1] == "perma")
  {
    l_duration_seconds = kal::PermanentModerationActionDuration;
  }
  else
  {
    l_duration_seconds = parseTime(argv[1]);
  }

  if (l_duration_seconds == -1)
  {
    sendServerMessage("Invalid time format. Format example: 1h30m");
    return;
  }

  l_ban.duration = l_duration_seconds;
  l_ban.ipid = argv[0];
  l_ban.reason = l_args_str;
  l_ban.time = QDateTime::currentDateTime().toSecsSinceEpoch();
  bool l_ban_logged = false;
  int l_kick_counter = 0;

  switch (ConfigManager::authType())
  {
  case DataTypes::AuthType::SIMPLE:
    l_ban.moderator = "moderator";
    break;
  case DataTypes::AuthType::ADVANCED:
    l_ban.moderator = m_moderator_name;
    break;
  }

  const QList<AOClient *> l_targets = m_server->clientListByIpid(l_ban.ipid);
  for (AOClient *l_client : l_targets)
  {
    if (!l_ban_logged)
    {
      l_ban.ip = l_client->m_remote_ip;
      l_ban.hdid = l_client->m_hwid;
      m_server->getDatabaseManager()->addBan(l_ban);
      sendServerMessage("Banned user with ipid " + l_ban.ipid + " for reason: " + l_ban.reason);
      l_ban_logged = true;
    }
    QString l_ban_duration;
    if (!(l_ban.duration == kal::PermanentModerationActionDuration))
    {
      l_ban_duration = QDateTime::fromSecsSinceEpoch(l_ban.time).addSecs(l_ban.duration).toString("MM/dd/yyyy, hh:mm");
    }
    else
    {
      l_ban_duration = "Permanently.";
    }
    int l_ban_id = m_server->getDatabaseManager()->getBanID(l_ban.ip);
    l_client->createAndShipPacket<kal::ModerationNotice>(l_ban.reason, l_ban.duration, l_ban_id);
    l_client->m_socket->close();
    l_kick_counter++;

    Q_EMIT logBan(l_ban.moderator, l_ban.ipid, l_ban_duration, l_ban.reason);
    if (ConfigManager::discordBanWebhookEnabled())
    {
      Q_EMIT m_server->banWebhookRequest(l_ban.ipid, l_ban.moderator, l_ban_duration, l_ban.reason, l_ban_id);
    }
  }

  if (l_kick_counter > 1)
  {
    sendServerMessage("Kicked " + QString::number(l_kick_counter) + " clients with matching ipids.");
  }

  // We're banning someone not connected.
  if (!l_ban_logged)
  {
    m_server->getDatabaseManager()->addBan(l_ban);
    sendServerMessage("Banned " + l_ban.ipid + " for reason: " + l_ban.reason);
  }
}

void AOClient::cmdUnBan(int argc, QStringList argv)
{
  Q_UNUSED(argc);

  bool ok;
  int l_target_ban = argv[0].toInt(&ok);
  if (!ok)
  {
    sendServerMessage("Invalid ban ID.");
    return;
  }
  else if (m_server->getDatabaseManager()->invalidateBan(l_target_ban))
  {
    sendServerMessage("Successfully invalidated ban " + argv[0] + ".");
  }
  else
  {
    sendServerMessage("Couldn't invalidate ban " + argv[0] + ", are you sure it exists?");
  }
}

void AOClient::cmdKick(int argc, QStringList argv)
{
  QString l_target_ipid = argv[0];
  QString l_reason = argv[1];
  int l_kick_counter = 0;

  if (argc > 2)
  {
    for (int i = 2; i < argv.length(); i++)
    {
      l_reason += " " + argv[i];
    }
  }

  const QList<AOClient *> l_targets = m_server->clientListByIpid(l_target_ipid);
  for (AOClient *l_client : l_targets)
  {
    l_client->createAndShipPacket<kal::ModerationNotice>(l_reason);
    l_client->m_socket->close();
    l_kick_counter++;
  }

  if (l_kick_counter > 0)
  {
    if (ConfigManager::authType() == DataTypes::AuthType::ADVANCED)
    {
      Q_EMIT logKick(m_moderator_name, l_target_ipid, l_reason);
    }
    else
    {
      Q_EMIT logKick("Moderator", l_target_ipid, l_reason);
    }
    sendServerMessage("Kicked " + QString::number(l_kick_counter) + " client(s) with ipid " + l_target_ipid + " for reason: " + l_reason);
  }
  else
  {
    sendServerMessage("User with ipid not found!");
  }
}

void AOClient::cmdMute(int argc, QStringList argv)
{
  Q_UNUSED(argc);

  bool conv_ok = false;
  int l_uid = argv[0].toInt(&conv_ok);
  if (!conv_ok)
  {
    sendServerMessage("Invalid user ID.");
    return;
  }

  AOClient *target = m_server->client(l_uid);

  if (target == nullptr)
  {
    sendServerMessage("No client with that ID found.");
    return;
  }

  if (target->m_is_muted)
  {
    sendServerMessage("That player is already muted!");
  }
  else
  {
    sendServerMessage("Muted player.");
    target->sendServerMessage("You were muted by a moderator. " + getReprimand());
  }
  target->m_is_muted = true;
}

void AOClient::cmdUnMute(int argc, QStringList argv)
{
  Q_UNUSED(argc);

  bool conv_ok = false;
  int l_uid = argv[0].toInt(&conv_ok);
  if (!conv_ok)
  {
    sendServerMessage("Invalid user ID.");
    return;
  }

  AOClient *l_target = m_server->client(l_uid);

  if (l_target == nullptr)
  {
    sendServerMessage("No client with that ID found.");
    return;
  }

  if (!l_target->m_is_muted)
  {
    sendServerMessage("That player is not muted!");
  }
  else
  {
    sendServerMessage("Unmuted player.");
    l_target->sendServerMessage("You were unmuted by a moderator. " + getReprimand(true));
  }
  l_target->m_is_muted = false;
}

void AOClient::cmdOocMute(int argc, QStringList argv)
{
  Q_UNUSED(argc);

  bool conv_ok = false;
  int l_uid = argv[0].toInt(&conv_ok);
  if (!conv_ok)
  {
    sendServerMessage("Invalid user ID.");
    return;
  }

  AOClient *l_target = m_server->client(l_uid);

  if (l_target == nullptr)
  {
    sendServerMessage("No client with that ID found.");
    return;
  }

  if (l_target->m_is_ooc_muted)
  {
    sendServerMessage("That player is already OOC muted!");
  }
  else
  {
    sendServerMessage("OOC muted player.");
    l_target->sendServerMessage("You were OOC muted by a moderator. " + getReprimand());
  }
  l_target->m_is_ooc_muted = true;
}

void AOClient::cmdOocUnMute(int argc, QStringList argv)
{
  Q_UNUSED(argc);

  bool conv_ok = false;
  int l_uid = argv[0].toInt(&conv_ok);
  if (!conv_ok)
  {
    sendServerMessage("Invalid user ID.");
    return;
  }

  AOClient *l_target = m_server->client(l_uid);

  if (l_target == nullptr)
  {
    sendServerMessage("No client with that ID found.");
    return;
  }

  if (!l_target->m_is_ooc_muted)
  {
    sendServerMessage("That player is not OOC muted!");
  }
  else
  {
    sendServerMessage("OOC unmuted player.");
    l_target->sendServerMessage("You were OOC unmuted by a moderator. " + getReprimand(true));
  }
  l_target->m_is_ooc_muted = false;
}

void AOClient::cmdBlockWtce(int argc, QStringList argv)
{
  Q_UNUSED(argc);

  bool conv_ok = false;
  int l_uid = argv[0].toInt(&conv_ok);
  if (!conv_ok)
  {
    sendServerMessage("Invalid user ID.");
    return;
  }

  AOClient *l_target = m_server->client(l_uid);

  if (l_target == nullptr)
  {
    sendServerMessage("No client with that ID found.");
    return;
  }

  if (l_target->m_is_wtce_blocked)
  {
    sendServerMessage("That player is already judge blocked!");
  }
  else
  {
    sendServerMessage("Revoked player's access to judge controls.");
    l_target->sendServerMessage("A moderator revoked your judge controls access. " + getReprimand());
  }
  l_target->m_is_wtce_blocked = true;
}

void AOClient::cmdUnBlockWtce(int argc, QStringList argv)
{
  Q_UNUSED(argc);

  bool conv_ok = false;
  int l_uid = argv[0].toInt(&conv_ok);
  if (!conv_ok)
  {
    sendServerMessage("Invalid user ID.");
    return;
  }

  AOClient *l_target = m_server->client(l_uid);

  if (l_target == nullptr)
  {
    sendServerMessage("No client with that ID found.");
    return;
  }

  if (!l_target->m_is_wtce_blocked)
  {
    sendServerMessage("That player is not judge blocked!");
  }
  else
  {
    sendServerMessage("Restored player's access to judge controls.");
    l_target->sendServerMessage("A moderator restored your judge controls access. " + getReprimand(true));
  }
  l_target->m_is_wtce_blocked = false;
}

void AOClient::cmdBans(int argc, QStringList argv)
{
  Q_UNUSED(argc);
  Q_UNUSED(argv);

  QStringList l_recent_bans;
  l_recent_bans << "Last 5 bans:";
  l_recent_bans << "-----";
  const QList<DBManager::BanInfo> l_bans_list = m_server->getDatabaseManager()->getRecentBans();
  for (const DBManager::BanInfo &l_ban : l_bans_list)
  {
    QString l_banned_until;
    if (l_ban.duration == kal::PermanentModerationActionDuration)
    {
      l_banned_until = "The heat death of the universe";
    }
    else
    {
      l_banned_until = QDateTime::fromSecsSinceEpoch(l_ban.time).addSecs(l_ban.duration).toString("MM/dd/yyyy, hh:mm");
    }
    l_recent_bans << "Ban ID: " + QString::number(l_ban.id);
    l_recent_bans << "Affected IPID: " + l_ban.ipid;
    l_recent_bans << "Affected HDID: " + l_ban.hdid;
    l_recent_bans << "Reason for ban: " + l_ban.reason;
    l_recent_bans << "Date of ban: " + QDateTime::fromSecsSinceEpoch(l_ban.time).toString("MM/dd/yyyy, hh:mm");
    l_recent_bans << "Ban lasts until: " + l_banned_until;
    l_recent_bans << "Moderator: " + l_ban.moderator;
    l_recent_bans << "-----";
  }
  sendServerMessage(l_recent_bans.join("\n"));
}

void AOClient::cmdAllowBlankposting(int argc, QStringList argv)
{
  Q_UNUSED(argc);
  Q_UNUSED(argv);

  QString l_sender_name = name();
  AreaData *l_area = m_server->area(areaId());
  l_area->toggleBlankposting();
  if (l_area->blankpostingAllowed() == false)
  {
    sendServerMessageArea(l_sender_name + " has set blankposting in the area to forbidden.");
  }
  else
  {
    sendServerMessageArea(l_sender_name + " has set blankposting in the area to allowed.");
  }
}

void AOClient::cmdBanInfo(int argc, QStringList argv)
{
  QStringList l_ban_info;
  l_ban_info << ("Ban Info for " + argv[0]);
  l_ban_info << "-----";
  QString l_lookup_type;

  if (argc == 1)
  {
    l_lookup_type = "banid";
  }
  else if (argc == 2)
  {
    l_lookup_type = argv[1];
    if (!((l_lookup_type == "banid") || (l_lookup_type == "ipid") || (l_lookup_type == "hdid")))
    {
      sendServerMessage("Invalid ID type.");
      return;
    }
  }
  else
  {
    sendServerMessage("Invalid command.");
    return;
  }
  QString l_id = argv[0];
  const QList<DBManager::BanInfo> l_bans = m_server->getDatabaseManager()->getBanInfo(l_lookup_type, l_id);
  for (const DBManager::BanInfo &l_ban : l_bans)
  {
    QString l_banned_until;
    if (l_ban.duration == kal::PermanentModerationActionDuration)
    {
      l_banned_until = "The heat death of the universe";
    }
    else
    {
      l_banned_until = QDateTime::fromSecsSinceEpoch(l_ban.time).addSecs(l_ban.duration).toString("MM/dd/yyyy, hh:mm");
    }
    l_ban_info << "Ban ID: " + QString::number(l_ban.id);
    l_ban_info << "Affected IPID: " + l_ban.ipid;
    l_ban_info << "Affected HDID: " + l_ban.hdid;
    l_ban_info << "Reason for ban: " + l_ban.reason;
    l_ban_info << "Date of ban: " + QDateTime::fromSecsSinceEpoch(l_ban.time).toString("MM/dd/yyyy, hh:mm");
    l_ban_info << "Ban lasts until: " + l_banned_until;
    l_ban_info << "Moderator: " + l_ban.moderator;
    l_ban_info << "-----";
  }
  sendServerMessage(l_ban_info.join("\n"));
}

void AOClient::cmdReload(int argc, QStringList argv)
{
  Q_UNUSED(argc);
  Q_UNUSED(argv);

  // Todo: Make this a signal when splitting AOClient and Server.
  m_server->reloadSettings();
  sendServerMessage("Reloaded configurations");
}

void AOClient::cmdForceImmediate(int argc, QStringList argv)
{
  Q_UNUSED(argc);
  Q_UNUSED(argv);

  AreaData *l_area = m_server->area(areaId());
  l_area->toggleImmediate();
  QString l_state = l_area->forceImmediate() ? "on." : "off.";
  sendServerMessage("Forced immediate text processing in this area is now " + l_state);
}

void AOClient::cmdAllowIniswap(int argc, QStringList argv)
{
  Q_UNUSED(argc);
  Q_UNUSED(argv);

  AreaData *l_area = m_server->area(areaId());
  l_area->toggleIniswap();
  QString state = l_area->iniswapAllowed() ? "allowed." : "disallowed.";
  sendServerMessage("Iniswapping in this area is now " + state);
}

void AOClient::cmdPermitSaving(int argc, QStringList argv)
{
  Q_UNUSED(argc);

  AOClient *l_client = m_server->client(argv[0].toInt());
  if (l_client == nullptr)
  {
    sendServerMessage("Invalid ID.");
    return;
  }
  l_client->m_testimony_saving = true;
  sendServerMessage("Testimony saving has been enabled for client " + QString::number(l_client->playerId()));
}

void AOClient::cmdKickUid(int argc, QStringList argv)
{
  QString l_reason = argv[1];

  if (argc > 2)
  {
    for (int i = 2; i < argv.length(); i++)
    {
      l_reason += " " + argv[i];
    }
  }

  bool conv_ok = false;
  int l_uid = argv[0].toInt(&conv_ok);
  if (!conv_ok)
  {
    sendServerMessage("Invalid user ID.");
    return;
  }

  AOClient *l_target = m_server->client(l_uid);
  if (l_target == nullptr)
  {
    sendServerMessage("No client with that ID found.");
    return;
  }
  l_target->createAndShipPacket<kal::ModerationNotice>(l_reason);
  l_target->m_socket->close();
  sendServerMessage("Kicked client with UID " + argv[0] + " for reason: " + l_reason);
}

void AOClient::cmdUpdateBan(int argc, QStringList argv)
{
  bool conv_ok = false;
  int l_ban_id = argv[0].toInt(&conv_ok);
  if (!conv_ok)
  {
    sendServerMessage("Invalid ban ID.");
    return;
  }
  QVariant l_updated_info;
  if (argv[1] == "duration")
  {
    long long l_duration_seconds = 0;
    if (argv[2] == "perma")
    {
      l_duration_seconds = kal::PermanentModerationActionDuration;
    }
    else
    {
      l_duration_seconds = parseTime(argv[2]);
    }
    if (l_duration_seconds == -1)
    {
      sendServerMessage("Invalid time format. Format example: 1h30m");
      return;
    }
    l_updated_info = QVariant(l_duration_seconds);
  }
  else if (argv[1] == "reason")
  {
    QString l_args_str = argv[2];
    if (argc > 3)
    {
      for (int i = 3; i < argc; i++)
      {
        l_args_str += " " + argv[i];
      }
    }
    l_updated_info = QVariant(l_args_str);
  }
  else
  {
    sendServerMessage("Invalid update type.");
    return;
  }
  if (!m_server->getDatabaseManager()->updateBan(l_ban_id, argv[1], l_updated_info))
  {
    sendServerMessage("There was an error updating the ban. Please confirm the ban ID is valid.");
    return;
  }
  sendServerMessage("Ban updated.");
}

void AOClient::cmdNotice(int argc, QStringList argv)
{
  Q_UNUSED(argc);

  sendNotice(argv.join(" "));
}
void AOClient::cmdNoticeGlobal(int argc, QStringList argv)
{
  Q_UNUSED(argc);

  sendNotice(argv.join(" "), true);
}

void AOClient::cmdClearCM(int argc, QStringList argv)
{
  Q_UNUSED(argc);
  Q_UNUSED(argv);

  AreaData *l_area = m_server->area(areaId());
  for (int l_client_id : l_area->owners())
  {
    l_area->removeOwner(l_client_id);
  }
  arup(kal::UpdateAreaPacket::CaseMaster, true);
  sendServerMessage("Removed all CMs from this area.");
}

void AOClient::cmdKickOther(int argc, QStringList argv)
{
  Q_UNUSED(argc);
  Q_UNUSED(argv);

  int l_kick_counter = 0;

  QList<AOClient *> l_target_clients;
  const QList<AOClient *> l_targets_hwid = m_server->clientListByHwid(m_hwid);
  l_target_clients = m_server->clientListByIpid(m_ipid);

  // Merge both lookups into one single list.)
  for (AOClient *l_target_candidate : qAsConst(l_targets_hwid))
  {
    if (!l_target_clients.contains(l_target_candidate))
    {
      l_target_clients.append(l_target_candidate);
    }
  }

  // The list is unique, we can only have on instance of the current client.
  l_target_clients.removeOne(this);
  for (AOClient *l_target_client : qAsConst(l_target_clients))
  {
    l_target_client->m_socket->close();
    l_kick_counter++;
  }
  sendServerMessage("Kicked " + QString::number(l_kick_counter) + " multiclients from the server.");
}
