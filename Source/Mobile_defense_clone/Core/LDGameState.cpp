#include "Core/LDGameState.h"

#include "Net/UnrealNetwork.h"

bool ALDGameState::InitializeMatch(const FLDMatchContext& Context)
{
	if (!HasAuthority() || !Context.IsValid() || MatchContext.IsValid())
	{
		return false;
	}
	MatchContext = Context;
	return true;
}

bool ALDGameState::SetPhase(ELDMatchPhase NewPhase)
{
	if (!HasAuthority() || NewPhase == Phase)
	{
		return false;
	}
	const bool bAllowed =
	    (Phase == ELDMatchPhase::Loading &&
	     (NewPhase == ELDMatchPhase::Preparing || NewPhase == ELDMatchPhase::Aborted)) ||
	    (Phase == ELDMatchPhase::Preparing &&
	     (NewPhase == ELDMatchPhase::Running || NewPhase == ELDMatchPhase::Aborted)) ||
	    (Phase == ELDMatchPhase::Running && (NewPhase == ELDMatchPhase::Result || NewPhase == ELDMatchPhase::Aborted));
	if (bAllowed)
	{
		Phase = NewPhase;
		ForceNetUpdate();
	}
	return bAllowed;
}

ELDMatchPhase ALDGameState::GetPhase() const
{
	return Phase;
}

const FLDMatchContext& ALDGameState::GetMatchContext() const
{
	return MatchContext;
}

void ALDGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALDGameState, MatchContext);
	DOREPLIFETIME(ALDGameState, Phase);
}
