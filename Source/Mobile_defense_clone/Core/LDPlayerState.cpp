#include "Core/LDPlayerState.h"

#include "Net/UnrealNetwork.h"

void ALDPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ALDPlayerState, ParticipantContext, COND_OwnerOnly);
	DOREPLIFETIME(ALDPlayerState, PublicPlayerIndex);
}

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
	PublicPlayerIndex = Context.PlayerIndex;
	ForceNetUpdate();
	return true;
}

const FLDParticipantContext& ALDPlayerState::GetParticipantContext() const
{
	return ParticipantContext;
}

int32 ALDPlayerState::GetPlayerIndex() const
{
	return PublicPlayerIndex;
}
