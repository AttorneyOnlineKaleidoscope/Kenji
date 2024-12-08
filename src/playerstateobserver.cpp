#include "playerstateobserver.h"

#include "network/packet/player_packets.h"

PlayerStateObserver::PlayerStateObserver(QObject *parent)
    : QObject{parent}
{}

PlayerStateObserver::~PlayerStateObserver()
{}

void PlayerStateObserver::registerClient(AOClient *client)
{
  Q_ASSERT(!m_client_list.contains(client));

  kal::RegisterPlayerPacket packet(client->playerId(), kal::RegisterPlayerPacket::Add);
  sendToClientList(packet);

  m_client_list.append(client);

  connect(client, &AOClient::nameChanged, this, &PlayerStateObserver::notifyNameChanged);
  connect(client, &AOClient::characterChanged, this, &PlayerStateObserver::notifyCharacterChanged);
  connect(client, &AOClient::characterNameChanged, this, &PlayerStateObserver::notifyCharacterNameChanged);
  connect(client, &AOClient::areaIdChanged, this, &PlayerStateObserver::notifyAreaIdChanged);

  QList<kal::Packet> packets;
  for (AOClient *i_client : qAsConst(m_client_list))
  {
    kal::PlayerId id = i_client->playerId();
    packets.append(kal::RegisterPlayerPacket(id, kal::RegisterPlayerPacket::Add));
    packets.append(kal::UpdatePlayerPacket(id, kal::UpdatePlayerPacket::Name, i_client->name()));
    packets.append(kal::UpdatePlayerPacket(id, kal::UpdatePlayerPacket::Character, i_client->character()));
    packets.append(kal::UpdatePlayerPacket(id, kal::UpdatePlayerPacket::CharacterName, i_client->characterName()));
    packets.append(kal::UpdatePlayerPacket(id, kal::UpdatePlayerPacket::AreaId, i_client->areaId()));
  }

  for (const kal::Packet &packet : qAsConst(packets))
  {
    client->shipPacket(packet);
  }
}

void PlayerStateObserver::unregisterClient(AOClient *client)
{
  Q_ASSERT(m_client_list.contains(client));

  disconnect(client, nullptr, this, nullptr);

  m_client_list.removeAll(client);

  kal::RegisterPlayerPacket packet(client->playerId(), kal::RegisterPlayerPacket::Remove);
  sendToClientList(packet);
}

void PlayerStateObserver::sendToClientList(const kal::Packet &packet)
{
  for (AOClient *client : qAsConst(m_client_list))
  {
    client->shipPacket(packet);
  }
}

void PlayerStateObserver::notifyNameChanged(const QString &name)
{
  sendToClientList(kal::UpdatePlayerPacket(qobject_cast<AOClient *>(sender())->playerId(), kal::UpdatePlayerPacket::Name, name));
}

void PlayerStateObserver::notifyCharacterChanged(const QString &character)
{
  sendToClientList(kal::UpdatePlayerPacket(qobject_cast<AOClient *>(sender())->playerId(), kal::UpdatePlayerPacket::Character, character));
}

void PlayerStateObserver::notifyCharacterNameChanged(const QString &characterName)
{
  sendToClientList(kal::UpdatePlayerPacket(qobject_cast<AOClient *>(sender())->playerId(), kal::UpdatePlayerPacket::CharacterName, characterName));
}

void PlayerStateObserver::notifyAreaIdChanged(int areaId)
{
  sendToClientList(kal::UpdatePlayerPacket(qobject_cast<AOClient *>(sender())->playerId(), kal::UpdatePlayerPacket::AreaId, areaId));
}
