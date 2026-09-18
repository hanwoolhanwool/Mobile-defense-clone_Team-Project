#include "Core/LDPlayerState.h"

#include "Net/UnrealNetwork.h"

bool ALDPlayerState::InitializeParticipant(const FLDParticipantContext& Context)
{
	if (!HasAuthority() || !Context.IsValid())
	{
		return false;
	}
	if (ParticipantContext.IsValid())
	{
		return ParticipantContext.MatchId == Context.MatchId && ParticipantContext.PlayerIndex == Context.PlayerIndex &&
		       ParticipantContext.ConnectionEpoch == Context.ConnectionEpoch;
	}
	ParticipantContext = Context;
	ForceNetUpdate();
	return true;
}

const FLDParticipantContext& ALDPlayerState::GetParticipantContext() const
{
	return ParticipantContext;
}

void ALDPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALDPlayerState, ParticipantContext);
}
