#pragma once

#include "network/aopacket.h"

class PacketMS : public AOPacket
{
public:
  PacketMS(QStringList &contents);
  virtual PacketInfo getPacketInfo() const;
  virtual void handlePacket(AreaData *area, AOClient &client) const;

private:
  AOPacket *validateIcPacket(AOClient &client) const;
  QRegularExpressionMatch isTestimonyJumpCommand(QString message) const;
};
