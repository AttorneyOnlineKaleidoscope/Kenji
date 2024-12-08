#include "aoclient.h"

#include "area_data.h"
#include "config_manager.h"
#include "network/packet/character_packets.h"
#include "network/packet/chat_packets.h"
#include "network/packet/handshake_packets.h"
#include "server.h"

// This file is for commands under the messaging category in aoclient.h
// Be sure to register the command in the header before adding it here!

void AOClient::cmdPos(int argc, QStringList argv)
{
  Q_UNUSED(argc);

  changePosition(argv[0]);
  updateEvidenceList(m_server->area(areaId()));
}

void AOClient::cmdForcePos(int argc, QStringList argv)
{
  Q_UNUSED(argc);

  bool ok;
  QList<AOClient *> l_targets;
  int l_target_id = argv[1].toInt(&ok);
  int l_forced_clients = 0;
  if (!ok && argv[1] != "*")
  {
    sendServerMessage("That does not look like a valid ID.");
    return;
  }
  else if (ok)
  {
    AOClient *l_target_client = m_server->client(l_target_id);
    if (l_target_client != nullptr)
    {
      l_targets.append(l_target_client);
    }
    else
    {
      sendServerMessage("Target ID not found!");
      return;
    }
  }

  else if (argv[1] == "*")
  { // force all clients in the area
    const QList<AOClient *> l_clients = m_server->clientList();
    for (AOClient *l_client : l_clients)
    {
      if (l_client->areaId() == areaId())
      {
        l_targets.append(l_client);
      }
    }
  }
  for (AOClient *l_target : l_targets)
  {
    l_target->sendServerMessage("Position forcibly changed by CM.");
    l_target->changePosition(argv[0]);
    l_forced_clients++;
  }
  sendServerMessage("Forced " + QString::number(l_forced_clients) + " into pos " + argv[0] + ".");
}

void AOClient::cmdSwitch(int argc, QStringList argv)
{
  Q_UNUSED(argc);

  int l_selected_char_id = m_server->getCharID(argv.join(" "));
  if (l_selected_char_id == -1)
  {
    sendServerMessage("That does not look like a valid character.");
    return;
  }
  if (changeCharacter(l_selected_char_id))
  {
    m_char_id = l_selected_char_id;
  }
  else
  {
    sendServerMessage("The character you picked is either taken or invalid.");
  }
}

void AOClient::cmdRandomChar(int argc, QStringList argv)
{
  Q_UNUSED(argc);
  Q_UNUSED(argv);

  AreaData *l_area = m_server->area(areaId());
  int l_selected_char_id;
  bool l_taken = true;
  while (l_taken)
  {
    l_selected_char_id = genRand(0, m_server->getCharacterCount() - 1);
    if (!l_area->charactersTaken().contains(l_selected_char_id))
    {
      l_taken = false;
    }
  }
  if (changeCharacter(l_selected_char_id))
  {
    m_char_id = l_selected_char_id;
  }
}

void AOClient::cmdG(int argc, QStringList argv)
{
  Q_UNUSED(argc);

  QString l_sender_name = name();
  QString l_sender_area = m_server->getAreaName(areaId());
  QString l_sender_message = argv.join(" ");
  // Better readability thanks to AwesomeAim.
  kal::ChatPacket l_mod_packet("[G][" + m_ipid + "][" + l_sender_area + "]" + l_sender_name, l_sender_message);
  kal::ChatPacket l_user_packet("[G][" + l_sender_area + "]" + l_sender_name, l_sender_message);
  m_server->broadcast(l_user_packet, l_mod_packet, Server::TARGET_TYPE::AUTHENTICATED);
  return;
}

void AOClient::cmdToggleGlobal(int argc, QStringList argv)
{
  Q_UNUSED(argc);
  Q_UNUSED(argv);

  m_global_enabled = !m_global_enabled;
  QString l_str_en = m_global_enabled ? "shown" : "hidden";
  sendServerMessage("Global chat set to " + l_str_en);
}

void AOClient::cmdPM(int argc, QStringList argv)
{
  Q_UNUSED(argc);

  bool ok;
  int l_target_id = argv.takeFirst().toInt(&ok); // using takeFirst removes the ID from our list of arguments...
  if (!ok)
  {
    sendServerMessage("That does not look like a valid ID.");
    return;
  }
  AOClient *l_target_client = m_server->client(l_target_id);
  if (l_target_client == nullptr)
  {
    sendServerMessage("No client with that ID found.");
    return;
  }
  if (l_target_client->m_pm_mute)
  {
    sendServerMessage("That user is not recieving PMs.");
    return;
  }
  QString l_message = argv.join(" "); //...which means it will not end up as part of the message
  l_target_client->sendServerMessage("Message from " + name() + " (" + QString::number(playerId()) + "): " + l_message);
  sendServerMessage("PM sent to " + QString::number(l_target_id) + ". Message: " + l_message);
}

void AOClient::cmdNeed(int argc, QStringList argv)
{
  Q_UNUSED(argc);

  QString l_sender_area = m_server->getAreaName(areaId());
  QString l_sender_message = argv.join(" ");
  m_server->broadcast(kal::ChatPacket(ConfigManager::serverTag(), "=== Advert ===\n[" + l_sender_area + "] needs " + l_sender_message + "."), Server::TARGET_TYPE::ADVERT);
}

void AOClient::cmdAnnounce(int argc, QStringList argv)
{
  Q_UNUSED(argc);

  sendServerBroadcast("=== Announcement ===\r\n" + argv.join(" ") + "\r\n=============");
}

void AOClient::cmdM(int argc, QStringList argv)
{
  Q_UNUSED(argc);

  QString l_sender_name = name();
  QString l_sender_message = argv.join(" ");
  m_server->broadcast(kal::ChatPacket("[M]" + l_sender_name, l_sender_message), Server::TARGET_TYPE::MODCHAT);
}

void AOClient::cmdGM(int argc, QStringList argv)
{
  Q_UNUSED(argc);

  QString l_sender_name = name();
  QString l_sender_area = m_server->getAreaName(areaId());
  QString l_sender_message = argv.join(" ");
  m_server->broadcast(kal::ChatPacket("[G][" + l_sender_area + "]" + "[" + l_sender_name + "][M]", l_sender_message), Server::TARGET_TYPE::MODCHAT);
}

void AOClient::cmdLM(int argc, QStringList argv)
{
  Q_UNUSED(argc);

  QString l_sender_name = name();
  QString l_sender_message = argv.join(" ");
  m_server->broadcast(kal::ChatPacket("[" + l_sender_name + "][M]", l_sender_message), areaId());
}

void AOClient::cmdGimp(int argc, QStringList argv)
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

  if (l_target->m_is_gimped)
  {
    sendServerMessage("That player is already gimped!");
  }
  else
  {
    sendServerMessage("Gimped player.");
    l_target->sendServerMessage("You have been gimped! " + getReprimand());
  }
  l_target->m_is_gimped = true;
}

void AOClient::cmdUnGimp(int argc, QStringList argv)
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

  if (!(l_target->m_is_gimped))
  {
    sendServerMessage("That player is not gimped!");
  }
  else
  {
    sendServerMessage("Ungimped player.");
    l_target->sendServerMessage("A moderator has ungimped you! " + getReprimand(true));
  }
  l_target->m_is_gimped = false;
}

void AOClient::cmdDisemvowel(int argc, QStringList argv)
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

  if (l_target->m_is_disemvoweled)
  {
    sendServerMessage("That player is already disemvoweled!");
  }
  else
  {
    sendServerMessage("Disemvoweled player.");
    l_target->sendServerMessage("You have been disemvoweled! " + getReprimand());
  }
  l_target->m_is_disemvoweled = true;
}

void AOClient::cmdUnDisemvowel(int argc, QStringList argv)
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

  if (!(l_target->m_is_disemvoweled))
  {
    sendServerMessage("That player is not disemvoweled!");
  }
  else
  {
    sendServerMessage("Undisemvoweled player.");
    l_target->sendServerMessage("A moderator has undisemvoweled you! " + getReprimand(true));
  }
  l_target->m_is_disemvoweled = false;
}

void AOClient::cmdShake(int argc, QStringList argv)
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

  if (l_target->m_is_shaken)
  {
    sendServerMessage("That player is already shaken!");
  }
  else
  {
    sendServerMessage("Shook player.");
    l_target->sendServerMessage("A moderator has shaken your words! " + getReprimand());
  }
  l_target->m_is_shaken = true;
}

void AOClient::cmdUnShake(int argc, QStringList argv)
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

  if (!(l_target->m_is_shaken))
  {
    sendServerMessage("That player is not shaken!");
  }
  else
  {
    sendServerMessage("Unshook player.");
    l_target->sendServerMessage("A moderator has unshook you! " + getReprimand(true));
  }
  l_target->m_is_shaken = false;
}

void AOClient::cmdMutePM(int argc, QStringList argv)
{
  Q_UNUSED(argc);
  Q_UNUSED(argv);

  m_pm_mute = !m_pm_mute;
  QString l_str_en = m_pm_mute ? "muted" : "unmuted";
  sendServerMessage("PM's are now " + l_str_en);
}

void AOClient::cmdToggleAdverts(int argc, QStringList argv)
{
  Q_UNUSED(argc);
  Q_UNUSED(argv);

  m_advert_enabled = !m_advert_enabled;
  QString l_str_en = m_advert_enabled ? "on" : "off";
  sendServerMessage("Advertisements turned " + l_str_en);
}

void AOClient::cmdAfk(int argc, QStringList argv)
{
  Q_UNUSED(argc);
  Q_UNUSED(argv);

  m_is_afk = true;
  sendServerMessage("You are now AFK.");
}

void AOClient::cmdCharCurse(int argc, QStringList argv)
{
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

  if (l_target->m_is_charcursed)
  {
    sendServerMessage("That player is already charcursed!");
    return;
  }

  if (argc == 1)
  {
    l_target->m_charcurse_list.append(m_server->getCharID(l_target->character()));
  }
  else
  {
    argv.removeFirst();
    QStringList l_char_names = argv.join(" ").split(",");

    l_target->m_charcurse_list.clear();
    for (const QString &l_char_name : qAsConst(l_char_names))
    {
      int char_id = m_server->getCharID(l_char_name);
      if (char_id == -1)
      {
        sendServerMessage("Could not find character: " + l_char_name);
        return;
      }
      l_target->m_charcurse_list.append(char_id);
    }
  }

  l_target->m_is_charcursed = true;

  // Kick back to char select screen
  if (!l_target->m_charcurse_list.contains(m_server->getCharID(l_target->character())))
  {
    l_target->changeCharacter(-1);
    m_server->updateCharsTaken(m_server->area(areaId()));
    l_target->shipPacket(kal::WelcomePacket());
  }
  else
  {
    m_server->updateCharsTaken(m_server->area(areaId()));
  }

  l_target->sendServerMessage("You have been charcursed!");
  sendServerMessage("Charcursed player.");
}

void AOClient::cmdUnCharCurse(int argc, QStringList argv)
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

  if (!l_target->m_is_charcursed)
  {
    sendServerMessage("That player is not charcursed!");
    return;
  }
  l_target->m_is_charcursed = false;
  l_target->m_charcurse_list.clear();
  m_server->updateCharsTaken(m_server->area(areaId()));
  sendServerMessage("Uncharcursed player.");
  l_target->sendServerMessage("You were uncharcursed.");
}

void AOClient::cmdCharSelect(int argc, QStringList argv)
{
  Q_UNUSED(argc);
  Q_UNUSED(argv);

  changeCharacter(-1);
  shipPacket(kal::WelcomePacket());
}

void AOClient::cmdForceCharSelect(int argc, QStringList argv)
{
  Q_UNUSED(argc);

  bool ok = false;
  int l_target_id = argv[0].toInt(&ok);
  if (!ok)
  {
    sendServerMessage("This ID does not look valid. Please use the client ID.");
    return;
  }

  AOClient *l_target = m_server->client(l_target_id);

  if (l_target == nullptr)
  {
    sendServerMessage("Unable to locate client with ID " + QString::number(l_target_id) + ".");
    return;
  }

  l_target->changeCharacter(-1);
  l_target->shipPacket(kal::WelcomePacket());
  sendServerMessage("Client has been forced into character select!");
}

void AOClient::cmdA(int argc, QStringList argv)
{
  Q_UNUSED(argc);

  bool ok;
  int l_area_id = argv[0].toInt(&ok);
  if (!ok)
  {
    sendServerMessage("This does not look like a valid AreaID.");
    return;
  }

  AreaData *l_area = m_server->area(l_area_id);
  if (!l_area->owners().contains(playerId()))
  {
    sendServerMessage("You are not CM in that area.");
    return;
  }

  argv.removeAt(0);
  QString l_sender_name = name();
  QString l_ooc_message = argv.join(" ");
  m_server->broadcast(kal::ChatPacket{"[CM]" + l_sender_name, l_ooc_message}, l_area_id);
}

void AOClient::cmdS(int argc, QStringList argv)
{
  Q_UNUSED(argc);

  int l_all_areas = m_server->getAreaCount() - 1;
  QString l_sender_name = name();
  QString l_ooc_message = argv.join(" ");

  for (int i = 0; i <= l_all_areas; i++)
  {
    if (m_server->area(i)->owners().contains(playerId()))
    {
      m_server->broadcast(kal::ChatPacket{"[CM]" + l_sender_name, l_ooc_message}, i);
    }
  }
}

void AOClient::cmdFirstPerson(int argc, QStringList argv)
{
  Q_UNUSED(argc);
  Q_UNUSED(argv);

  m_first_person = !m_first_person;
  QString l_str_en = m_first_person ? "enabled" : "disabled";
  sendServerMessage("First person mode " + l_str_en + ".");
}
