#pragma once

#include <QVariant>

#include <math.h>

class AkashiUtils
{
private:
  AkashiUtils(){};

public:
  template <typename T>
  static inline bool checkArgType(QString arg)
  {
    QVariant qvar = arg;
    if (!qvar.canConvert<T>())
    {
      return false;
    }

    if (std::is_same<T, int>())
    {
      bool ok;
      qvar.toInt(&ok);
      return ok;
    }
    else if (std::is_same<T, float>())
    {
      bool ok;
      float f = qvar.toFloat(&ok);
      return ok && !isnan((float)f);
    }
    else if (std::is_same<T, double>())
    {
      bool ok;
      double d = qvar.toDouble(&ok);
      return ok && !isnan((double)d);
    }

    return true;
  };
};
