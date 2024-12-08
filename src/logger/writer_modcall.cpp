#include "logger/writer_modcall.h"

WriterModcall::WriterModcall(QObject *parent)
    : QObject(parent)
{
  l_dir.setPath("logs/");
  if (!l_dir.exists())
  {
    l_dir.mkpath(".");
  }

  l_dir.setPath("logs/modcall");
  if (!l_dir.exists())
  {
    l_dir.mkpath(".");
  }
}

void WriterModcall::flush(const QString f_area_name, QQueue<QString> f_buffer)
{
  l_logfile.setFileName(QString("logs/modcall/report_%1_%2.log").arg(f_area_name, (QDateTime::currentDateTime().toString("yyyy-MM-dd_hhmmss"))));

  if (l_logfile.open(QIODevice::WriteOnly | QIODevice::Append))
  {
    QTextStream file_stream(&l_logfile);

    while (!f_buffer.isEmpty())
    {
      file_stream << f_buffer.dequeue();
    }
  }

  l_logfile.close();
};
