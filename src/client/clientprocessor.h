#pragma once

#include "area_data.h"
#include "network/cargohandler.h"

#include <QString>

namespace kal
{
class ClientProcessor : public kal::CargoHandler
{
public:
  enum Error
  {
    NoError,
    InvalidPacketError,
  };

  void setArea(AreaData *area);

  Error error() const;
  QString errorString() const;

protected:
  AreaData *area;

  void setError(Error error, const QString &what = QString());

private:
  Error m_error = NoError;
  QString m_what;
};
} // namespace kal
