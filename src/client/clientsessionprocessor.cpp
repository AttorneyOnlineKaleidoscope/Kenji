#include "clientsessionprocessor.h"

#include "aoclient.h"
#include "config_manager.h"
#include "db_manager.h"
#include "music_manager.h"
#include "server.h"

#include "network/packet/area_packets.h"
#include "network/packet/background_packets.h"
#include "network/packet/case_packets.h"
#include "network/packet/character_packets.h"
#include "network/packet/chat_packets.h"
#include "network/packet/evidence_packets.h"
#include "network/packet/judge_packets.h"
#include "network/packet/login_packets.h"
#include "network/packet/message_packets.h"
#include "network/packet/moderation_packets.h"
#include "network/packet/music_packets.h"
#include "network/packet/player_packets.h"
#include "network/packet/splash_packets.h"
#include "network/packet/system_packets.h"
#include "network/packet/timer_packets.h"

kal::ClientSessionProcessor::ClientSessionProcessor(AOClient &client)
    : client(client)
{}

void kal::ClientSessionProcessor::cargo(AreaListPacket &cargo)
{}

void kal::ClientSessionProcessor::cargo(UpdateAreaPacket &cargo)
{}

void kal::ClientSessionProcessor::cargo(JoinAreaPacket &cargo)
{
  Server *server = client.getServer();
  if (!server->hasArea(cargo.area_id))
  {
    client.sendServerMessage(QString("Failed to join area %1; does not exist.").arg(cargo.area_id));
    return;
  }

  client.changeArea(cargo.area_id);
}

void kal::ClientSessionProcessor::cargo(BackgroundPacket &cargo)
{}

void kal::ClientSessionProcessor::cargo(PositionPacket &cargo)
{}

void kal::ClientSessionProcessor::cargo(CasePacket_CASEA &cargo)
{
  Q_UNUSED(area)

  QStringList needed_roles;
  auto append_needed_roles = [&](kal::CasePacket_CASEA::Role role) {
    if (cargo.roles.testFlag(role))
    {
      needed_roles.append(QVariant::fromValue(role).toString());
    }
  };
  append_needed_roles(kal::CasePacket_CASEA::Defense);
  append_needed_roles(kal::CasePacket_CASEA::Prosecution);
  append_needed_roles(kal::CasePacket_CASEA::Judge);
  append_needed_roles(kal::CasePacket_CASEA::Jurors);
  append_needed_roles(kal::CasePacket_CASEA::Stenographer);
  if (needed_roles.isEmpty())
  {
    return;
  }

  QString case_title = cargo.title;
  kal::CasePacket_CASEA casea_packet;
  casea_packet.title = "=== Case Announcement ===\r\n" + (client.name() == "" ? client.character() : client.name()) + " needs " + needed_roles.join(", ").toLower() + " for " + (case_title == "" ? "a case" : case_title) + "!";
  casea_packet.roles = cargo.roles;
  client.getServer()->broadcast(casea_packet);
}

void kal::ClientSessionProcessor::cargo(CharacterListPacket &cargo)
{}

void kal::ClientSessionProcessor::cargo(SelectCharacterPacket &cargo)
{
  Q_UNUSED(area)

  kal::CharacterId character_id = cargo.character_id;
  if (character_id < kal::NoCharacterId || character_id > client.getServer()->getCharacters().size() - 1)
  {
    client.createAndShipPacket<kal::ModerationNotice>("Character ID out of range.");
    client.m_socket->close();
  }

  if (client.changeCharacter(character_id))
  {
    client.m_char_id = character_id;
  }

  client.shipPacket(cargo);
}

void kal::ClientSessionProcessor::cargo(CharacterSelectStatePacket &cargo)
{}

void kal::ClientSessionProcessor::cargo(ChatPacket &cargo)
{
  if (client.m_is_ooc_muted)
  {
    client.sendServerMessage("You are OOC muted, and cannot speak.");
    return;
  }

  client.setName(client.dezalgo(cargo.name).replace(QRegularExpression("\\[|\\]|\\{|\\}|\\#|\\$|\\%|\\&"), "")); // no fucky wucky shit here
  if (client.name().isEmpty() || client.name() == ConfigManager::serverName())                                   // impersonation & empty name protection
  {
    return;
  }

  if (client.name().length() > 30)
  {
    client.sendServerMessage("Your name is too long! Please limit it to under 30 characters.");
    return;
  }

  if (client.m_is_logging_in)
  {
    client.loginAttempt(cargo.message);
    return;
  }

  QString l_message = client.dezalgo(cargo.message);

  if (l_message.length() == 0 || l_message.length() > ConfigManager::maxCharacters())
  {
    return;
  }

  if (!ConfigManager::filterList().isEmpty())
  {
    for (const QString &regex : ConfigManager::filterList())
    {
      QRegularExpression re(regex, QRegularExpression::CaseInsensitiveOption);
      l_message.replace(re, "❌");
    }
  }

  if (l_message.at(0) == '/')
  {
    QStringList l_cmd_argv = l_message.split(" ", Qt::SkipEmptyParts);
    QString l_command = l_cmd_argv[0].trimmed().toLower();
    l_command = l_command.right(l_command.length() - 1);
    l_cmd_argv.removeFirst();
    int l_cmd_argc = l_cmd_argv.length();

    client.handleCommand(l_command, l_cmd_argc, l_cmd_argv);
    Q_EMIT client.logCMD((client.character() + " " + client.characterName()), client.m_ipid, client.name(), l_command, l_cmd_argv, client.getServer()->area(client.areaId())->name());
    return;
  }
  else
  {
    client.getServer()->broadcast(kal::ChatPacket(client.name(), l_message), client.areaId());
  }
  Q_EMIT client.logOOC((client.character() + " " + client.characterName()), client.name(), client.m_ipid, area->name(), l_message);
}

void kal::ClientSessionProcessor::cargo(EvidenceListPacket &cargo)
{}

void kal::ClientSessionProcessor::cargo(CreateEvidencePacket &cargo)
{
  if (!client.checkEvidenceAccess(area))
  {
    return;
  }

  area->appendEvidence(cargo.evidence);
  client.sendEvidenceList(area);
}

void kal::ClientSessionProcessor::cargo(EditEvidencePacket &cargo)
{
  if (!client.checkEvidenceAccess(area))
  {
    return;
  }
  bool is_int = false;
  int l_idx = cargo.id;
  kal::Evidence l_evi = cargo.evidence;
  if (is_int && l_idx < area->evidence().size() && l_idx >= 0)
  {
    area->replaceEvidence(l_idx, l_evi);
  }
  client.sendEvidenceList(area);
}

void kal::ClientSessionProcessor::cargo(DeleteEvidencePacket &cargo)
{
  if (!client.checkEvidenceAccess(area))
  {
    return;
  }

  int index = cargo.id;
  if (index < 0 || index >= area->evidence().size())
  {
    return;
  }

  area->deleteEvidence(index);
  client.sendEvidenceList(area);
}

void kal::ClientSessionProcessor::cargo(JudgeStatePacket &cargo)
{}

void kal::ClientSessionProcessor::cargo(HealthStatePacket &cargo)
{
  if (client.m_is_wtce_blocked)
  {
    client.sendServerMessage("You are blocked from using the judge controls.");
    return;
  }

  area->changeHP(cargo.type, cargo.health);
  client.getServer()->broadcast(kal::HealthStatePacket(kal::DefenseHealth, area->defHP()), area->index());
  client.getServer()->broadcast(kal::HealthStatePacket(kal::ProsecutionHealth, area->proHP()), area->index());

  client.updateJudgeLog(area, &client, "updated the penalties");
}

void kal::ClientSessionProcessor::cargo(SplashPacket &cargo)
{
  if (client.m_is_wtce_blocked)
  {
    client.sendServerMessage("You are blocked from using the judge controls.");
    return;
  }

  if (!area->isWtceAllowed())
  {
    client.sendServerMessage("WTCE animations have been disabled in this area.");
    return;
  }

  if (QDateTime::currentDateTime().toSecsSinceEpoch() - client.m_last_wtce_time <= 5)
  {
    return;
  }

  client.m_last_wtce_time = QDateTime::currentDateTime().toSecsSinceEpoch();
  client.getServer()->broadcast(cargo, client.areaId());
  client.updateJudgeLog(area, &client, "WT/CE");
}

void kal::ClientSessionProcessor::cargo(LoginPacket &cargo)
{}

void kal::ClientSessionProcessor::cargo(MessagePacket &cargo)
{}

void kal::ClientSessionProcessor::cargo(ModerationAction &cargo)
{
  Q_UNUSED(area);

  if (!client.m_authenticated)
  {
    client.sendServerMessage("You are not logged in!");
    return;
  }

  int client_id = cargo.player_id;
  kal::ModerationDuration duration = cargo.duration;
  QString reason = cargo.reason;

  if (duration < kal::PermanentModerationActionDuration)
  {
    return;
  }

  bool is_kick = duration == 0;
  if (is_kick)
  {
    if (!client.checkPermission(ACLRole::KICK))
    {
      client.sendServerMessage("You do not have permission to kick users.");
      return;
    }
  }
  else
  {
    if (!client.checkPermission(ACLRole::BAN))
    {
      client.sendServerMessage("You do not have permission to ban users.");
      return;
    }
  }

  AOClient *target = client.getServer()->client(client_id);
  if (target == nullptr)
  {
    client.sendServerMessage("User not found.");
    return;
  }

  QString moderator_name;
  if (ConfigManager::authType() == DataTypes::AuthType::ADVANCED)
  {
    moderator_name = client.m_moderator_name;
  }
  else
  {
    moderator_name = "Moderator";
  }

  QList<AOClient *> clients = client.getServer()->clientListByIpid(target->m_ipid);
  if (is_kick)
  {
    kal::ModerationNotice packet(reason);
    for (AOClient *subclient : clients)
    {
      subclient->shipPacket(packet);
      subclient->m_socket->close();
    }

    Q_EMIT client.logKick(moderator_name, target->m_ipid, reason);

    client.sendServerMessage("Kicked " + QString::number(clients.size()) + " client(s) with ipid " + target->m_ipid + " for reason: " + reason);
  }
  else
  {
    DBManager::BanInfo ban;

    ban.ip = target->m_remote_ip;
    ban.ipid = target->m_ipid;
    ban.moderator = moderator_name;
    ban.reason = reason;
    ban.time = QDateTime::currentDateTime().toSecsSinceEpoch();

    QString timestamp;
    if (duration == kal::PermanentModerationActionDuration)
    {
      ban.duration = kal::PermanentModerationActionDuration;
      timestamp = "permanently";
    }
    else
    {
      ban.duration = duration * 60;
      timestamp = QDateTime::fromSecsSinceEpoch(ban.time).addSecs(ban.duration).toString("MM/dd/yyyy, hh:mm");
    }

    for (AOClient *subclient : clients)
    {
      ban.hdid = subclient->m_hwid;

      client.getServer()->getDatabaseManager()->addBan(ban);

      subclient->createAndShipPacket<kal::ModerationNotice>(reason);
      subclient->m_socket->close();
    }

    if (ban.duration == kal::PermanentModerationActionDuration)
    {
      timestamp = "permanently";
    }
    else
    {
      timestamp = QString::number(ban.time + ban.duration);
    }

    Q_EMIT client.logBan(moderator_name, target->m_ipid, timestamp, reason);

    client.sendServerMessage("Banned " + QString::number(clients.size()) + " client(s) with ipid " + target->m_ipid + " for reason: " + reason);

    int ban_id = client.getServer()->getDatabaseManager()->getBanID(ban.ip);
    if (ConfigManager::discordBanWebhookEnabled())
    {
      Q_EMIT client.getServer()->banWebhookRequest(ban.ipid, ban.moderator, timestamp, ban.reason, ban_id);
    }
  }
}

void kal::ClientSessionProcessor::cargo(ModerationNotice &cargo)
{}

void kal::ClientSessionProcessor::cargo(MusicListPacket &cargo)
{}

void kal::ClientSessionProcessor::cargo(PlayTrackPacket &cargo)
{
  QString l_argument = cargo.track;

  if (client.getServer()->getMusicList().contains(l_argument) || client.m_music_manager->isCustom(client.areaId(), l_argument) || l_argument == "~stop.mp3")
  { // ~stop.mp3 is a dummy track used by 2.9+
    // We have a song here

    if (client.isSpectator())
    {
      client.sendServerMessage("Spectator are blocked from changing the music.");
      return;
    }

    if (client.m_is_dj_blocked)
    {
      client.sendServerMessage("You are blocked from changing the music.");
      return;
    }
    if (!area->isMusicAllowed() && !client.checkPermission(ACLRole::CM))
    {
      client.sendServerMessage("Music is disabled in this area.");
      return;
    }

    QString l_final_song;
    // As categories can be used to stop music we need to check if it has a dot for the extension. If not, we assume its a category.
    if (!l_argument.contains("."))
    {
      l_final_song = "~stop.mp3";
    }
    else
    {
      l_final_song = l_argument;
    }

    // Jukebox intercepts the direct playing of messages.
    if (area->isjukeboxEnabled())
    {
      QString l_jukebox_reply = area->addJukeboxSong(l_final_song);
      client.sendServerMessage(l_jukebox_reply);
      return;
    }

    if (l_final_song != "~stop.mp3")
    {
      // We might have an aliased song. We check for its real songname and send it to the clients.
      QPair<QString, float> l_song = client.m_music_manager->songInformation(l_final_song, client.areaId());
      l_final_song = l_song.first;
    }

    kal::PlayTrackPacket packet = cargo;
    packet.track = l_final_song;
    packet.character_name = client.characterName();
    client.getServer()->broadcast(packet, client.areaId());

    // Since we can't ensure a user has their showname set, we check if its empty to prevent
    //"played by ." in /currentmusic.
    if (client.characterName().isEmpty())
    {
      area->changeMusic(client.character(), l_final_song);
      return;
    }
    area->changeMusic(client.characterName(), l_final_song);
    return;
  }
}

void kal::ClientSessionProcessor::cargo(RegisterPlayerPacket &cargo)
{}

void kal::ClientSessionProcessor::cargo(UpdatePlayerPacket &cargo)
{}

void kal::ClientSessionProcessor::cargo(HelpMePacket &cargo)
{
  QString l_name = client.name();
  if (client.name().isEmpty())
  {
    l_name = client.character();
  }

  QString l_areaName = area->name();

  QString l_modcallNotice = "!!!MODCALL!!!\nArea: " + l_areaName + "\nCaller: " + l_name + "\n";

  int target_id = cargo.player_id.value_or(kal::NoPlayerId);
  if (target_id != -1)
  {
    AOClient *target = client.getServer()->client(target_id);
    if (target)
    {
      l_modcallNotice.append("Regarding: " + target->name() + "\n");
    }
  }
  l_modcallNotice.append("Reason: " + cargo.message);

  kal::HelpMePacket packet(l_modcallNotice);
  const QList<AOClient *> l_clients = client.getServer()->clientList();
  for (AOClient *l_client : l_clients)
  {
    if (l_client->m_authenticated)
    {
      l_client->shipPacket(packet);
    }
  }
  Q_EMIT client.logModcall((client.character() + " " + client.characterName()), client.m_ipid, client.name(), client.getServer()->area(client.areaId())->name());

  if (ConfigManager::discordModcallWebhookEnabled())
  {
    QString l_name = client.name();
    if (client.name().isEmpty())
    {
      l_name = client.character();
    }

    QString l_areaName = area->name();

    QString webhook_reason = cargo.message;
    if (target_id != -1)
    {
      AOClient *target = client.getServer()->client(target_id);
      if (target)
      {
        webhook_reason.append(" (Regarding: " + target->name() + ")");
      }
    }

    Q_EMIT client.getServer()->modcallWebhookRequest(l_name, l_areaName, webhook_reason, client.getServer()->getAreaBuffer(l_areaName));
  }
}

void kal::ClientSessionProcessor::cargo(NoticePacket &cargo)
{}

void kal::ClientSessionProcessor::cargo(SetTimerStatePacket &cargo)
{}
