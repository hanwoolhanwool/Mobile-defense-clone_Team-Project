#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LDEntryGameMode.generated.h"

/** Local entry map. Deliberately has no match services or participant loading deadline. */
UCLASS()
class MOBILE_DEFENSE_CLONE_API ALDEntryGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ALDEntryGameMode();
};
