#include "aoclient.h"

#include "config_manager.h"
#include "crypto_helper.h"
#include "db_manager.h"
#include "network/packet/login_packets.h"
#include "server.h"

// This file is for commands under the authentication category in aoclient.h
// Be sure to register the command in the header before adding it here!

void AOClient::cmdLogin(int argc, QStringList argv)
{
  Q_UNUSED(argc);
  Q_UNUSED(argv);

  if (m_authenticated)
  {
    sendServerMessage("You are already logged in!");
    return;
  }
  switch (ConfigManager::authType())
  {
  case DataTypes::AuthType::SIMPLE:
    if (ConfigManager::modpass() == "")
    {
      sendServerMessage("No modpass is set. Please set a modpass before logging in.");
      return;
    }
    else
    {
      sendServerMessage("Entering login prompt.\nPlease enter the server modpass.");
      m_is_logging_in = true;
      return;
    }
    break;
  case DataTypes::AuthType::ADVANCED:
    sendServerMessage("Entering login prompt.\nPlease enter your username and password.");
    m_is_logging_in = true;
    return;
    break;
  }
}

void AOClient::cmdChangeAuth(int argc, QStringList argv)
{
  Q_UNUSED(argc);
  Q_UNUSED(argv);

  if (ConfigManager::authType() == DataTypes::AuthType::SIMPLE)
  {
    change_auth_started = true;
    sendServerMessage("WARNING!\nThis command will change how logging in as a moderator works.\nOnly proceed if you know what you are doing\nUse the command /rootpass to set the password for your root account.");
  }
}

void AOClient::cmdSetRootPass(int argc, QStringList argv)
{
  Q_UNUSED(argc);

  if (!change_auth_started)
  {
    return;
  }

  if (!checkPasswordRequirements("root", argv[0]))
  {
    sendServerMessage("Password does not meet server requirements.");
    return;
  }

  sendServerMessage("Changing auth type and setting root password.\nLogin again with /login root [password]");
  m_authenticated = false;
  ConfigManager::setAuthType(DataTypes::AuthType::ADVANCED);

  QByteArray l_salt = CryptoHelper::randbytes(16);

  m_server->getDatabaseManager()->createUser("root", l_salt, argv[0], ACLRolesHandler::SUPER_ID);
}

void AOClient::cmdAddUser(int argc, QStringList argv)
{
  Q_UNUSED(argc);

  if (!checkPasswordRequirements(argv[0], argv[1]))
  {
    sendServerMessage("Password does not meet server requirements.");
    return;
  }

  QByteArray l_salt = CryptoHelper::randbytes(16);

  if (m_server->getDatabaseManager()->createUser(argv[0], l_salt, argv[1], ACLRolesHandler::NONE_ID))
  {
    sendServerMessage("Created user " + argv[0] + ".\nUse /setperms to modify their permissions.");
  }
  else
  {
    sendServerMessage("Unable to create user " + argv[0] + ".\nDoes a user with that name already exist?");
  }
}

void AOClient::cmdRemoveUser(int argc, QStringList argv)
{
  Q_UNUSED(argc);

  if (m_server->getDatabaseManager()->deleteUser(argv[0]))
  {
    sendServerMessage("Successfully removed user " + argv[0] + ".");
  }
  else
  {
    sendServerMessage("Unable to remove user " + argv[0] + ".\nDoes it exist?");
  }
}

void AOClient::cmdListPerms(int argc, QStringList argv)
{
  const ACLRole l_role = m_server->getACLRolesHandler()->getRoleById(m_acl_role_id);

  ACLRole l_target_role = l_role;
  QStringList l_message;
  if (argc == 0)
  {
    l_message.append("You have been given the following permissions:");
  }
  else
  {
    if (!l_role.checkPermission(ACLRole::MODIFY_USERS))
    {
      sendServerMessage("You do not have permission to view other users' permissions.");
      return;
    }

    l_message.append("User " + argv[0] + " has the following permissions:");
    l_target_role = m_server->getACLRolesHandler()->getRoleById(argv[0]);
  }

  if (l_target_role.getPermissions() == ACLRole::NONE)
  {
    l_message.append("NONE");
  }
  else if (l_target_role.checkPermission(ACLRole::SUPER))
  {
    l_message.append("SUPER (Be careful! This grants the user all permissions.)");
  }
  else
  {
    const QList<ACLRole::Permission> l_permissions = ACLRole::PERMISSION_CAPTIONS.keys();
    for (const ACLRole::Permission i_permission : l_permissions)
    {
      if (l_target_role.checkPermission(i_permission))
      {
        l_message.append(ACLRole::PERMISSION_CAPTIONS.value(i_permission));
      }
    }
  }
  sendServerMessage(l_message.join("\n"));
}

void AOClient::cmdSetPerms(int argc, QStringList argv)
{
  Q_UNUSED(argc);

  const QString l_target_acl = argv[1];
  if (!m_server->getACLRolesHandler()->roleExists(l_target_acl))
  {
    sendServerMessage("That role doesn't exist!");
    return;
  }

  if (l_target_acl == ACLRolesHandler::SUPER_ID && !checkPermission(ACLRole::SUPER))
  {
    sendServerMessage("You aren't allowed to set that role!");
    return;
  }

  const QString l_target_username = argv[0];
  if (l_target_username == "root")
  {
    sendServerMessage("You can't change root's role!");
    return;
  }

  if (m_server->getDatabaseManager()->updateACL(l_target_username, l_target_acl))
  {
    sendServerMessage("Successfully applied role " + l_target_acl + " to user " + l_target_username);
  }
  else
  {
    sendServerMessage(l_target_username + " wasn't found!");
  }
}

void AOClient::cmdRemovePerms(int argc, QStringList argv)
{
  argv.append(ACLRolesHandler::NONE_ID);
  cmdSetPerms(argc, argv);
}

void AOClient::cmdListUsers(int argc, QStringList argv)
{
  Q_UNUSED(argc);
  Q_UNUSED(argv);

  QStringList l_users = m_server->getDatabaseManager()->getUsers();
  sendServerMessage("All users:\n" + l_users.join("\n"));
}

void AOClient::cmdLogout(int argc, QStringList argv)
{
  Q_UNUSED(argc);
  Q_UNUSED(argv);

  if (!m_authenticated)
  {
    sendServerMessage("You are not logged in!");
    return;
  }
  m_authenticated = false;
  m_acl_role_id = "";
  m_moderator_name = "";
  shipPacket(kal::LoginPacket{kal::LoginPacket::Logout});
}

void AOClient::cmdChangePassword(int argc, QStringList argv)
{
  QString l_username;
  QString l_password = argv[0];
  if (argc == 1)
  {
    if (m_moderator_name.isEmpty())
    {
      sendServerMessage("You are not logged in.");
      return;
    }
    l_username = m_moderator_name;
  }
  else if (argc == 2 && checkPermission(ACLRole::SUPER))
  {
    l_username = argv[1];
  }
  else
  {
    sendServerMessage("Invalid command syntax.");
    return;
  }

  if (!checkPasswordRequirements(l_username, l_password))
  {
    sendServerMessage("Password does not meet server requirements.");
    return;
  }

  if (m_server->getDatabaseManager()->updatePassword(l_username, l_password))
  {
    sendServerMessage("Successfully changed password.");
  }
  else
  {
    sendServerMessage("There was an error changing the password.");
    return;
  }
}
