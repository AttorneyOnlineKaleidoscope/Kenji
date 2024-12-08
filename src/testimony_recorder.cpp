#include "aoclient.h"

#include "area_data.h"
#include "config_manager.h"
#include "server.h"

void AOClient::addStatement(QStringList packet)
{
  if (checkTestimonySymbols(packet[4]))
  {
    return;
  }
  AreaData *area = m_server->area(areaId());
  int c_statement = area->statement();
  if (c_statement >= -1)
  {
    if (area->testimonyRecording() == AreaData::TestimonyRecording::RECORDING)
    {
      if (c_statement <= ConfigManager::maxStatements())
      {
        if (c_statement == -1)
        {
          packet[14] = "3";
        }
        else
        {
          packet[14] = "1";
        }
        area->recordStatement(packet);
        return;
      }
      else
      {
        sendServerMessage("Unable to add more statements. The maximum amount of statements has been reached.");
      }
    }
    else if (area->testimonyRecording() == AreaData::TestimonyRecording::ADD)
    {
      packet[14] = "1";
      area->addStatement(c_statement + 1, packet);
      area->setTestimonyRecording(AreaData::TestimonyRecording::PLAYBACK);
    }
    else
    {
      sendServerMessage("Unable to add more statements. The maximum amount of statements has been reached.");
      area->setTestimonyRecording(AreaData::TestimonyRecording::PLAYBACK);
    }
  }
}

QStringList AOClient::updateStatement(QStringList packet)
{
  if (checkTestimonySymbols(packet[4]))
  {
    return packet;
  }
  AreaData *area = m_server->area(areaId());
  int c_statement = area->statement();
  area->setTestimonyRecording(AreaData::TestimonyRecording::PLAYBACK);
  if (c_statement <= 0 || area->testimony()[c_statement].empty())
  {
    sendServerMessage("Unable to update an empty statement. Please use /addtestimony.");
  }
  else
  {
    packet[14] = "1";
    area->replaceStatement(c_statement, packet);
    sendServerMessage("Updated current statement.");
    return area->testimony()[c_statement];
  }
  return packet;
}

void AOClient::clearTestimony()
{
  AreaData *area = m_server->area(areaId());
  area->clearTestimony();
}

bool AOClient::checkTestimonySymbols(const QString &message)
{
  if (message.contains('>') || message.contains('<'))
  {
    sendServerMessage("Unable to add statements containing '>' or '<'.");
    return true;
  }
  return false;
}
