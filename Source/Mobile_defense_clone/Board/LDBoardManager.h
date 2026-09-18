#pragma once

#include "CoreMinimal.h"
#include "Board/LDBoardGeometry.h"
#include "Board/LDBoardTypes.h"
#include "Data/LDGameData.h"
#include "Data/LDMatchTypes.h"
#include "Network/LDCommandTypes.h"
#include "UObject/Object.h"
#include "LDBoardManager.generated.h"

class ALDUnitActor;

struct FLDBoardPlan
{
	FLDBoardSnapshot After;
	FLDBoardCommit Commit;
	int32 ExpectedRevision = 0;
	uint64 ExpectedNextInstanceId = 0;
	TArray<TObjectPtr<ALDUnitActor>> PreparedActors;
	bool bCommitted = false;
};

// Native preparation adapter isolates fallible actor/asset work from the atomic state commit.
using FLDPrepareUnit = TFunction<ALDUnitActor*(UWorld&, const FLDPlacedUnit&, const FLDUnitRow&, const FTransform&)>;

UCLASS()
class MOBILE_DEFENSE_CLONE_API ULDBoardManager : public UObject
{
	GENERATED_BODY()

public:
	bool Initialize(UWorld& World, const FLDMatchContext& Context, ULDGameData& Data, FLDPrepareUnit PrepareUnit = {});
	ELDCommandResultCode ValidateCommand(const FLDParticipantContext& Context, const FLDCommand& Command,
	                                     double ServerSeconds) const;
	ELDCommandResultCode TryPrepare(const FLDParticipantContext& Context, const FLDCommand& Command, FName ResultUnitId,
	                                double ServerSeconds, FLDBoardPlan& OutPlan);
	bool ValidatePrepared(const FLDBoardPlan& Plan) const;
	void CommitPrepared(FLDBoardPlan& Plan);
	void PublishPrepared(FLDBoardPlan& Plan);
	void CancelPrepared(FLDBoardPlan& Plan);
	bool TryGetCellTransform(int32 PlayerIndex, int32 CellId, FTransform& OutTransform) const;
	bool TryGetCommittedUnitActor(uint64 InstanceId, ALDUnitActor*& OutActor) const;
	bool TryGetUnit(uint64 InstanceId, FLDPlacedUnit& OutUnit) const;
	FLDBoardSnapshot GetSnapshot(int32 PlayerIndex) const;
	void Close();
	FLDBoardCommitted OnBoardCommitted;

private:
	int32 FindPlacementCell(const FLDBoardSnapshot& State, FName UnitId) const;
	bool IsValidSnapshot(const FLDBoardSnapshot& State) const;
	UPROPERTY()
	TObjectPtr<ULDGameData> GameData;
	UPROPERTY()
	TMap<uint64, TObjectPtr<ALDUnitActor>> UnitActors;
	TWeakObjectPtr<UWorld> ServerWorld;
	FLDMatchContext MatchContext;
	FLDBoardGeometry Geometry;
	TArray<FLDBoardSnapshot> States;
	FLDPrepareUnit UnitPreparation;
	uint64 NextInstanceId = 1;
	bool bClosed = false;
};
