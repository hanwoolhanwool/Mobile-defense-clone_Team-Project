#include "Board/LDBoardManager.h"

#include "Battle/LDUnitActor.h"
#include "Engine/World.h"

namespace
{
	const FLDPlacedUnit* FindUnit(const FLDBoardSnapshot& State, uint64 Id)
	{
		return State.Units.FindByPredicate([Id](const FLDPlacedUnit& Unit) { return Unit.InstanceId == Id; });
	}

	int32 CountCell(const FLDBoardSnapshot& State, int32 Cell)
	{
		int32 Count = 0;
		for (const FLDPlacedUnit& Unit : State.Units)
		{
			Count += Unit.CellId == Cell ? 1 : 0;
		}
		return Count;
	}
} // namespace

bool ULDBoardManager::Initialize(UWorld& World, const FLDMatchContext& Context, ULDGameData& Data,
                                 FLDPrepareUnit PrepareUnit)
{
	FString Error;
	if (GameData || World.GetNetMode() == NM_Client || !Context.IsValid() || !Data.IsLoaded() ||
	    !Geometry.Initialize(Data.GetRules(), Error))
	{
		return false;
	}
	GameData = &Data;
	ServerWorld = &World;
	MatchContext = Context;
	UnitPreparation = MoveTemp(PrepareUnit);
	if (!UnitPreparation)
	{
		UnitPreparation =
		    [](UWorld& ActorWorld, const FLDPlacedUnit& Unit, const FLDUnitRow& Row, const FTransform& Transform)
		{
			ALDUnitActor* Actor = ActorWorld.SpawnActor<ALDUnitActor>();
			if (Actor && !Actor->InitializePrepared(Unit, Row, Transform))
			{
				Actor->Destroy();
				Actor = nullptr;
			}
			return Actor;
		};
	}
	for (int32 Player = 0; Player < 2; ++Player)
	{
		FLDBoardSnapshot State;
		State.MatchId = Context.MatchId;
		State.PlayerIndex = Player;
		States.Add(State);
	}
	return true;
}

int32 ULDBoardManager::FindPlacementCell(const FLDBoardSnapshot& State, FName UnitId) const
{
	for (const FLDPlacedUnit& Unit : State.Units)
	{
		if (Unit.UnitId == UnitId && CountCell(State, Unit.CellId) < GameData->GetRules().MaxStack)
		{
			return Unit.CellId;
		}
	}
	for (int32 Cell : GameData->GetRules().PlacementOrderByPlayer[State.PlayerIndex])
	{
		if (CountCell(State, Cell) == 0)
		{
			return Cell;
		}
	}
	return INDEX_NONE;
}

ELDCommandResultCode ULDBoardManager::ValidateCommand(const FLDParticipantContext& Context, const FLDCommand& Command,
                                                      double ServerSeconds) const
{
	if (!GameData || bClosed || !ServerWorld.IsValid() || !Context.IsValid() ||
	    Context.MatchId != MatchContext.MatchId || !States.IsValidIndex(Context.PlayerIndex) ||
	    !FMath::IsFinite(ServerSeconds))
	{
		return ELDCommandResultCode::PhaseNotAllowed;
	}
	const FLDBoardSnapshot& State = States[Context.PlayerIndex];
	if (Command.ExpectedBoardRevision != State.BoardRevision)
	{
		return ELDCommandResultCode::StaleBoard;
	}
	if (Command.CommandType == ELDCommandType::Summon)
	{
		if (State.Population >= GameData->GetRules().MaxUnitsPerPlayer)
		{
			return ELDCommandResultCode::LimitReached;
		}
		for (const auto& Pair : GameData->GetUnits())
		{
			if (Pair.Value.bEnabledInP0 && FindPlacementCell(State, Pair.Key) == INDEX_NONE)
			{
				return ELDCommandResultCode::NoSpace;
			}
		}
		return ELDCommandResultCode::Success;
	}
	if (Command.CommandType != ELDCommandType::Merge && Command.CommandType != ELDCommandType::Sell &&
	    Command.CommandType != ELDCommandType::Move)
	{
		return ELDCommandResultCode::FeatureDisabled;
	}
	const FLDPlacedUnit* Source = FindUnit(State, Command.InstanceId);
	if (!Source)
	{
		FLDPlacedUnit Other;
		return TryGetUnit(Command.InstanceId, Other) ? ELDCommandResultCode::NotOwner
		                                             : ELDCommandResultCode::MissingInstance;
	}
	if (Command.CommandType == ELDCommandType::Merge)
	{
		const uint64 Ids[] = {Command.ConsumedInstanceId0, Command.ConsumedInstanceId1, Command.ConsumedInstanceId2};
		if (Ids[0] == Ids[1] || Ids[0] == Ids[2] || Ids[1] == Ids[2] ||
		    (Command.InstanceId != Ids[0] && Command.InstanceId != Ids[1] && Command.InstanceId != Ids[2]))
		{
			return ELDCommandResultCode::InvalidPayload;
		}
		for (uint64 Id : Ids)
		{
			const FLDPlacedUnit* Material = FindUnit(State, Id);
			if (!Material || Material->CellId != Source->CellId || Material->UnitId != Source->UnitId)
			{
				return ELDCommandResultCode::MissingInstance;
			}
		}
	}
	if (Command.CommandType == ELDCommandType::Move)
	{
		if (Geometry.GetCellOwner(Command.DestinationCellId) != Context.PlayerIndex)
		{
			return ELDCommandResultCode::NotOwner;
		}
		if (Command.DestinationCellId == Source->CellId)
		{
			return ELDCommandResultCode::NoChange;
		}
		for (const FLDPlacedUnit& Unit : State.Units)
		{
			if ((Unit.CellId == Source->CellId || Unit.CellId == Command.DestinationCellId) &&
			    ServerSeconds < Unit.MoveBlockedUntilServerSeconds)
			{
				return ELDCommandResultCode::Locked;
			}
		}
	}
	return ELDCommandResultCode::Success;
}

ELDCommandResultCode ULDBoardManager::TryPrepare(const FLDParticipantContext& Context, const FLDCommand& Command,
                                                 FName ResultUnitId, double ServerSeconds, FLDBoardPlan& OutPlan)
{
	const ELDCommandResultCode Validation = ValidateCommand(Context, Command, ServerSeconds);
	if (Validation != ELDCommandResultCode::Success)
	{
		return Validation;
	}
	OutPlan = FLDBoardPlan();
	const FLDBoardSnapshot& Before = States[Context.PlayerIndex];
	OutPlan.After = Before;
	OutPlan.ExpectedRevision = Before.BoardRevision;
	OutPlan.ExpectedNextInstanceId = NextInstanceId;
	OutPlan.Commit.MatchId = MatchContext.MatchId;
	OutPlan.Commit.PlayerIndex = Context.PlayerIndex;
	OutPlan.Commit.CommitServerSeconds = ServerSeconds;
	FLDPlacedUnit Source;
	TryGetUnit(Command.InstanceId, Source);
	if (Command.CommandType == ELDCommandType::Move)
	{
		OutPlan.Commit.ChangeReason = ELDBoardChangeReason::Move;
		for (FLDPlacedUnit& Unit : OutPlan.After.Units)
		{
			if (Unit.CellId == Source.CellId || Unit.CellId == Command.DestinationCellId)
			{
				Unit.CellId = Unit.CellId == Source.CellId ? Command.DestinationCellId : Source.CellId;
				Unit.MoveBlockedUntilServerSeconds = FMath::Max(Unit.MoveBlockedUntilServerSeconds,
				                                                ServerSeconds + GameData->GetRules().MoveLockSeconds);
				OutPlan.Commit.AddedOrUpdatedUnits.Add(Unit);
			}
		}
	}
	else if (Command.CommandType == ELDCommandType::Sell)
	{
		OutPlan.Commit.ChangeReason = ELDBoardChangeReason::Sell;
		OutPlan.Commit.RemovedInstanceIds.Add(Source.InstanceId);
		OutPlan.After.Units.RemoveAll([&Source](const FLDPlacedUnit& Unit)
		                              { return Unit.InstanceId == Source.InstanceId; });
		if (CountCell(Before, Source.CellId) == GameData->GetRules().MaxStack)
		{
			FLDPlacedUnit* Donor = nullptr;
			for (FLDPlacedUnit& Unit : OutPlan.After.Units)
			{
				if (Unit.UnitId == Source.UnitId && Unit.CellId != Source.CellId &&
				    CountCell(Before, Unit.CellId) < GameData->GetRules().MaxStack &&
				    (!Donor || Unit.InstanceId < Donor->InstanceId))
				{
					Donor = &Unit;
				}
			}
			if (Donor)
			{
				Donor->CellId = Source.CellId;
				OutPlan.Commit.AddedOrUpdatedUnits.Add(*Donor);
			}
		}
	}
	else
	{
		OutPlan.Commit.ChangeReason =
		    Command.CommandType == ELDCommandType::Merge ? ELDBoardChangeReason::Merge : ELDBoardChangeReason::Summon;
		if (Command.CommandType == ELDCommandType::Merge)
		{
			OutPlan.Commit.RemovedInstanceIds = {Command.ConsumedInstanceId0, Command.ConsumedInstanceId1,
			                                     Command.ConsumedInstanceId2};
			OutPlan.After.Units.RemoveAll([&OutPlan](const FLDPlacedUnit& Unit)
			                              { return OutPlan.Commit.RemovedInstanceIds.Contains(Unit.InstanceId); });
		}
		FLDUnitRow Row;
		const int32 Cell = FindPlacementCell(OutPlan.After, ResultUnitId);
		if (!GameData->TryGetUnitRow(ResultUnitId, Row) || !Row.bEnabledInP0 || Cell == INDEX_NONE)
		{
			return ELDCommandResultCode::NoSpace;
		}
		FLDPlacedUnit Unit;
		Unit.InstanceId = NextInstanceId;
		Unit.UnitId = ResultUnitId;
		Unit.PlayerIndex = Context.PlayerIndex;
		Unit.CellId = Cell;
		FTransform Transform;
		if (!TryGetCellTransform(Context.PlayerIndex, Cell, Transform))
		{
			return ELDCommandResultCode::InvalidCell;
		}
		ALDUnitActor* Prepared = UnitPreparation(*ServerWorld.Get(), Unit, Row, Transform);
		if (!IsValid(Prepared))
		{
			return ELDCommandResultCode::InvalidData;
		}
		OutPlan.PreparedActors.Add(Prepared);
		OutPlan.After.Units.Add(Unit);
		OutPlan.Commit.AddedOrUpdatedUnits.Add(Unit);
	}
	OutPlan.After.Population = OutPlan.After.Units.Num();
	OutPlan.Commit.BoardRevision = ++OutPlan.After.BoardRevision;
	return IsValidSnapshot(OutPlan.After) ? ELDCommandResultCode::Success : ELDCommandResultCode::InvalidData;
}

bool ULDBoardManager::IsValidSnapshot(const FLDBoardSnapshot& State) const
{
	if (State.Population != State.Units.Num() || State.Population > GameData->GetRules().MaxUnitsPerPlayer)
	{
		return false;
	}
	TSet<uint64> Ids;
	TMap<int32, FName> Kinds;
	TMap<int32, int32> Counts;
	TMap<FName, int32> PartialCounts;
	for (const FLDPlacedUnit& Unit : State.Units)
	{
		if (Unit.InstanceId == 0 || Ids.Contains(Unit.InstanceId) || Unit.PlayerIndex != State.PlayerIndex ||
		    Geometry.GetCellOwner(Unit.CellId) != State.PlayerIndex ||
		    (Kinds.Contains(Unit.CellId) && Kinds[Unit.CellId] != Unit.UnitId))
		{
			return false;
		}
		Ids.Add(Unit.InstanceId);
		Kinds.Add(Unit.CellId, Unit.UnitId);
		if (++Counts.FindOrAdd(Unit.CellId) > GameData->GetRules().MaxStack)
		{
			return false;
		}
	}
	for (const auto& Pair : Counts)
	{
		if (Pair.Value < GameData->GetRules().MaxStack && ++PartialCounts.FindOrAdd(Kinds[Pair.Key]) > 1)
		{
			return false;
		}
	}
	return true;
}

bool ULDBoardManager::ValidatePrepared(const FLDBoardPlan& Plan) const
{
	if (bClosed || !States.IsValidIndex(Plan.After.PlayerIndex) || Plan.bCommitted ||
	    Plan.After.MatchId != MatchContext.MatchId ||
	    States[Plan.After.PlayerIndex].BoardRevision != Plan.ExpectedRevision ||
	    Plan.ExpectedNextInstanceId != NextInstanceId || !IsValidSnapshot(Plan.After))
	{
		return false;
	}
	for (ALDUnitActor* Actor : Plan.PreparedActors)
	{
		if (!IsValid(Actor) || Actor->IsCommitted())
		{
			return false;
		}
	}
	return true;
}

void ULDBoardManager::CommitPrepared(FLDBoardPlan& Plan)
{
	States[Plan.After.PlayerIndex] = Plan.After;
	for (ALDUnitActor* Actor : Plan.PreparedActors)
	{
		UnitActors.Add(Actor->GetPlacement().InstanceId, Actor);
		++NextInstanceId;
	}
	Plan.bCommitted = true;
}

void ULDBoardManager::PublishPrepared(FLDBoardPlan& Plan)
{
	TArray<FLDPlacedUnit> DisplayUnits = Plan.After.Units;
	DisplayUnits.Sort([](const FLDPlacedUnit& Left, const FLDPlacedUnit& Right)
	                  { return Left.InstanceId < Right.InstanceId; });
	TMap<int32, int32> NextSlots;
	for (const FLDPlacedUnit& Unit : DisplayUnits)
	{
		if (const TObjectPtr<ALDUnitActor>* Actor = UnitActors.Find(Unit.InstanceId))
		{
			(*Actor)->SetPresentationSlot(NextSlots.FindOrAdd(Unit.CellId)++, GameData->GetRules().VisualMoveSeconds);
		}
	}
	for (const FLDPlacedUnit& Unit : Plan.Commit.AddedOrUpdatedUnits)
	{
		if (const TObjectPtr<ALDUnitActor>* Actor = UnitActors.Find(Unit.InstanceId))
		{
			FTransform Transform;
			TryGetCellTransform(Unit.PlayerIndex, Unit.CellId, Transform);
			(*Actor)->ApplyCommittedPlacement(Unit, Transform);
		}
	}
	// Removed actors remain alive until the composition root unregisters their combat schedules.
	OnBoardCommitted.Broadcast(Plan.Commit);
	for (uint64 Id : Plan.Commit.RemovedInstanceIds)
	{
		TObjectPtr<ALDUnitActor> Removed;
		if (UnitActors.RemoveAndCopyValue(Id, Removed) && IsValid(Removed))
		{
			Removed->DeactivateCommitted();
			Removed->Destroy();
		}
	}
	Plan.PreparedActors.Reset();
}

void ULDBoardManager::CancelPrepared(FLDBoardPlan& Plan)
{
	if (!Plan.bCommitted)
	{
		for (ALDUnitActor* Actor : Plan.PreparedActors)
		{
			if (IsValid(Actor))
			{
				Actor->Destroy();
			}
		}
		Plan.PreparedActors.Reset();
	}
}

bool ULDBoardManager::TryGetCellTransform(int32 PlayerIndex, int32 CellId, FTransform& OutTransform) const
{
	FVector Center;
	if (Geometry.GetCellOwner(CellId) != PlayerIndex || !Geometry.TryGetCellCenter(CellId, Center))
	{
		return false;
	}
	OutTransform = FTransform(Center);
	return true;
}

bool ULDBoardManager::TryGetCommittedUnitActor(uint64 InstanceId, ALDUnitActor*& OutActor) const
{
	FLDPlacedUnit Unit;
	const TObjectPtr<ALDUnitActor>* Actor = UnitActors.Find(InstanceId);
	OutActor =
	    Actor && IsValid(*Actor) && (*Actor)->IsCommitted() && TryGetUnit(InstanceId, Unit) ? Actor->Get() : nullptr;
	return OutActor != nullptr;
}

bool ULDBoardManager::TryGetUnit(uint64 InstanceId, FLDPlacedUnit& OutUnit) const
{
	for (const FLDBoardSnapshot& State : States)
	{
		if (const FLDPlacedUnit* Unit = FindUnit(State, InstanceId))
		{
			OutUnit = *Unit;
			return true;
		}
	}
	return false;
}

FLDBoardSnapshot ULDBoardManager::GetSnapshot(int32 PlayerIndex) const
{
	return States.IsValidIndex(PlayerIndex) ? States[PlayerIndex] : FLDBoardSnapshot();
}

void ULDBoardManager::Close()
{
	bClosed = true;
}
