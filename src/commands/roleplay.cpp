#include "aoclient.h"

#include "area_data.h"
#include "config_manager.h"
#include "network/packet/timer_packets.h"
#include "server.h"

// This file is for commands under the roleplay category in aoclient.h
// Be sure to register the command in the header before adding it here!

void AOClient::cmdFlip(int argc, QStringList argv)
{
  Q_UNUSED(argc);
  Q_UNUSED(argv);

  QString l_sender_name = name();
  QStringList l_faces = {"heads", "tails"};
  QString l_face = l_faces[AOClient::genRand(0, 1)];
  sendServerMessageArea(l_sender_name + " flipped a coin and got " + l_face + ".");
}

void AOClient::cmdRoll(int argc, QStringList argv)
{
  int l_sides = 6;
  int l_dice = 1;
  QStringList results;

  if (argc >= 1)
  {
    if (argv[0].contains('d'))
    {
      QStringList l_arguments = argv[0].split('d');

      bool l_dice_ok;
      bool l_sides_ok;
      l_dice = l_arguments[0].toInt(&l_dice_ok);
      l_sides = l_arguments[1].toInt(&l_sides_ok);

      if (argv[0].contains('+'))
      {
        bool l_mod_ok;
        QStringList l_modifier = l_arguments[1].split('+');
        int modifier = l_modifier[1].toInt(&l_mod_ok);
        l_sides = l_modifier[0].toInt(&l_sides_ok);

        if (l_mod_ok && l_dice_ok && l_sides_ok)
        {
          diceThrower(l_sides, l_dice, false, modifier);
        }
        else
        {
          sendServerMessage("Invalid dice notation.");
        }
        return;
      }
      else if (argv[0].contains('-'))
      {
        bool l_mod_ok;
        QStringList l_modifier = l_arguments[1].split('-');
        int modifier = l_modifier[1].toInt(&l_mod_ok);
        l_sides = l_modifier[0].toInt(&l_sides_ok);

        if (l_mod_ok && l_dice_ok && l_sides_ok)
        {
          diceThrower(l_sides, l_dice, false, -modifier);
        }
        else
        {
          sendServerMessage("Invalid dice notation.");
        }
        return;
      }
      else if (l_dice_ok && l_sides_ok)
      {
        diceThrower(l_sides, l_dice, false);
        return;
      }
      else
      {
        sendServerMessage("Invalid dice notation.");
        return;
      }
    }
    else
    {
      l_sides = qBound(1, argv[0].toInt(), ConfigManager::diceMaxValue());
    }
  }
  if (argc == 2)
  {
    l_dice = qBound(1, argv[1].toInt(), ConfigManager::diceMaxDice());
  }
  diceThrower(l_sides, l_dice, false);
}

void AOClient::cmdRollP(int argc, QStringList argv)
{
  int l_sides = 6;
  int l_dice = 1;
  QStringList results;

  if (argc >= 1)
  {
    if (argv[0].contains('d'))
    {
      QStringList l_arguments = argv[0].split('d');

      bool l_dice_ok;
      bool l_sides_ok;
      l_dice = l_arguments[0].toInt(&l_dice_ok);
      l_sides = l_arguments[1].toInt(&l_sides_ok);

      if (argv[0].contains('+'))
      {
        bool l_mod_ok;
        QStringList l_modifier = l_arguments[1].split('+');
        int modifier = l_modifier[1].toInt(&l_mod_ok);
        l_sides = l_modifier[0].toInt(&l_sides_ok);

        if (l_mod_ok && l_dice_ok && l_sides_ok)
        {
          diceThrower(l_sides, l_dice, true, modifier);
        }
        else
        {
          sendServerMessage("Invalid dice notation.");
        }
        return;
      }
      else if (argv[0].contains('-'))
      {
        bool l_mod_ok;
        QStringList l_modifier = l_arguments[1].split('-');
        int modifier = l_modifier[1].toInt(&l_mod_ok);
        l_sides = l_modifier[0].toInt(&l_sides_ok);

        if (l_mod_ok && l_dice_ok && l_sides_ok)
        {
          diceThrower(l_sides, l_dice, true, -modifier);
        }
        else
        {
          sendServerMessage("Invalid dice notation.");
        }
        return;
      }
      else if (l_dice_ok && l_sides_ok)
      {
        diceThrower(l_sides, l_dice, true);
        return;
      }
      else
      {
        sendServerMessage("Invalid dice notation.");
        return;
      }
    }
    else
    {
      l_sides = qBound(1, argv[0].toInt(), ConfigManager::diceMaxValue());
    }
  }
  if (argc == 2)
  {
    l_dice = qBound(1, argv[1].toInt(), ConfigManager::diceMaxDice());
  }
  diceThrower(l_sides, l_dice, true);
}

void AOClient::cmdTimer(int argc, QStringList argv)
{
  AreaData *l_area = m_server->area(areaId());

  // Called without arguments
  // Shows a brief of all timers
  if (argc == 0)
  {
    QStringList l_timers;
    l_timers.append("Currently active timers:");
    for (int i = 0; i <= 4; i++)
    {
      l_timers.append(getAreaTimer(l_area->index(), i));
    }
    sendServerMessage(l_timers.join("\n"));
    return;
  }

  // Called with more than one argument
  bool ok;
  int l_timer_id = argv[0].toInt(&ok);
  if (!ok || l_timer_id < 0 || l_timer_id > 4)
  {
    sendServerMessage("Invalid timer ID. Timer ID must be a whole number between 0 and 4.");
    return;
  }

  // Called with one argument
  // Shows the status of one timer
  if (argc == 1)
  {
    sendServerMessage(getAreaTimer(l_area->index(), l_timer_id));
    return;
  }

  // Called with more than one argument
  // Updates the state of a timer

  // Select the proper timer
  // Check against permissions if global timer is selected
  QTimer *l_requested_timer;
  if (l_timer_id == 0)
  {
    if (!checkPermission(ACLRole::GLOBAL_TIMER))
    {
      sendServerMessage("You are not authorized to alter the global timer.");
      return;
    }
    l_requested_timer = m_server->timer;
  }
  else
  {
    l_requested_timer = l_area->timers().at(l_timer_id - 1);
  }

  kal::SetTimerStatePacket l_show_timer(l_timer_id, kal::ShowTimer);
  kal::SetTimerStatePacket l_hide_timer(l_timer_id, kal::HideTimer);
  bool l_is_global = l_timer_id == 0;

  // Set the timer's time remaining if the second
  // argument is a valid time
  QTime l_requested_time = QTime::fromString(argv[1], "hh:mm:ss");
  if (l_requested_time.isValid())
  {
    l_requested_timer->setInterval(QTime(0, 0).msecsTo(l_requested_time));
    l_requested_timer->start();
    sendServerMessage("Set timer " + QString::number(l_timer_id) + " to " + argv[1] + ".");

    kal::SetTimerStatePacket l_update_timer(l_timer_id, kal::StartTimer, QTime(0, 0).msecsTo(l_requested_time));
    if (l_is_global)
    {
      m_server->broadcast(l_show_timer);
      m_server->broadcast(l_update_timer);
    }
    else
    {
      m_server->broadcast(l_show_timer, areaId());
      m_server->broadcast(l_update_timer, areaId());
    }
  }
  // Otherwise, update the state of the timer
  else
  {
    if (argv[1] == "start")
    {
      l_requested_timer->start();
      sendServerMessage("Started timer " + QString::number(l_timer_id) + ".");

      kal::SetTimerStatePacket l_update_timer(l_timer_id, kal::StartTimer, QTime(0, 0).msecsTo(QTime(0, 0).addMSecs(l_requested_timer->remainingTime())));
      if (l_is_global)
      {
        m_server->broadcast(l_show_timer);
        m_server->broadcast(l_update_timer);
      }
      else
      {
        m_server->broadcast(l_show_timer, areaId());
        m_server->broadcast(l_update_timer, areaId());
      }
    }
    else if (argv[1] == "pause" || argv[1] == "stop")
    {
      l_requested_timer->setInterval(l_requested_timer->remainingTime());
      l_requested_timer->stop();

      sendServerMessage("Stopped timer " + QString::number(l_timer_id) + ".");

      kal::SetTimerStatePacket l_update_timer(l_timer_id, kal::StopTimer, QTime(0, 0).msecsTo(QTime(0, 0).addMSecs(l_requested_timer->interval())));
      l_is_global ? m_server->broadcast(l_update_timer) : m_server->broadcast(l_update_timer, areaId());
      if (l_is_global)
      {
        m_server->broadcast(l_update_timer);
      }
      else
      {
        m_server->broadcast(l_update_timer, areaId());
      }
    }
    else if (argv[1] == "hide" || argv[1] == "unset")
    {
      l_requested_timer->setInterval(0);
      l_requested_timer->stop();
      sendServerMessage("Hid timer " + QString::number(l_timer_id) + ".");
      // Hide the timer
      if (l_is_global)
      {
        m_server->broadcast(l_hide_timer);
      }
      else
      {
        m_server->broadcast(l_hide_timer, areaId());
      }
    }
  }
}

void AOClient::cmdNoteCard(int argc, QStringList argv)
{
  Q_UNUSED(argc);

  AreaData *l_area = m_server->area(areaId());
  QString l_notecard = argv.join(" ");
  l_area->addNotecard(character(), l_notecard);
  sendServerMessageArea(character() + " wrote a note card.");
}

void AOClient::cmdNoteCardReveal(int argc, QStringList argv)
{
  Q_UNUSED(argc);
  Q_UNUSED(argv);

  AreaData *l_area = m_server->area(areaId());
  const QStringList l_notecards = l_area->getNotecards();

  if (l_notecards.isEmpty())
  {
    sendServerMessage("There are no cards to reveal in this area.");
    return;
  }

  QString l_message("Note cards have been revealed.\n");
  l_message.append(l_notecards.join(""));

  sendServerMessageArea(l_message);
}

void AOClient::cmdNoteCardClear(int argc, QStringList argv)
{
  Q_UNUSED(argc);
  Q_UNUSED(argv);

  AreaData *l_area = m_server->area(areaId());
  if (!l_area->addNotecard(character(), QString()))
  {
    sendServerMessageArea(character() + " erased their note card.");
  }
}

void AOClient::cmd8Ball(int argc, QStringList argv)
{
  Q_UNUSED(argc);

  if (ConfigManager::magic8BallAnswers().isEmpty())
  {
    qWarning() << "8ball.txt is empty!";
    sendServerMessage("8ball.txt is empty.");
  }
  else
  {
    QString l_response = ConfigManager::magic8BallAnswers().at((genRand(1, ConfigManager::magic8BallAnswers().size() - 1)));
    QString l_sender_name = name();
    QString l_sender_message = argv.join(" ");

    sendServerMessageArea(l_sender_name + " asked the magic 8-ball, \"" + l_sender_message + "\" and the answer is: " + l_response);
  }
}
