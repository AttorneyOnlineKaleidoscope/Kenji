#pragma once

#include "clientprocessor.h"

class AOClient;

namespace kal
{
class ClientHandshakeProcessor : public ClientProcessor
{
public:
  ClientHandshakeProcessor(AOClient &client);

  // handshake
  void cargo(HelloPacket &cargo) override;

private:
  AOClient &client;
};
} // namespace kal
