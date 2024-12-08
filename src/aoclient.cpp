#include "aoclient.h"

#include "area_data.h"
#include "client/clienthandshakeprocessor.h"
#include "client/clientsessionprocessor.h"
#include "command_extension.h"
#include "config_manager.h"
#include "network/packet/area_packets.h"
#include "network/packet/background_packets.h"
#include "network/packet/character_packets.h"
#include "network/packet/chat_packets.h"
#include "network/packet/judge_packets.h"
#include "network/packet/timer_packets.h"
#include "server.h"

const QMap<QString, AOClient::CommandInfo> AOClient::COMMANDS{
    {"login", {{ACLRole::NONE}, 0, &AOClient::cmdLogin}},
    {"getarea", {{ACLRole::NONE}, 0, &AOClient::cmdGetArea}},
    {"getareas", {{ACLRole::NONE}, 0, &AOClient::cmdGetAreas}},
    {"ban", {{ACLRole::BAN}, 3, &AOClient::cmdBan}},
    {"kick", {{ACLRole::KICK}, 2, &AOClient::cmdKick}},
    {"changeauth", {{ACLRole::SUPER}, 0, &AOClient::cmdChangeAuth}},
    {"rootpass", {{ACLRole::SUPER}, 1, &AOClient::cmdSetRootPass}},
    {"background", {{ACLRole::NONE}, 1, &AOClient::cmdSetBackground}},
    {"side", {{ACLRole::NONE}, 0, &AOClient::cmdSetSide}},
    {"lock_background", {{ACLRole::CM}, 0, &AOClient::cmdBgLock}},
    {"unlock_background", {{ACLRole::CM}, 0, &AOClient::cmdBgUnlock}},
    {"adduser", {{ACLRole::MODIFY_USERS}, 2, &AOClient::cmdAddUser}},
    {"removeuser", {{ACLRole::MODIFY_USERS}, 1, &AOClient::cmdRemoveUser}},
    {"listusers", {{ACLRole::MODIFY_USERS}, 0, &AOClient::cmdListUsers}},
    {"setperms", {{ACLRole::MODIFY_USERS}, 2, &AOClient::cmdSetPerms}},
    {"removeperms", {{ACLRole::MODIFY_USERS}, 1, &AOClient::cmdRemovePerms}},
    {"listperms", {{ACLRole::NONE}, 0, &AOClient::cmdListPerms}},
    {"logout", {{ACLRole::NONE}, 0, &AOClient::cmdLogout}},
    {"pos", {{ACLRole::NONE}, 1, &AOClient::cmdPos}},
    {"g", {{ACLRole::NONE}, 1, &AOClient::cmdG}},
    {"need", {{ACLRole::NONE}, 1, &AOClient::cmdNeed}},
    {"coinflip", {{ACLRole::NONE}, 0, &AOClient::cmdFlip}},
    {"roll", {{ACLRole::NONE}, 0, &AOClient::cmdRoll}},
    {"rollp", {{ACLRole::NONE}, 0, &AOClient::cmdRollP}},
    {"doc", {{ACLRole::NONE}, 0, &AOClient::cmdDoc}},
    {"cleardoc", {{ACLRole::NONE}, 0, &AOClient::cmdClearDoc}},
    {"cm", {{ACLRole::NONE}, 0, &AOClient::cmdCM}},
    {"uncm", {{ACLRole::CM}, 0, &AOClient::cmdUnCM}},
    {"invite", {{ACLRole::CM}, 1, &AOClient::cmdInvite}},
    {"uninvite", {{ACLRole::CM}, 1, &AOClient::cmdUnInvite}},
    {"area_lock", {{ACLRole::CM}, 0, &AOClient::cmdLock}},
    {"area_spectate", {{ACLRole::CM}, 0, &AOClient::cmdSpectatable}},
    {"area_unlock", {{ACLRole::CM}, 0, &AOClient::cmdUnLock}},
    {"timer", {{ACLRole::CM}, 0, &AOClient::cmdTimer}},
    {"area", {{ACLRole::NONE}, 1, &AOClient::cmdArea}},
    {"play", {{ACLRole::NONE}, 1, &AOClient::cmdPlay}},
    {"area_kick", {{ACLRole::CM}, 1, &AOClient::cmdAreaKick}},
    {"randomchar", {{ACLRole::NONE}, 0, &AOClient::cmdRandomChar}},
    {"switch", {{ACLRole::NONE}, 1, &AOClient::cmdSwitch}},
    {"toggleglobal", {{ACLRole::NONE}, 0, &AOClient::cmdToggleGlobal}},
    {"mods", {{ACLRole::NONE}, 0, &AOClient::cmdMods}},
    {"commands", {{ACLRole::NONE}, 0, &AOClient::cmdCommands}},
    {"status", {{ACLRole::NONE}, 1, &AOClient::cmdStatus}},
    {"forcepos", {{ACLRole::CM}, 2, &AOClient::cmdForcePos}},
    {"currentmusic", {{ACLRole::NONE}, 0, &AOClient::cmdCurrentMusic}},
    {"pm", {{ACLRole::NONE}, 2, &AOClient::cmdPM}},
    {"evidence_mod", {{ACLRole::EVI_MOD}, 1, &AOClient::cmdEvidenceMod}},
    {"motd", {{ACLRole::NONE}, 0, &AOClient::cmdMOTD}},
    {"set_motd", {{ACLRole::MOTD}, 1, &AOClient::cmdSetMOTD}},
    {"announce", {{ACLRole::ANNOUNCE}, 1, &AOClient::cmdAnnounce}},
    {"m", {{ACLRole::MODCHAT}, 1, &AOClient::cmdM}},
    {"gm", {{ACLRole::MODCHAT}, 1, &AOClient::cmdGM}},
    {"mute", {{ACLRole::MUTE}, 1, &AOClient::cmdMute}},
    {"unmute", {{ACLRole::MUTE}, 1, &AOClient::cmdUnMute}},
    {"bans", {{ACLRole::BAN}, 0, &AOClient::cmdBans}},
    {"unban", {{ACLRole::BAN}, 1, &AOClient::cmdUnBan}},
    {"about", {{ACLRole::NONE}, 0, &AOClient::cmdAbout}},
    {"evidence_swap", {{ACLRole::CM}, 2, &AOClient::cmdEvidence_Swap}},
    {"notecard", {{ACLRole::NONE}, 1, &AOClient::cmdNoteCard}},
    {"notecard_reveal", {{ACLRole::CM}, 0, &AOClient::cmdNoteCardReveal}},
    {"notecard_clear", {{ACLRole::NONE}, 0, &AOClient::cmdNoteCardClear}},
    {"8ball", {{ACLRole::NONE}, 1, &AOClient::cmd8Ball}},
    {"lm", {{ACLRole::MODCHAT}, 1, &AOClient::cmdLM}},
    {"judgelog", {{ACLRole::CM}, 0, &AOClient::cmdJudgeLog}},
    {"allow_blankposting", {{ACLRole::MODCHAT}, 0, &AOClient::cmdAllowBlankposting}},
    {"gimp", {{ACLRole::MUTE}, 1, &AOClient::cmdGimp}},
    {"ungimp", {{ACLRole::MUTE}, 1, &AOClient::cmdUnGimp}},
    {"baninfo", {{ACLRole::BAN}, 1, &AOClient::cmdBanInfo}},
    {"testify", {{ACLRole::CM}, 0, &AOClient::cmdTestify}},
    {"testimony", {{ACLRole::NONE}, 0, &AOClient::cmdTestimony}},
    {"examine", {{ACLRole::CM}, 0, &AOClient::cmdExamine}},
    {"pause", {{ACLRole::CM}, 0, &AOClient::cmdPauseTestimony}},
    {"delete", {{ACLRole::CM}, 0, &AOClient::cmdDeleteStatement}},
    {"update", {{ACLRole::CM}, 0, &AOClient::cmdUpdateStatement}},
    {"add", {{ACLRole::CM}, 0, &AOClient::cmdAddStatement}},
    {"reload", {{ACLRole::SUPER}, 0, &AOClient::cmdReload}},
    {"disemvowel", {{ACLRole::MUTE}, 1, &AOClient::cmdDisemvowel}},
    {"undisemvowel", {{ACLRole::MUTE}, 1, &AOClient::cmdUnDisemvowel}},
    {"shake", {{ACLRole::MUTE}, 1, &AOClient::cmdShake}},
    {"unshake", {{ACLRole::MUTE}, 1, &AOClient::cmdUnShake}},
    {"force_noint_pres", {{ACLRole::CM}, 0, &AOClient::cmdForceImmediate}},
    {"allow_iniswap", {{ACLRole::CM}, 0, &AOClient::cmdAllowIniswap}},
    {"afk", {{ACLRole::NONE}, 0, &AOClient::cmdAfk}},
    {"savetestimony", {{ACLRole::NONE}, 1, &AOClient::cmdSaveTestimony}},
    {"loadtestimony", {{ACLRole::CM}, 1, &AOClient::cmdLoadTestimony}},
    {"permitsaving", {{ACLRole::MODCHAT}, 1, &AOClient::cmdPermitSaving}},
    {"mutepm", {{ACLRole::NONE}, 0, &AOClient::cmdMutePM}},
    {"toggleadverts", {{ACLRole::NONE}, 0, &AOClient::cmdToggleAdverts}},
    {"ooc_mute", {{ACLRole::MUTE}, 1, &AOClient::cmdOocMute}},
    {"ooc_unmute", {{ACLRole::MUTE}, 1, &AOClient::cmdOocUnMute}},
    {"block_wtce", {{ACLRole::MUTE}, 1, &AOClient::cmdBlockWtce}},
    {"unblock_wtce", {{ACLRole::MUTE}, 1, &AOClient::cmdUnBlockWtce}},
    {"block_dj", {{ACLRole::MUTE}, 1, &AOClient::cmdBlockDj}},
    {"unblock_dj", {{ACLRole::MUTE}, 1, &AOClient::cmdUnBlockDj}},
    {"charcurse", {{ACLRole::MUTE}, 1, &AOClient::cmdCharCurse}},
    {"uncharcurse", {{ACLRole::MUTE}, 1, &AOClient::cmdUnCharCurse}},
    {"charselect", {{ACLRole::NONE}, 0, &AOClient::cmdCharSelect}},
    {"force_charselect", {{ACLRole::FORCE_CHARSELECT}, 1, &AOClient::cmdForceCharSelect}},
    {"togglemusic", {{ACLRole::CM}, 0, &AOClient::cmdToggleMusic}},
    {"a", {{ACLRole::NONE}, 2, &AOClient::cmdA}},
    {"s", {{ACLRole::NONE}, 0, &AOClient::cmdS}},
    {"kick_uid", {{ACLRole::KICK}, 2, &AOClient::cmdKickUid}},
    {"firstperson", {{ACLRole::NONE}, 0, &AOClient::cmdFirstPerson}},
    {"update_ban", {{ACLRole::BAN}, 3, &AOClient::cmdUpdateBan}},
    {"changepass", {{ACLRole::NONE}, 1, &AOClient::cmdChangePassword}},
    {"ignore_bglist", {{ACLRole::IGNORE_BGLIST}, 0, &AOClient::cmdIgnoreBgList}},
    {"notice", {{ACLRole::SEND_NOTICE}, 1, &AOClient::cmdNotice}},
    {"noticeg", {{ACLRole::SEND_NOTICE}, 1, &AOClient::cmdNoticeGlobal}},
    {"togglejukebox", {{ACLRole::CM, ACLRole::JUKEBOX}, 0, &AOClient::cmdToggleJukebox}},
    {"help", {{ACLRole::NONE}, 1, &AOClient::cmdHelp}},
    {"clearcm", {{ACLRole::KICK}, 0, &AOClient::cmdClearCM}},
    {"togglemessage", {{ACLRole::CM}, 0, &AOClient::cmdToggleAreaMessageOnJoin}},
    {"clearmessage", {{ACLRole::CM}, 0, &AOClient::cmdClearAreaMessage}},
    {"areamessage", {{ACLRole::CM}, 0, &AOClient::cmdAreaMessage}},
    {"webfiles", {{ACLRole::NONE}, 0, &AOClient::cmdWebfiles}},
    {"addsong", {{ACLRole::CM}, 1, &AOClient::cmdAddSong}},
    {"addcategory", {{ACLRole::CM}, 1, &AOClient::cmdAddCategory}},
    {"removeentry", {{ACLRole::CM}, 1, &AOClient::cmdRemoveCategorySong}},
    {"toggleroot", {{ACLRole::CM}, 0, &AOClient::cmdToggleRootlist}},
    {"clearcustom", {{ACLRole::CM}, 0, &AOClient::cmdClearCustom}},
    {"toggle_wtce", {{ACLRole::CM}, 0, &AOClient::cmdToggleWtce}},
    {"toggle_shouts", {{ACLRole::CM}, 0, &AOClient::cmdToggleShouts}},
    {"kick_other", {{ACLRole::NONE}, 0, &AOClient::cmdKickOther}},
    {"jukebox_skip", {{ACLRole::CM}, 0, &AOClient::cmdJukeboxSkip}},
    {"play_ambience", {{ACLRole::NONE}, 1, &AOClient::cmdPlayAmbience}},
};

AOClient::AOClient(Server *server, kal::SocketClient *socket, const QHostAddress &remoteIp, const QString &ipid, kal::PlayerId playerId, MusicManager *musicManager, QObject *parent)
    : QObject(parent)
    , m_server(server)
    , m_socket(socket)
    , m_remote_ip(remoteIp)
    , m_ipid(ipid)
    , m_id(playerId)
    , m_music_manager(musicManager)
    , m_last_wtce_time(0)
    , m_handshake_processor(*this)
    , m_session_processor(*this)
{
  m_afk_timer = new QTimer;
  m_afk_timer->setSingleShot(true);
  connect(m_afk_timer, &QTimer::timeout, this, &AOClient::onAfkTimeout);

  connect(m_socket, &kal::SocketClient::websocketDisconnected, this, &AOClient::finishSession);
}

AOClient::~AOClient()
{
  disconnect(m_socket, &kal::SocketClient::websocketDisconnected, this, &AOClient::finishSession);

  m_socket->deleteLater();
}

QString AOClient::getIpid() const
{
  return m_ipid;
}

QString AOClient::getHwid() const
{
  return m_hwid;
}

bool AOClient::isAuthenticated() const
{
  return m_authenticated;
}

Server *AOClient::getServer()
{
  return m_server;
}

kal::PlayerId AOClient::playerId() const
{
  return m_id;
}

QString AOClient::name() const
{
  return m_ooc_name;
}

void AOClient::setName(const QString &f_name)
{
  m_ooc_name = f_name;
  Q_EMIT nameChanged(m_ooc_name);
}

QString AOClient::character() const
{
  return m_current_char;
}

void AOClient::setCharacter(const QString &f_character)
{
  m_current_char = f_character;
  Q_EMIT characterChanged(m_current_char);
}

QString AOClient::characterName() const
{
  return m_showname;
}

void AOClient::setCharacterName(const QString &f_showname)
{
  m_showname = f_showname;
  Q_EMIT characterNameChanged(m_showname);
}

kal::AreaId AOClient::areaId() const
{
  return m_current_area;
}

void AOClient::setAreaId(const kal::AreaId f_area_id)
{
  m_current_area = f_area_id;
  Q_EMIT areaIdChanged(m_current_area);
}

void AOClient::finishHandshake()
{
  m_processor = &m_session_processor;
}

bool AOClient::checkPermission(ACLRole::Permission f_permission) const
{
  if (f_permission == ACLRole::NONE)
  {
    return true;
  }

  if ((f_permission == ACLRole::CM) && m_server->area(areaId())->owners().contains(playerId()))
  {
    return true; // I'm sorry for this hack.
  }

  if (!isAuthenticated())
  {
    return false;
  }

  if (ConfigManager::authType() == DataTypes::AuthType::SIMPLE)
  {
    return true;
  }

  const ACLRole l_role = m_server->getACLRolesHandler()->getRoleById(m_acl_role_id);
  return l_role.checkPermission(f_permission);
}

bool AOClient::isSpectator() const
{
  return m_char_id == kal::NoCharacterId;
}

void AOClient::arup(kal::UpdateAreaPacket::Update type, bool broadcast)
{
  kal::UpdateAreaPacket l_arup_packet;
  QStringList l_arup_data;
  l_arup_data.append(QString::number(type));
  const QList<AreaData *> l_areas = m_server->areaList();
  for (AreaData *l_area : l_areas)
  {
    switch (type)
    {
    case kal::UpdateAreaPacket::PlayerCount:
      {
        l_arup_data.append(QString::number(l_area->playerCount()));
        break;
      }
    case kal::UpdateAreaPacket::Status:
      {
        QString l_area_status = QVariant::fromValue(l_area->status()).toString().replace("_", "-"); // LOOKING_FOR_PLAYERS to LOOKING-FOR-PLAYERS
        l_arup_data.append(l_area_status);
        break;
      }
    case kal::UpdateAreaPacket::CaseMaster:
      {
        if (l_area->owners().isEmpty())
        {
          l_arup_data.append("FREE");
        }
        else
        {
          QStringList l_area_owners;
          const QList<int> l_owner_ids = l_area->owners();
          for (int l_owner_id : l_owner_ids)
          {
            AOClient *l_owner = m_server->client(l_owner_id);
            l_area_owners.append("[" + QString::number(l_owner->playerId()) + "] " + l_owner->character());
          }
          l_arup_data.append(l_area_owners.join(", "));
        }
        break;
      }
    case kal::UpdateAreaPacket::Locked:
      {
        QString l_lock_status = QVariant::fromValue(l_area->lockStatus()).toString();
        l_arup_data.append(l_lock_status);
        break;
      }
    default:
      {
        return;
      }
    }
  }
  if (broadcast)
  {
    // TODO rework ARUP (area updates)
    m_server->broadcast(l_arup_packet);
  }
  else
  {
    // TODO rework ARUP (area updates)
    shipPacket(l_arup_packet);
  }
}

void AOClient::fullArup()
{
  arup(kal::UpdateAreaPacket::PlayerCount, false);
  arup(kal::UpdateAreaPacket::Status, false);
  arup(kal::UpdateAreaPacket::CaseMaster, false);
  arup(kal::UpdateAreaPacket::Locked, false);
}

void AOClient::sendServerMessage(QString message)
{
  shipPacket(kal::ChatPacket(ConfigManager::serverTag(), message, true));
}

void AOClient::sendServerMessageArea(QString message)
{
  m_server->broadcast(kal::ChatPacket(ConfigManager::serverTag(), message, true), areaId());
}

void AOClient::sendServerBroadcast(QString message)
{
  m_server->broadcast(kal::ChatPacket(ConfigManager::serverTag(), message, true));
}

bool AOClient::changeCharacter(int char_id)
{
  AreaData *l_area = m_server->area(areaId());

  if (char_id >= m_server->getCharacterCount())
  {
    return false;
  }

  if (m_is_charcursed && !m_charcurse_list.contains(char_id))
  {
    return false;
  }

  bool l_successfulChange = l_area->changeCharacter(m_server->getCharID(character()), char_id);

  if (char_id < 0)
  {
    setCharacter("");
  }
  m_char_id = char_id;

  if (l_successfulChange == true)
  {
    QString l_char_selected = m_server->getCharacterById(char_id);
    setCharacter(l_char_selected);
    m_pos = "";
    m_server->updateCharsTaken(l_area);
    shipPacket(kal::SelectCharacterPacket(char_id));
    return true;
  }
  return false;
}

void AOClient::changeArea(kal::AreaId id)
{
  if (!m_server->hasArea(id))
  {
    sendServerMessage(QStringLiteral("Area %1 does not exist.").arg(id));
    return;
  }

  if (m_current_area == id)
  {
    sendServerMessage(QStringLiteral("You are already in area %1.").arg(m_server->getAreaName(id)));
    return;
  }

  AreaData *area = m_server->area(id);
  if (area->lockStatus() == AreaData::LockStatus::LOCKED && !area->invited().contains(playerId()) && !checkPermission(ACLRole::BYPASS_LOCKS))
  {
    sendServerMessage("Area " + m_server->getAreaName(id) + " is locked.");
    return;
  }

  if (character() != "")
  {
    m_server->area(areaId())->changeCharacter(m_server->getCharID(character()), -1);
    m_server->updateCharsTaken(m_server->area(areaId()));
  }
  m_server->area(areaId())->removeClient(m_char_id, playerId());
  bool l_character_taken = false;
  if (m_server->area(id)->charactersTaken().contains(m_server->getCharID(character())))
  {
    setCharacter("");
    m_char_id = -1;
    l_character_taken = true;
  }
  m_server->area(id)->addClient(m_char_id, playerId());
  setAreaId(id);
  arup(kal::UpdateAreaPacket::PlayerCount, true);
  sendEvidenceList(m_server->area(id));

  shipPacket(kal::HealthStatePacket(kal::DefenseHealth, m_server->area(id)->defHP()));
  shipPacket(kal::HealthStatePacket(kal::ProsecutionHealth, m_server->area(id)->proHP()));
  shipPacket(kal::BackgroundPacket(m_server->area(id)->background(), m_server->area(id)->side()));

  const QList<QTimer *> l_timers = m_server->area(areaId())->timers();
  for (QTimer *l_timer : l_timers)
  {
    int l_timer_id = m_server->area(areaId())->timers().indexOf(l_timer) + 1;
    if (l_timer->isActive())
    {
      shipPacket(kal::SetTimerStatePacket(l_timer_id, kal::ShowTimer));
      shipPacket(kal::SetTimerStatePacket(l_timer_id, kal::StartTimer, QTime(0, 0).msecsTo(QTime(0, 0).addMSecs(l_timer->remainingTime()))));
    }
    else
    {
      shipPacket(kal::SetTimerStatePacket(l_timer_id, kal::HideTimer));
    }
  }
  sendServerMessage("You moved to area " + m_server->getAreaName(areaId()));
  if (m_server->area(areaId())->sendAreaMessageOnJoin())
  {
    sendServerMessage(m_server->area(areaId())->areaMessage());
  }

  if (m_server->area(areaId())->lockStatus() == AreaData::LockStatus::SPECTATABLE)
  {
    sendServerMessage("Area " + m_server->getAreaName(areaId()) + " is spectate-only; to chat IC you will need to be invited by the CM.");
  }
}

void AOClient::handleCommand(QString command, int argc, QStringList argv)
{
  command = command.toLower();
  QString l_target_command = command;
  QList<ACLRole::Permission> l_permissions;

  // check for aliases
  const QList<CommandExtension> l_extensions = m_server->getCommandExtensionCollection()->getExtensions();
  for (const CommandExtension &i_extension : l_extensions)
  {
    if (i_extension.checkCommandNameAndAlias(command))
    {
      l_target_command = i_extension.getCommandName();
      l_permissions = i_extension.getPermissions();
      break;
    }
  }

  CommandInfo l_command = COMMANDS.value(l_target_command, {{ACLRole::NONE}, -1, &AOClient::cmdDefault});
  if (l_permissions.isEmpty())
  {
    l_permissions.append(l_command.acl_permissions);
  }

  bool l_has_permissions = false;
  for (const ACLRole::Permission i_permission : qAsConst(l_permissions))
  {
    if (checkPermission(i_permission))
    {
      l_has_permissions = true;
      break;
    }
  }
  if (!l_has_permissions)
  {
    sendServerMessage("You do not have permission to use that command.");
    return;
  }

  if (argc < l_command.minArgs)
  {
    sendServerMessage("Invalid command syntax.");
    sendServerMessage("The expected syntax for this command is: \n" + ConfigManager::commandHelp(command).usage);
    return;
  }

  (this->*(l_command.action))(argc, argv);
}

void AOClient::handlePacket(const kal::Packet &packet)
{
  AreaData *area = m_server->area(areaId());
  m_processor->setArea(area);
  packet.process(*m_processor);
  if (m_processor->error())
  {
    qWarning().noquote() << "Error processing packet" << packet.header() << ":" << m_processor->error() << m_processor->errorString();
    return;
  }

  if (m_is_afk)
  {
    sendServerMessage("You are no longer AFK.");
  }
  m_is_afk = false;
  m_afk_timer->start(ConfigManager::afkTimeout() * 1000);
}

void AOClient::finishSession()
{
  m_server->area(areaId())->removeClient(m_server->getCharID(character()), playerId());
  arup(kal::UpdateAreaPacket::PlayerCount, true);

  if (character() != "")
  {
    m_server->updateCharsTaken(m_server->area(areaId()));
  }

  bool l_updateLocks = false;

  const QList<AreaData *> l_areas = m_server->areaList();
  for (AreaData *l_area : l_areas)
  {
    l_updateLocks = l_updateLocks || l_area->removeOwner(playerId());
  }

  if (l_updateLocks)
  {
    arup(kal::UpdateAreaPacket::Locked, true);
  }
  arup(kal::UpdateAreaPacket::CaseMaster, true);

  Q_EMIT sessionFinished();
}

void AOClient::shipPacket(const kal::Packet &packet)
{
  m_socket->shipPacket(packet);
}

void AOClient::onAfkTimeout()
{
  if (!m_is_afk)
  {
    sendServerMessage("You are now AFK.");
  }
  m_is_afk = true;
}

void AOClient::changePosition(QString new_pos)
{
  m_pos = new_pos;
  sendServerMessage("Position changed to " + m_pos + ".");
  shipPacket(kal::PositionPacket(m_pos));
}
