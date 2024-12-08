#include "aoclient.h"

#include "area_data.h"
#include "config_manager.h"
#include "db_manager.h"
#include "music_manager.h"
#include "network/packet/evidence_packets.h"
#include "network/packet/login_packets.h"
#include "server.h"

#include <QQueue>

void AOClient::sendEvidenceList(AreaData *area) const
{
  const QList<AOClient *> l_clients = m_server->clientList();
  for (AOClient *l_client : l_clients)
  {
    if (l_client->areaId() == areaId())
    {
      l_client->updateEvidenceList(area);
    }
  }
}

void AOClient::updateEvidenceList(AreaData *area)
{
  QList<kal::Evidence> l_evidence_list;
  const QList<kal::Evidence> l_area_evidence = area->evidence();
  for (const kal::Evidence &evidence : l_area_evidence)
  {
    if (!checkPermission(ACLRole::CM) && area->eviMod() == AreaData::EvidenceMod::HIDDEN_CM)
    {
      QRegularExpression l_regex("<owner=(.*?)>");
      QRegularExpressionMatch l_match = l_regex.match(evidence.description);
      if (l_match.hasMatch())
      {
        QStringList owners = l_match.captured(1).split(",");
        if (!owners.contains("all", Qt::CaseSensitivity::CaseInsensitive) && !owners.contains(m_pos, Qt::CaseSensitivity::CaseInsensitive))
        {
          continue;
        }
      }
      // no match = show it to all
    }
    l_evidence_list.append(evidence);
  }

  shipPacket(kal::EvidenceListPacket{l_evidence_list});
}

QString AOClient::dezalgo(QString p_text)
{
  QRegularExpression rxp("([̴̵̶̷̸̡̢̧̨̛̖̗̘̙̜̝̞̟̠̣̤̥̦̩̪̫̬̭̮̯̰̱̲̳̹̺̻̼͇͈͉͍͎̀́̂̃̄̅̆̇̈̉̊̋̌̍̎̏̐̑̒̓̔̽̾̿̀́͂̓̈́͆͊͋͌̕̚ͅ͏͓͔͕͖͙͚͐͑͒͗͛ͣͤͥͦͧͨͩͪͫͬͭͮͯ͘͜͟͢͝͞͠͡])");
  QString filtered = p_text.replace(rxp, "");
  return filtered;
}

bool AOClient::checkEvidenceAccess(AreaData *area)
{
  switch (area->eviMod())
  {
  case AreaData::EvidenceMod::FFA:
    return true;
  case AreaData::EvidenceMod::CM:
  case AreaData::EvidenceMod::HIDDEN_CM:
    return checkPermission(ACLRole::CM);
  case AreaData::EvidenceMod::MOD:
    return m_authenticated;
  default:
    return false;
  }
}

void AOClient::loginAttempt(QString message)
{
  switch (ConfigManager::authType())
  {
  case DataTypes::AuthType::SIMPLE:
    if (message == ConfigManager::modpass())
    {
      shipPacket(kal::LoginPacket{kal::LoginPacket::Successful});
      m_authenticated = true;
      m_acl_role_id = ACLRolesHandler::SUPER_ID;
    }
    else
    {
      shipPacket(kal::LoginPacket{kal::LoginPacket::Unsuccessful}); // Client: "Login unsuccessful."
      sendServerMessage("Incorrect password.");
    }
    Q_EMIT logLogin((character() + " " + characterName()), name(), "Moderator", m_ipid, m_server->area(areaId())->name(), m_authenticated);
    break;
  case DataTypes::AuthType::ADVANCED:
    QStringList l_login = message.split(" ");
    if (l_login.size() < 2)
    {
      sendServerMessage("You must specify a username and a password");
      sendServerMessage("Exiting login prompt.");
      m_is_logging_in = false;
      return;
    }
    QString username = l_login[0];
    QString password = l_login[1];
    if (m_server->getDatabaseManager()->authenticate(username, password))
    {
      m_authenticated = true;
      m_acl_role_id = m_server->getDatabaseManager()->getACL(username);
      m_moderator_name = username;
      shipPacket(kal::LoginPacket{kal::LoginPacket::Successful});
      sendServerMessage("Welcome, " + username);
    }
    else
    {
      shipPacket(kal::LoginPacket{kal::LoginPacket::Unsuccessful});
      sendServerMessage("Incorrect password.");
    }
    Q_EMIT logLogin((character() + " " + characterName()), name(), username, m_ipid, m_server->area(areaId())->name(), m_authenticated);
    break;
  }
  sendServerMessage("Exiting login prompt.");
  m_is_logging_in = false;
  return;
}

QString AOClient::decodeMessage(QString incoming_message)
{
  QString decoded_message = incoming_message.replace("<num>", "#").replace("<percent>", "%").replace("<dollar>", "$").replace("<and>", "&");
  return decoded_message;
}

void AOClient::updateJudgeLog(AreaData *area, AOClient *client, QString action)
{
  QString l_timestamp = QTime::currentTime().toString("hh:mm:ss");
  QString l_uid = QString::number(client->playerId());
  QString l_char_name = client->character();
  QString l_ipid = client->getIpid();
  QString l_message = action;
  QString l_logmessage = QString("[%1]: [%2] %3 (%4) %5").arg(l_timestamp, l_uid, l_char_name, l_ipid, l_message);
  area->appendJudgelog(l_logmessage);
}
