#include "ipidutils.h"

#include <QCryptographicHash>

QString calculate_ipid(const QHostAddress &address)
{
  // TODO: add support for longer ipids?
  // This reduces the (fairly high) chance of
  // birthday paradox issues arising. However,
  // typing more than 8 characters might be a
  // bit cumbersome.

  QCryptographicHash hash(QCryptographicHash::Md5); // Don't need security, just hashing for uniqueness

  hash.addData(address.toString().toUtf8());

  return hash.result().toHex().right(8); // Use the last 8 characters (4 bytes)
}
