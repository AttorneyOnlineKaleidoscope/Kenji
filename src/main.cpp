#include "config_manager.h"
#include "server.h"

#include <QCoreApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
  QCoreApplication app(argc, argv);
  QCoreApplication::setApplicationName("akashi");
  QCoreApplication::setApplicationVersion("jackfruit (1.9)");

  int exit_code = 0;
  if (ConfigManager::verifyServerConfig())
  {
    Server server;
    server.start(ConfigManager::serverPort());
    exit_code = app.exec();
  }
  else
  {
    qCritical() << "config.ini is invalid!";
    qCritical() << "Exiting server due to configuration issue.";
    exit_code = EXIT_FAILURE;
  }

  return exit_code;
}
