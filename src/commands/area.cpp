#include "aoclient.h"

#include "area_data.h"
#include "config_manager.h"
#include "network/packet/background_packets.h"
#include "network/packet/chat_packets.h"
#include "network/packet/music_packets.h"
#include "server.h"

// This file is for commands under the area category in aoclient.h
// Be sure to register the command in the header before adding it here!

void AOClient::cmdCM(int argc, QStringList argv)
{
  QString l_sender_name = name();
  AreaData *l_area = m_server->area(areaId());
  if (l_area->isProtected())
  {
    sendServerMessage("This area is protected, you may not become CM.");
    return;
  }
  else if (l_area->owners().isEmpty())
  { // no one owns this area, and it's not protected
    l_area->addOwner(playerId());
    sendServerMessageArea(l_sender_name + " is now CM in this area.");
    arup(kal::UpdateAreaPacket::CaseMaster, true);
  }
  else if (!l_area->owners().contains(playerId()))
  { // there is already a CM, and it isn't us
    sendServerMessage("You cannot become a CM in this area.");
  }
  else if (argc == 1)
  { // we are CM, and we want to make ID argv[0] also CM
    bool ok;
    AOClient *l_owner_candidate = m_server->client(argv[0].toInt(&ok));
    if (!ok)
    {
      sendServerMessage("That doesn't look like a valid ID.");
      return;
    }
    if (l_owner_candidate == nullptr)
    {
      sendServerMessage("Unable to find client with ID " + argv[0] + ".");
      return;
    }
    if (l_area->owners().contains(l_owner_candidate->playerId()))
    {
      sendServerMessage("User is already a CM in this area.");
      return;
    }
    l_area->addOwner(l_owner_candidate->playerId());
    sendServerMessageArea(l_owner_candidate->name() + " is now CM in this area.");
    arup(kal::UpdateAreaPacket::CaseMaster, true);
  }
  else
  {
    sendServerMessage("You are already a CM in this area.");
  }
}

void AOClient::cmdUnCM(int argc, QStringList argv)
{
  AreaData *l_area = m_server->area(areaId());
  int l_uid;

  if (l_area->owners().isEmpty())
  {
    sendServerMessage("There are no CMs in this area.");
    return;
  }
  else if (argc == 0)
  {
    l_uid = playerId();
    sendServerMessage("You are no longer CM in this area.");
  }
  else if (checkPermission(ACLRole::UNCM) && argc >= 1)
  {
    bool conv_ok = false;
    l_uid = argv[0].toInt(&conv_ok);
    if (!conv_ok)
    {
      sendServerMessage("Invalid user ID.");
      return;
    }
    if (!l_area->owners().contains(l_uid))
    {
      sendServerMessage("That user is not CMed.");
      return;
    }
    AOClient *l_target = m_server->client(l_uid);
    if (l_target == nullptr)
    {
      sendServerMessage("No client with that ID found.");
      return;
    }
    sendServerMessage(l_target->name() + " was successfully unCMed.");
    l_target->sendServerMessage("You have been unCMed by a moderator.");
  }
  else
  {
    sendServerMessage("You do not have permission to unCM others.");
    return;
  }

  if (l_area->removeOwner(l_uid))
  {
    arup(kal::UpdateAreaPacket::Locked, true);
  }

  arup(kal::UpdateAreaPacket::CaseMaster, true);
}

void AOClient::cmdInvite(int argc, QStringList argv)
{
  Q_UNUSED(argc);

  AreaData *l_area = m_server->area(areaId());
  bool ok;
  int l_invited_id = argv[0].toInt(&ok);
  if (!ok)
  {
    sendServerMessage("That does not look like a valid ID.");
    return;
  }

  AOClient *target_client = m_server->client(l_invited_id);
  if (target_client == nullptr)
  {
    sendServerMessage("No client with that ID found.");
    return;
  }
  else if (!l_area->invite(l_invited_id))
  {
    sendServerMessage("That ID is already on the invite list.");
    return;
  }
  sendServerMessage("You invited ID " + argv[0]);
  target_client->sendServerMessage("You were invited and given access to " + l_area->name());
}

void AOClient::cmdUnInvite(int argc, QStringList argv)
{
  Q_UNUSED(argc);

  AreaData *l_area = m_server->area(areaId());
  bool ok;
  int l_uninvited_id = argv[0].toInt(&ok);
  if (!ok)
  {
    sendServerMessage("That does not look like a valid ID.");
    return;
  }

  AOClient *target_client = m_server->client(l_uninvited_id);
  if (target_client == nullptr)
  {
    sendServerMessage("No client with that ID found.");
    return;
  }
  else if (l_area->owners().contains(l_uninvited_id))
  {
    sendServerMessage("You cannot uninvite a CM!");
    return;
  }
  else if (!l_area->uninvite(l_uninvited_id))
  {
    sendServerMessage("That ID is not on the invite list.");
    return;
  }
  sendServerMessage("You uninvited ID " + argv[0]);
  target_client->sendServerMessage("You were uninvited from " + l_area->name());
}

void AOClient::cmdLock(int argc, QStringList argv)
{
  Q_UNUSED(argc);
  Q_UNUSED(argv);

  AreaData *area = m_server->area(areaId());
  if (area->lockStatus() == AreaData::LockStatus::LOCKED)
  {
    sendServerMessage("This area is already locked.");
    return;
  }
  sendServerMessageArea("This area is now locked.");
  area->lock();
  const QList<AOClient *> l_clients = m_server->clientList();
  for (AOClient *l_client : l_clients)
  {
    if (l_client->areaId() == areaId())
    {
      area->invite(l_client->playerId());
    }
  }
  arup(kal::UpdateAreaPacket::Locked, true);
}

void AOClient::cmdSpectatable(int argc, QStringList argv)
{
  Q_UNUSED(argc);
  Q_UNUSED(argv);

  AreaData *l_area = m_server->area(areaId());
  if (l_area->lockStatus() == AreaData::LockStatus::SPECTATABLE)
  {
    sendServerMessage("This area is already in spectate mode.");
    return;
  }
  sendServerMessageArea("This area is now spectatable.");
  l_area->spectatable();
  const QList<AOClient *> l_clients = m_server->clientList();
  for (AOClient *l_client : l_clients)
  {
    if (l_client->areaId() == areaId())
    {
      l_area->invite(l_client->playerId());
    }
  }
  arup(kal::UpdateAreaPacket::Locked, true);
}

void AOClient::cmdUnLock(int argc, QStringList argv)
{
  Q_UNUSED(argc);
  Q_UNUSED(argv);

  AreaData *l_area = m_server->area(areaId());
  if (l_area->lockStatus() == AreaData::LockStatus::FREE)
  {
    sendServerMessage("This area is not locked.");
    return;
  }
  sendServerMessageArea("This area is now unlocked.");
  l_area->unlock();
  arup(kal::UpdateAreaPacket::Locked, true);
}

void AOClient::cmdGetAreas(int argc, QStringList argv)
{
  Q_UNUSED(argc);
  Q_UNUSED(argv);

  QStringList details;
  details.append("\n== Currently Online: " + QString::number(m_server->playerCount()) + " ==");

  QList<AreaData *> area_list = m_server->areaList();
  for (int i = 0; i < area_list.length(); i++)
  {
    AreaData *area = area_list.at(i);
    if (area->playerCount() > 0)
    {
      details.append(buildAreaList(i));
    }
  }

  sendServerMessage(details.join("\n"));
}

void AOClient::cmdGetArea(int argc, QStringList argv)
{
  Q_UNUSED(argc);
  Q_UNUSED(argv);

  QStringList details = buildAreaList(areaId());
  sendServerMessage(details.join("\n"));
}

void AOClient::cmdArea(int argc, QStringList argv)
{
  Q_UNUSED(argc);

  bool ok;
  int l_new_area = argv[0].toInt(&ok);
  if (!ok || l_new_area >= m_server->getAreaCount() || l_new_area < 0)
  {
    sendServerMessage("That does not look like a valid area ID.");
    return;
  }
  changeArea(l_new_area);
}

void AOClient::cmdAreaKick(int argc, QStringList argv)
{
  Q_UNUSED(argc);

  AreaData *l_area = m_server->area(areaId());

  bool ok;
  int l_idx = argv[0].toInt(&ok);
  if (!ok)
  {
    sendServerMessage("That does not look like a valid ID.");
    return;
  }
  if (m_server->area(areaId())->owners().contains(l_idx))
  {
    sendServerMessage("You cannot kick another CM!");
    return;
  }
  AOClient *l_client_to_kick = m_server->client(l_idx);
  if (l_client_to_kick == nullptr)
  {
    sendServerMessage("No client with that ID found.");
    return;
  }
  else if (l_client_to_kick->areaId() != areaId())
  {
    sendServerMessage("That client is not in this area.");
    return;
  }
  l_client_to_kick->changeArea(0);
  l_area->uninvite(l_client_to_kick->playerId());

  sendServerMessage("Client " + argv[0] + " kicked back to area 0.");
}

void AOClient::cmdSetBackground(int argc, QStringList argv)
{
  Q_UNUSED(argc);

  QString f_background = argv.join(" ");
  AreaData *area = m_server->area(areaId());
  if (m_authenticated || !area->bgLocked())
  {
    if (m_server->getBackgrounds().contains(f_background, Qt::CaseInsensitive) || area->ignoreBgList() == true)
    {
      area->setBackground(f_background);
      m_server->broadcast(kal::BackgroundPacket(f_background, area->side()), areaId());
      QString ambience_name = ConfigManager::ambience()->value(f_background + "/ambience").toString();
      if (ambience_name != "")
      {
        kal::PlayTrackPacket packet;
        packet.track = ambience_name;
        packet.character_name = characterName();
        m_server->broadcast(packet, areaId());
      }
      else
      {
        kal::PlayTrackPacket packet;
        packet.track = ambience_name;
        packet.character_name = characterName();
        m_server->broadcast(packet, areaId());
      }
      sendServerMessageArea(character() + " changed the background to " + f_background);
    }
    else
    {
      sendServerMessage("Invalid background name.");
    }
  }
  else
  {
    sendServerMessage("This area's background is locked.");
  }
}

void AOClient::cmdSetSide(int argc, QStringList argv)
{
  Q_UNUSED(argc);

  AreaData *area = m_server->area(areaId());
  if (area->bgLocked())
  {
    sendServerMessage("This area's background is locked.");
    return;
  }

  QString side = argv.join(" ");
  area->setSide(side);
  m_server->broadcast(kal::BackgroundPacket(area->background(), area->side()), areaId());
  if (side.isEmpty())
  {
    sendServerMessageArea(character() + " unlocked the background side");
  }
  else
  {
    sendServerMessageArea(character() + " locked the background side to " + side);
  }
}

void AOClient::cmdBgLock(int argc, QStringList argv)
{
  Q_UNUSED(argc);
  Q_UNUSED(argv);

  AreaData *l_area = m_server->area(areaId());

  if (l_area->bgLocked() == false)
  {
    l_area->toggleBgLock();
  };

  m_server->broadcast(kal::ChatPacket(ConfigManager::serverTag(), character() + " locked the background.", true), areaId());
}

void AOClient::cmdBgUnlock(int argc, QStringList argv)
{
  Q_UNUSED(argc);
  Q_UNUSED(argv);

  AreaData *l_area = m_server->area(areaId());

  if (l_area->bgLocked() == true)
  {
    l_area->toggleBgLock();
  };

  m_server->broadcast(kal::ChatPacket(ConfigManager::serverTag(), character() + " unlocked the background.", true), areaId());
}

void AOClient::cmdStatus(int argc, QStringList argv)
{
  Q_UNUSED(argc);

  AreaData *l_area = m_server->area(areaId());
  QString l_arg = argv[0].toLower();

  if (l_area->changeStatus(l_arg))
  {
    arup(kal::UpdateAreaPacket::Status, true);
    m_server->broadcast(kal::ChatPacket(ConfigManager::serverTag(), character() + " changed status to " + l_arg.toUpper(), true), areaId());
  }
  else
  {
    const QStringList keys = AreaData::map_statuses.keys();
    sendServerMessage("That does not look like a valid status. Valid statuses are " + keys.join(", "));
  }
}

void AOClient::cmdJudgeLog(int argc, QStringList argv)
{
  Q_UNUSED(argc);
  Q_UNUSED(argv);

  AreaData *l_area = m_server->area(areaId());
  if (l_area->judgelog().isEmpty())
  {
    sendServerMessage("There have been no judge actions in this area.");
    return;
  }
  QString l_message = l_area->judgelog().join("\n");
  // Judgelog contains an IPID, so we shouldn't send that unless the caller has appropriate permissions
  if (checkPermission(ACLRole::KICK) || checkPermission(ACLRole::BAN))
  {
    sendServerMessage(l_message);
  }
  else
  {
    QString filteredmessage = l_message.remove(QRegularExpression("[(].*[)]")); // Filter out anything between two parentheses. This should only ever be the IPID
    sendServerMessage(filteredmessage);
  }
}

void AOClient::cmdIgnoreBgList(int argc, QStringList argv)
{
  Q_UNUSED(argc);
  Q_UNUSED(argv);

  AreaData *l_area = m_server->area(areaId());
  l_area->toggleIgnoreBgList();
  QString l_state = l_area->ignoreBgList() ? "ignored." : "enforced.";
  sendServerMessage("BG list in this area is now " + l_state);
}

void AOClient::cmdAreaMessage(int argc, QStringList argv)
{
  AreaData *l_area = m_server->area(areaId());
  if (argc == 0)
  {
    sendServerMessage(l_area->areaMessage());
    return;
  }

  if (argc >= 1)
  {
    l_area->changeAreaMessage(argv.join(" "));
    sendServerMessage("Updated this area's message.");
  }
}

void AOClient::cmdClearAreaMessage(int argc, QStringList argv)
{
  Q_UNUSED(argc);
  Q_UNUSED(argv);

  AreaData *l_area = m_server->area(areaId());
  l_area->clearAreaMessage();
  if (l_area->sendAreaMessageOnJoin()) // Turn off the automatic sending.
  {
    cmdToggleAreaMessageOnJoin(0, QStringList{}); // Dummy values.
  }
}

void AOClient::cmdToggleAreaMessageOnJoin(int argc, QStringList argv)
{
  Q_UNUSED(argc);
  Q_UNUSED(argv);

  AreaData *l_area = m_server->area(areaId());
  l_area->toggleAreaMessageJoin();
  QString l_state = l_area->sendAreaMessageOnJoin() ? "enabled." : "disabled.";
  sendServerMessage("Sending message on area join is now " + l_state);
}

void AOClient::cmdToggleWtce(int argc, QStringList argv)
{
  Q_UNUSED(argc);
  Q_UNUSED(argv);

  AreaData *l_area = m_server->area(areaId());
  l_area->toggleWtceAllowed();
  QString l_state = l_area->isWtceAllowed() ? "enabled." : "disabled.";
  sendServerMessage("Using testimony animations is now " + l_state);
}

void AOClient::cmdToggleShouts(int argc, QStringList argv)
{
  Q_UNUSED(argc);
  Q_UNUSED(argv);

  AreaData *l_area = m_server->area(areaId());
  l_area->toggleShoutAllowed();
  QString l_state = l_area->isShoutAllowed() ? "enabled." : "disabled.";
  sendServerMessage("Using shouts is now " + l_state);
}

void AOClient::cmdWebfiles(int argc, QStringList argv)
{
  const QList<AOClient *> l_clients = m_server->clientList();
  QStringList l_weblinks;
  for (AOClient *l_client : l_clients)
  {
    if (l_client->m_current_iniswap.isEmpty() || l_client->areaId() != areaId())
    {
      continue;
    }

    if (l_client->character().toLower() != l_client->m_current_iniswap.toLower())
    {
      l_weblinks.append("https://attorneyonline.github.io/webDownloader/index.html?char=" + l_client->m_current_iniswap);
    }
  }
  sendServerMessage("Character files:\n" + l_weblinks.join("\n"));
}
