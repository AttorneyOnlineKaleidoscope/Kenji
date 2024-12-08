#include "clientprocessor.h"

void kal::ClientProcessor::setArea(AreaData *area)
{
  this->area = area;
}

kal::ClientProcessor::Error kal::ClientProcessor::error() const
{
  return m_error;
}

QString kal::ClientProcessor::errorString() const
{
  QString message;
  switch (m_error)
  {
  case NoError:
    message = QStringLiteral("No error");
    break;

  case InvalidPacketError:
    message = QStringLiteral("Invalid packet");
    break;
  }

  if (!m_what.isEmpty())
  {
    message += QStringLiteral(": ") + m_what;
  }

  return message;
}

void kal::ClientProcessor::setError(Error error, const QString &what)
{
  m_error = error;
  m_what = what;
}
