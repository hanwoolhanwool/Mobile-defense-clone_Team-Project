#include "Core/LDGameState.h"

#include "Net/UnrealNetwork.h"

ALDGameState::ALDGameState()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
}

void ALDGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALDGameState, MatchContext);
	DOREPLIFETIME(ALDGameState, BattleSnapshot);
	DOREPLIFETIME(ALDGameState, ReadinessReason);
}

bool ALDGameState::InitializeMatch(const FLDMatchContext& Context)
{
	if (!HasAuthority() || !Context.IsValid())
	{
		return false;
	}
	if (MatchContext.IsValid())
	{
		return MatchContext.MatchId == Context.MatchId && MatchContext.RulesVersion == Context.RulesVersion;
	}
	MatchContext = Context;
	BattleSnapshot.MatchId = Context.MatchId;
	++BattleSnapshot.Revision;
	OnRep_CommonState();
	ForceNetUpdate();
	return true;
}

bool ALDGameState::SetPhase(ELDMatchPhase NewPhase)
{
	if (!HasAuthority())
	{
		return false;
	}
	if (BattleSnapshot.Phase == NewPhase)
	{
		return true;
	}
	const bool bTerminal =
	    BattleSnapshot.Phase == ELDMatchPhase::Result || BattleSnapshot.Phase == ELDMatchPhase::Aborted;
	const bool bValidTransition =
	    !bTerminal && (NewPhase == ELDMatchPhase::Aborted ||
	                   (BattleSnapshot.Phase == ELDMatchPhase::Loading && NewPhase == ELDMatchPhase::Preparing) ||
	                   (BattleSnapshot.Phase == ELDMatchPhase::Preparing && NewPhase == ELDMatchPhase::Running) ||
	                   (BattleSnapshot.Phase == ELDMatchPhase::Running && NewPhase == ELDMatchPhase::Result));
	if (!bValidTransition || (!MatchContext.IsValid() && NewPhase != ELDMatchPhase::Aborted))
	{
		return false;
	}
	BattleSnapshot.Phase = NewPhase;
	++BattleSnapshot.Revision;
	OnRep_CommonState();
	ForceNetUpdate();
	return true;
}

void ALDGameState::SetReadinessReason(const FString& Reason)
{
	if (HasAuthority() && ReadinessReason != Reason)
	{
		ReadinessReason = Reason;
		OnRep_CommonState();
		ForceNetUpdate();
	}
}

const FLDMatchContext& ALDGameState::GetMatchContext() const
{
	return MatchContext;
}

ELDMatchPhase ALDGameState::GetPhase() const
{
	return BattleSnapshot.Phase;
}

const FString& ALDGameState::GetReadinessReason() const
{
	return ReadinessReason;
}

void ALDGameState::OnRep_CommonState()
{
	OnMatchStateChanged.Broadcast();
}

const FLDBattleSnapshot& ALDGameState::GetBattleSnapshot() const
{
	return BattleSnapshot;
}
