#pragma once

#include "area_data.h"
#include "clientprocessor.h"

class AOClient;

namespace kal
{
class ClientSessionProcessor : public ClientProcessor
{
public:
  ClientSessionProcessor(AOClient &client);

  // area
  void cargo(AreaListPacket &cargo);
  void cargo(UpdateAreaPacket &cargo);
  void cargo(JoinAreaPacket &cargo);
  // background
  void cargo(BackgroundPacket &cargo);
  void cargo(PositionPacket &cargo);
  // case
  void cargo(CasePacket_CASEA &cargo);
  // character
  void cargo(CharacterListPacket &cargo);
  void cargo(SelectCharacterPacket &cargo);
  void cargo(CharacterSelectStatePacket &cargo);
  // chat
  void cargo(ChatPacket &cargo);
  // evidence
  void cargo(EvidenceListPacket &cargo);
  void cargo(CreateEvidencePacket &cargo);
  void cargo(EditEvidencePacket &cargo);
  void cargo(DeleteEvidencePacket &cargo);
  // judge
  void cargo(JudgeStatePacket &cargo);
  void cargo(HealthStatePacket &cargo);
  // splash
  void cargo(SplashPacket &cargo);
  // login
  void cargo(LoginPacket &cargo);
  // message
  void cargo(MessagePacket &cargo);
  // moderation/support
  void cargo(ModerationAction &cargo);
  void cargo(ModerationNotice &cargo);
  void cargo(HelpMePacket &cargo);
  // music
  void cargo(MusicListPacket &cargo);
  void cargo(PlayTrackPacket &cargo);
  // player
  void cargo(RegisterPlayerPacket &cargo);
  void cargo(UpdatePlayerPacket &cargo);
  // system
  void cargo(NoticePacket &cargo);
  // timer
  void cargo(SetTimerStatePacket &cargo);

private:
  AOClient &client;
};
} // namespace kal
