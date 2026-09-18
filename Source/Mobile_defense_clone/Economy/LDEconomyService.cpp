#include "Economy/LDEconomyService.h"

bool ULDEconomyService::Initialize(const FLDMatchContext& Context, ULDGameData& Data, int32 Seed)
{
	if (GameData || !Context.IsValid() || !Data.IsLoaded())
	{
		return false;
	}
	GameData = &Data;
	MatchContext = Context;
	for (int32 Player = 0; Player < 2; ++Player)
	{
		FLDEconomySnapshot State;
		State.MatchId = Context.MatchId;
		State.PlayerIndex = Player;
		State.Gold = Data.GetRules().StartingGold;
		State.Stars = Data.GetRules().StartingStars;
		State.NextSummonGold = Data.GetRules().SummonBaseGold;
		States.Add(State);
		RandomStreams.Add(FRandomStream(static_cast<int32>(static_cast<uint32>(Seed) + Player * 2654435761u)));
	}
	return true;
}

FName ULDEconomyService::DrawUnit(FName Grade, FRandomStream& Random) const
{
	TArray<FName> Candidates;
	for (const auto& Pair : GameData->GetUnits())
	{
		if (Pair.Value.Grade == Grade && Pair.Value.bEnabledInP0)
		{
			Candidates.Add(Pair.Key);
		}
	}
	Candidates.Sort(FNameLexicalLess());
	return Candidates.IsEmpty() ? NAME_None : Candidates[Random.RandRange(0, Candidates.Num() - 1)];
}

ELDCommandResultCode ULDEconomyService::TryPrepare(int32 PlayerIndex, const FLDCommand& Command, FName SourceUnitId,
                                                   FLDEconomyPlan& OutPlan) const
{
	if (!States.IsValidIndex(PlayerIndex) || bClosed)
	{
		return ELDCommandResultCode::PhaseNotAllowed;
	}
	OutPlan = FLDEconomyPlan();
	OutPlan.Before = States[PlayerIndex];
	OutPlan.After = OutPlan.Before;
	OutPlan.ExpectedRandomSeed = RandomStreams[PlayerIndex].GetCurrentSeed();
	OutPlan.NextRandom = RandomStreams[PlayerIndex];
	const FLDGameRules& Rules = GameData->GetRules();
	FLDUnitRow Row;
	const FName Grades[] = {TEXT("Common"), TEXT("Rare"), TEXT("Epic"), TEXT("Legendary")};
	if (Command.CommandType == ELDCommandType::Summon)
	{
		if (Command.Source != 0)
		{
			return ELDCommandResultCode::FeatureDisabled;
		}
		if (OutPlan.After.Gold < OutPlan.After.NextSummonGold)
		{
			return ELDCommandResultCode::InsufficientResource;
		}
		if (OutPlan.After.NextSummonGold > MAX_int32 - Rules.SummonIncrementGold)
		{
			return ELDCommandResultCode::LimitReached;
		}
		int32 Draw = OutPlan.NextRandom.RandRange(0, 9999);
		for (FName Grade : Grades)
		{
			Draw -= Rules.GoldGradeWeights.FindRef(Grade);
			if (Draw < 0)
			{
				OutPlan.ResultUnitId = DrawUnit(Grade, OutPlan.NextRandom);
				break;
			}
		}
		OutPlan.After.Gold -= OutPlan.After.NextSummonGold;
		++OutPlan.After.PaidSummonCount;
		OutPlan.After.NextSummonGold += Rules.SummonIncrementGold;
	}
	else if (Command.CommandType == ELDCommandType::Merge || Command.CommandType == ELDCommandType::Sell)
	{
		if (!GameData->TryGetUnitRow(SourceUnitId, Row))
		{
			return ELDCommandResultCode::InvalidData;
		}
		if (Command.CommandType == ELDCommandType::Merge)
		{
			int32 GradeIndex = INDEX_NONE;
			for (int32 Index = 0; Index < 3; ++Index)
			{
				if (Row.Grade == Grades[Index])
				{
					GradeIndex = Index;
				}
			}
			if (GradeIndex == INDEX_NONE)
			{
				return ELDCommandResultCode::FeatureDisabled;
			}
			OutPlan.ResultUnitId = DrawUnit(Grades[GradeIndex + 1], OutPlan.NextRandom);
		}
		else
		{
			const int32 Amount =
			    Row.Grade == Grades[0]
			        ? FMath::FloorToInt(OutPlan.After.NextSummonGold * Rules.CommonSaleFractionOfNextPaidPrice)
			        : Row.SaleAmount;
			int32& Balance = Row.Grade == Grades[0] ? OutPlan.After.Gold : OutPlan.After.Stars;
			if (Amount < 0 || Balance > MAX_int32 - Amount)
			{
				return ELDCommandResultCode::LimitReached;
			}
			Balance += Amount;
		}
	}
	else if (Command.CommandType != ELDCommandType::Move)
	{
		return ELDCommandResultCode::FeatureDisabled;
	}
	if ((Command.CommandType == ELDCommandType::Summon || Command.CommandType == ELDCommandType::Merge) &&
	    OutPlan.ResultUnitId.IsNone())
	{
		return ELDCommandResultCode::InvalidData;
	}
	OutPlan.bChangesEconomy = Command.CommandType != ELDCommandType::Move;
	if (OutPlan.bChangesEconomy)
	{
		++OutPlan.After.EconomyRevision;
	}
	return ELDCommandResultCode::Success;
}

bool ULDEconomyService::ValidatePrepared(const FLDEconomyPlan& Plan) const
{
	return !bClosed && States.IsValidIndex(Plan.Before.PlayerIndex) && Plan.Before.MatchId == MatchContext.MatchId &&
	       States[Plan.Before.PlayerIndex].EconomyRevision == Plan.Before.EconomyRevision &&
	       RandomStreams[Plan.Before.PlayerIndex].GetCurrentSeed() == Plan.ExpectedRandomSeed;
}

void ULDEconomyService::CommitPrepared(const FLDEconomyPlan& Plan)
{
	if (Plan.bChangesEconomy)
	{
		States[Plan.After.PlayerIndex] = Plan.After;
		RandomStreams[Plan.After.PlayerIndex] = Plan.NextRandom;
	}
}

void ULDEconomyService::PublishPrepared(const FLDEconomyPlan& Plan)
{
	if (Plan.bChangesEconomy)
	{
		OnEconomyChanged.Broadcast(Plan.After);
	}
}

bool ULDEconomyService::ApplyCombatReward(const FLDCombatDeath& Death)
{
	if (!GameData || bClosed || Death.MatchId != MatchContext.MatchId || Death.DeathEventId == 0 ||
	    RewardedDeaths.Contains(Death.DeathEventId) || !FMath::IsFinite(Death.SpawnedServerSeconds) ||
	    !FMath::IsFinite(Death.DeathServerSeconds) || Death.DeathServerSeconds < Death.SpawnedServerSeconds)
	{
		return false;
	}
	FLDEnemyRow Enemy;
	if (!GameData->TryGetEnemyRow(Death.EnemyTypeId, Enemy))
	{
		return false;
	}
	const FLDGameRules& Rules = GameData->GetRules();
	const bool bBoss = Enemy.Kind == TEXT("Boss");
	const int32 Gold = bBoss ? Rules.BossKillGoldPerPlayer : Rules.NormalKillGoldPerPlayer;
	const int32 Stars = bBoss ? Rules.BossKillStarsPerPlayer +
	                                (Death.DeathServerSeconds - Death.SpawnedServerSeconds <= Rules.FastBossKillSeconds
	                                     ? Rules.FastBossBonusStarsPerPlayer
	                                     : 0)
	                          : 0;
	for (const FLDEconomySnapshot& State : States)
	{
		if (State.Gold > MAX_int32 - Gold || State.Stars > MAX_int32 - Stars)
		{
			return false;
		}
	}
	// One death commits both recipients before any callback can observe a partial reward.
	RewardedDeaths.Add(Death.DeathEventId);
	for (FLDEconomySnapshot& State : States)
	{
		State.Gold += Gold;
		State.Stars += Stars;
		++State.EconomyRevision;
	}
	for (const FLDEconomySnapshot& State : States)
	{
		OnEconomyChanged.Broadcast(State);
	}
	return true;
}

FLDEconomySnapshot ULDEconomyService::GetSnapshot(int32 PlayerIndex) const
{
	return States.IsValidIndex(PlayerIndex) ? States[PlayerIndex] : FLDEconomySnapshot();
}

int32 ULDEconomyService::GetRandomState(int32 PlayerIndex) const
{
	return RandomStreams.IsValidIndex(PlayerIndex) ? RandomStreams[PlayerIndex].GetCurrentSeed() : 0;
}

void ULDEconomyService::Close()
{
	bClosed = true;
}
