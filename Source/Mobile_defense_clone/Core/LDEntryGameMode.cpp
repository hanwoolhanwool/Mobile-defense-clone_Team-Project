#include "Core/LDEntryGameMode.h"

#include "Core/LDEntryPlayerController.h"

ALDEntryGameMode::ALDEntryGameMode()
{
	PlayerControllerClass = ALDEntryPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
	bStartPlayersAsSpectators = true;
}
