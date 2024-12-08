#include "clienthandshakeprocessor.h"

#include "aoclient.h"
#include "config_manager.h"
#include "db_manager.h"
#include "server.h"
#include "turnaboutversion.h"

#include "network/packet/area_packets.h"
#include "network/packet/background_packets.h"
#include "network/packet/case_packets.h"
#include "network/packet/character_packets.h"
#include "network/packet/chat_packets.h"
#include "network/packet/evidence_packets.h"
#include "network/packet/handshake_packets.h"
#include "network/packet/judge_packets.h"
#include "network/packet/login_packets.h"
#include "network/packet/message_packets.h"
#include "network/packet/moderation_packets.h"
#include "network/packet/music_packets.h"
#include "network/packet/player_packets.h"
#include "network/packet/splash_packets.h"
#include "network/packet/system_packets.h"
#include "network/packet/timer_packets.h"

kal::ClientHandshakeProcessor::ClientHandshakeProcessor(AOClient &client)
    : client(client)
{
  setError(InvalidPacketError);
}

void kal::ClientHandshakeProcessor::cargo(HelloPacket &cargo)
{
  setError(NoError);

  if (!ConfigManager::webaoEnabled() && cargo.software_name == "webAO")
  {
    client.shipPacket(kal::ModerationNotice("WebAO is disabled on this server."));
    client.m_socket->close();
    return;
  }

  if (cargo.protocol_version != kal::Turnabout::softwareVersion())
  {
    client.shipPacket(kal::ModerationNotice("Unsupported protocol version."));
    client.m_socket->close();
    return;
  }

  QString incoming_hwid = cargo.hwid;
  if (incoming_hwid.isEmpty() || !client.m_hwid.isEmpty())
  {
    // No double sending or empty HWIDs!
    client.shipPacket(kal::ModerationNotice("A protocol error has been encountered."));
    client.m_socket->close();
    return;
  }

  client.m_hwid = incoming_hwid;
  Q_EMIT client.getServer()->logConnectionAttempt(client.m_remote_ip.toString(), client.m_ipid, client.m_hwid);
  auto ban = client.getServer()->getDatabaseManager()->isHDIDBanned(client.m_hwid);
  if (ban.first)
  {
    kal::ModerationNotice packet;
    packet.reason = ban.second.reason;
    packet.duration = ban.second.duration;
    packet.id = ban.second.id;
    client.shipPacket(packet);
    client.m_socket->close();
    return;
  }

  client.finishHandshake();
  client.shipPacket(kal::WelcomePacket(client.playerId(), ConfigManager::assetUrl()));

  client.shipPacket(kal::CharacterListPacket(client.getServer()->getCharacters()));
  client.shipPacket(kal::MusicListPacket(client.getServer()->getMusicList()));

  area = client.getServer()->area(client.areaId());
  client.getServer()->updateCharsTaken(area);
  client.sendEvidenceList(area);

  client.shipPacket(kal::HealthStatePacket(kal::DefenseHealth, area->defHP()));
  client.shipPacket(kal::HealthStatePacket(kal::ProsecutionHealth, area->proHP()));
  client.shipPacket(kal::AreaListPacket(client.getServer()->getAreaNames()));
  // Here lies OPPASS, the genius of FanatSors who send the modpass to everyone in plain text.
  client.shipPacket(kal::BackgroundPacket(area->background(), area->side()));

  client.sendServerMessage("=== MOTD ===\r\n" + ConfigManager::motd() + "\r\n=============");

  client.fullArup(); // Give client all the area data
  if (client.getServer()->timer->isActive())
  {
    client.shipPacket(kal::SetTimerStatePacket(0, kal::SetTimerStatePacket::ShowTimer));
    client.shipPacket(kal::SetTimerStatePacket(0, kal::SetTimerStatePacket::StartTimer, QTime(0, 0).msecsTo(QTime(0, 0).addMSecs(client.getServer()->timer->remainingTime()))));
  }
  else
  {
    client.shipPacket(kal::SetTimerStatePacket(0, kal::SetTimerStatePacket::HideTimer));
  }
  const QList<QTimer *> l_timers = area->timers();
  for (QTimer *l_timer : l_timers)
  {
    int l_timer_id = area->timers().indexOf(l_timer) + 1;
    if (l_timer->isActive())
    {
      client.shipPacket(kal::SetTimerStatePacket(l_timer_id, kal::SetTimerStatePacket::ShowTimer));
      client.shipPacket(kal::SetTimerStatePacket(l_timer_id, kal::SetTimerStatePacket::StartTimer, QTime(0, 0).msecsTo(QTime(0, 0).addMSecs(l_timer->remainingTime()))));
    }
    else
    {
      client.shipPacket(kal::SetTimerStatePacket(l_timer_id, kal::SetTimerStatePacket::HideTimer));
    }
  }

  area->addClient(-1, client.playerId());
  client.arup(kal::UpdateAreaPacket::PlayerCount, true); // Tell everyone there is a new player
}
