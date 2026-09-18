#include "Verification/LDG2ProbeSubsystem.h"

#include "Battle/LDCombatService.h"
#include "Battle/LDEnemyActor.h"
#include "Battle/LDUnitActor.h"
#include "Board/LDBoardManager.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Core/LDGameMode.h"
#include "Core/LDGameState.h"
#include "Core/LDPlayerController.h"
#include "Economy/LDEconomyService.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "Misc/EngineVersion.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Net/UnrealNetwork.h"
#include "Network/LDCommandProcessor.h"
#include "Serialization/JsonSerializer.h"
#include "UI/LDGameplayWidget.h"
#include "UnrealClient.h"
#include "Widgets/SWindow.h"

DEFINE_LOG_CATEGORY_STATIC(LogLDG2Probe, Log, All);

namespace
{
	bool SameBoard(const FLDBoardSnapshot& A, const FLDBoardSnapshot& B)
	{
		if (A.MatchId != B.MatchId || A.PlayerIndex != B.PlayerIndex || A.BoardRevision != B.BoardRevision ||
		    A.Population != B.Population || A.Units.Num() != B.Units.Num())
		{
			return false;
		}
		for (const FLDPlacedUnit& Unit : A.Units)
		{
			const FLDPlacedUnit* Other = B.Units.FindByPredicate([&Unit](const FLDPlacedUnit& Candidate)
			                                                     { return Candidate.InstanceId == Unit.InstanceId; });
			if (!Other || Other->CellId != Unit.CellId || Other->UnitId != Unit.UnitId ||
			    Other->PlayerIndex != Unit.PlayerIndex ||
			    Other->MoveBlockedUntilServerSeconds != Unit.MoveBlockedUntilServerSeconds)
			{
				return false;
			}
		}
		return true;
	}
	bool SameEconomy(const FLDEconomySnapshot& A, const FLDEconomySnapshot& B)
	{
		return A.MatchId == B.MatchId && A.PlayerIndex == B.PlayerIndex && A.Gold == B.Gold && A.Stars == B.Stars &&
		       A.PaidSummonCount == B.PaidSummonCount && A.NextSummonGold == B.NextSummonGold &&
		       A.EconomyRevision == B.EconomyRevision;
	}
	TArray<uint64> InCell(const FLDBoardSnapshot& Board, int32 Cell)
	{
		TArray<uint64> IDs;
		for (const FLDPlacedUnit& Unit : Board.Units)
		{
			if (Unit.CellId == Cell)
			{
				IDs.Add(Unit.InstanceId);
			}
		}
		IDs.Sort();
		return IDs;
	}
	bool IsCommandStage(int32 Stage)
	{
		return Stage != 1 && Stage != 7 && Stage != 17 && Stage < 20;
	}
} // namespace

ALDG2ProbeState::ALDG2ProbeState()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	SetNetUpdateFrequency(20);
}

void ALDG2ProbeState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALDG2ProbeState, Stage);
	DOREPLIFETIME(ALDG2ProbeState, ActingPlayer);
	DOREPLIFETIME(ALDG2ProbeState, Command);
	DOREPLIFETIME(ALDG2ProbeState, bCheckpoint);
	DOREPLIFETIME(ALDG2ProbeState, Boards);
	DOREPLIFETIME(ALDG2ProbeState, Economies);
}

bool ULDG2ProbeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
	return false;
#else
	FString Probe;
	const UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld() &&
	       FParse::Value(FCommandLine::Get(), TEXT("P0Probe="), Probe) &&
	                     Probe == TEXT("G2") && Super::ShouldCreateSubsystem(Outer);
#endif
}

void ULDG2ProbeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	CreatedAt = FPlatformTime::Seconds();
	FParse::Value(FCommandLine::Get(), TEXT("P0ProbeOutput="), OutputDirectory);
	OutputDirectory = FPaths::ConvertRelativePathToFull(OutputDirectory);
	IFileManager::Get().MakeDirectory(*OutputDirectory, true);
}

TStatId ULDG2ProbeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(ULDG2ProbeSubsystem, STATGROUP_Tickables);
}

void ULDG2ProbeSubsystem::Check(const FString& Name, bool bPass, const FString& Detail)
{
	bFailed |= !bPass;
	TSharedPtr<FJsonObject> Entry = MakeShared<FJsonObject>();
	Entry->SetStringField(TEXT("name"), Name);
	Entry->SetBoolField(TEXT("pass"), bPass);
	Entry->SetStringField(TEXT("detail"), Detail);
	Entry->SetNumberField(TEXT("worldSeconds"), GetWorld()->GetTimeSeconds());
	Checks.Add(MakeShared<FJsonValueObject>(Entry));
	UE_LOG(LogLDG2Probe, Display, TEXT("%s %s %s"), bPass ? TEXT("PASS") : TEXT("FAIL"), *Name, *Detail);
}

void ULDG2ProbeSubsystem::BeginStage(ALDGameMode& Mode, int32 Stage)
{
	ALDG2ProbeState& Probe = *State.Get();
	Probe.Stage = Stage;
	Probe.bCheckpoint = false;
	Probe.ActingPlayer = Stage == 2 || Stage == 13 || Stage == 19 ? 1 : 0;
	Probe.Command = FLDCommand();
	StageStartedAt = FPlatformTime::Seconds();
	CheckpointAt = -1;
	for (int32 Player = 0; Player < 2; ++Player)
	{
		BeforeBoards[Player] = Mode.GetBoardManager()->GetSnapshot(Player);
		BeforeEconomies[Player] = Mode.GetEconomyService()->GetSnapshot(Player);
		RNG[Player] = Mode.GetEconomyService()->GetRandomState(Player);
	}
	AttackTimes.Reset();
	UnitIdentities.Reset();
	for (const FLDBoardSnapshot& Board : BeforeBoards)
	{
		for (const FLDPlacedUnit& Unit : Board.Units)
		{
			double NextAt = 0;
			ALDUnitActor* Actor = nullptr;
			if (Mode.GetCombatService()->TryGetUnitAttackState(Unit.InstanceId, NextAt))
			{
				AttackTimes.Add(Unit.InstanceId, NextAt);
			}
			Mode.GetBoardManager()->TryGetCommittedUnitActor(Unit.InstanceId, Actor);
			UnitIdentities.Add(Unit.InstanceId, Actor);
		}
	}
	BeforeCacheCount = Mode.GetCommandProcessor()->GetCachedResultCount(Probe.ActingPlayer);
	Probe.Command.ExpectedBoardRevision = BeforeBoards[Probe.ActingPlayer].BoardRevision;
	if (Stage == 11)
	{
		Probe.Command.CommandType = ELDCommandType::Sell;
		Probe.Command.InstanceId = InCell(BeforeBoards[0], 17)[0];
	}
	else if (Stage == 12 || Stage == 13)
	{
		Probe.Command.CommandType = ELDCommandType::Move;
		Probe.Command.InstanceId = InCell(BeforeBoards[0], Stage == 12 ? 17 : 0)[0];
		Probe.Command.DestinationCellId = Stage == 12 ? 0 : 18;
	}
	else if (Stage == 14 || Stage == 15)
	{
		const TArray<uint64> IDs = InCell(BeforeBoards[0], 11);
		Probe.Command.CommandType = ELDCommandType::Merge;
		Probe.Command.InstanceId = IDs[0];
		Probe.Command.ConsumedInstanceId0 = IDs[0];
		Probe.Command.ConsumedInstanceId1 = IDs[1];
		Probe.Command.ConsumedInstanceId2 = Stage == 14 ? InCell(BeforeBoards[0], 0)[0] : IDs[2];
	}
	if (Stage == 1)
	{
		FirstEnemy = SpawnEnemy(Mode, 70);
		Check(TEXT("first-enemy-initialized"), FirstEnemy.IsValid());
		DuplicateDeathHandle = Mode.GetCombatService()->OnEnemyDeathCommitted.AddWeakLambda(
		    this,
		    [this, WeakMode = TWeakObjectPtr<ALDGameMode>(&Mode)](const FLDCombatDeath& Death)
		    {
			    if (WeakMode.IsValid() && FirstEnemy.IsValid() &&
			        Death.EnemyId == FirstEnemy->GetRouteSnapshot().EnemyId)
			    {
				    WeakMode->GetCommandProcessor()->EnqueueCombatReward(Death);
				    WeakMode->GetCommandProcessor()->EnqueueCombatReward(Death);
			    }
		    });
	}
	Probe.ForceNetUpdate();
	UE_LOG(LogLDG2Probe, Display, TEXT("STAGE %d player=%d"), Stage, Probe.ActingPlayer);
}

ALDEnemyActor* ULDG2ProbeSubsystem::SpawnEnemy(ALDGameMode& Mode, double HP)
{
	ALDEnemyActor* Enemy = GetWorld()->SpawnActor<ALDEnemyActor>();
	FLDEnemyRow Row;
	const uint64 ID = NextEnemyId++;
	const double Now = GetWorld()->GetTimeSeconds();
	// Explicit stationary target 140cm above player0's first stack; HP1 only in the funding fixture.
	const TArray<FVector> Points = {FVector(350, 0, 0), FVector(490, 0, 0), FVector(490, 140, 0), FVector(350, 140, 0)};
	if (!Enemy || !Mode.GetGameData()->TryGetEnemyRow(
	                  TEXT("N01"), Row) ||
	                  !Enemy->InitializeRoute(GetWorld()->GetGameState<ALDGameState>()->GetMatchContext().MatchId, ID,
	                                          0, Points, 0, Now) ||
	                  !Enemy->InitializeCombat(Row, HP, ID, 1, Now) || !Mode.GetCombatService()->RegisterEnemy(*Enemy))
	{
		Check(TEXT("fixture-enemy-spawn"), false);
		return nullptr;
	}
	return Enemy;
}

void ULDG2ProbeSubsystem::Checkpoint(ALDGameMode& Mode)
{
	ALDG2ProbeState& Probe = *State.Get();
	Probe.Boards.Reset();
	Probe.Economies.Reset();
	for (int32 Player = 0; Player < 2; ++Player)
	{
		Probe.Boards.Add(Mode.GetBoardManager()->GetSnapshot(Player));
		Probe.Economies.Add(Mode.GetEconomyService()->GetSnapshot(Player));
	}
	const FLDBoardSnapshot& Board = Probe.Boards[0];
	const FLDEconomySnapshot& Economy = Probe.Economies[0];
	const int32 Stage = Probe.Stage;
	if (Stage == 16)
	{
		ALDPlayerController* Owner = Cast<ALDPlayerController>(GetWorld()->GetFirstPlayerController());
		Check(TEXT("completed-replay-has-original-request"), ResultStages.Contains(15) && LastMerge.RequestId > 0);
		if (Owner && ResultStages.Contains(15))
		{
			const FLDCommandResult Replay = Owner->SubmitServerCommand(LastMerge);
			FLDCommand Conflict = LastMerge;
			++Conflict.ExpectedBoardRevision;
			Check(TEXT("server-replay-api-original-result"),
			           Replay.ResultCode == LastMergeResult.ResultCode && Replay.EventId == LastMergeResult.EventId &&
			               Replay.NewBoardRevision == LastMergeResult.NewBoardRevision &&
			               Replay.CreatedInstanceIds == LastMergeResult.CreatedInstanceIds &&
			               Replay.RemovedInstanceIds == LastMergeResult.RemovedInstanceIds);
			const ELDCommandResultCode ConflictResult = Owner->SubmitServerCommand(Conflict).ResultCode;
			Check(TEXT("server-replay-api-conflict"),
			           Conflict.IsValidPayload() && ConflictResult == ELDCommandResultCode::RequestIdConflict,
			           FString::Printf(TEXT("actual=%d"), static_cast<int32>(ConflictResult)));
		}
	}
	if (Stage == 0)
	{
		Check(TEXT("first-summon-80-gold-C01-cell17"),
		           Board.Population == 1 && Economy.Gold == 80 && Economy.PaidSummonCount == 1 &&
		               Board.Units[0].CellId == 17 && Board.Units[0].UnitId == TEXT("C01"));
	}
	if (Stage == 1)
	{
		Check(TEXT("first-kill-both-reward-once"), Economy.Gold == 81 && Probe.Economies[1].Gold == 101 &&
		                                               FirstEnemy.IsValid() && FirstEnemy->GetCombatSnapshot().HP == 0);
	}
	if (Stage == 2)
	{
		Check(TEXT("remote-summon-retry-single-commit"), Probe.Boards[1].Population == 1 &&
		                                                     Probe.Economies[1].Gold == 81 &&
		                                                     Probe.Economies[1].PaidSummonCount == 1);
	}
	if (Stage == 5)
	{
		Check(TEXT("four-summons-after-one-reward"), Board.Population == 4 && Economy.Gold == 9 &&
		                                                 Economy.PaidSummonCount == 4 && Economy.NextSummonGold == 28);
	}
	if (Stage == 6 || Stage == 13 || Stage == 14 || Stage == 16)
	{
		for (int32 Player = 0; Player < 2; ++Player)
		{
			Check(FString::Printf(TEXT("stage%d-failure-or-retry-all-state-unchanged-p%d"), Stage, Player),
			                      SameBoard(BeforeBoards[Player], Probe.Boards[Player]) &&
			                          SameEconomy(BeforeEconomies[Player], Probe.Economies[Player]) &&
			                          RNG[Player] == Mode.GetEconomyService()->GetRandomState(Player));
		}
	}
	if (Stage == 7)
	{
		Check(TEXT("100-fixture-kills-both-reward"), Economy.Gold == 109 && Probe.Economies[1].Gold == 181);
	}
	if (Stage == 10)
	{
		Check(TEXT("seven-C01-stacks-3-3-1"), Board.Population == 7 && InCell(Board, 17).Num() == 3 &&
		                                          InCell(Board, 11).Num() == 3 && InCell(Board, 5).Num() == 1 &&
		                                          Economy.Gold == 19 && Economy.PaidSummonCount == 7);
	}
	if (Stage == 11)
	{
		const uint64 Donor = InCell(BeforeBoards[0], 5)[0];
		Check(TEXT("sell-refund17-refill-existing-donor"),
		           Board.Population == 6 && Economy.Gold == 36 && Economy.NextSummonGold == 34 &&
		               InCell(Board, 17).Contains(Donor) && InCell(Board, 5).IsEmpty());
	}
	if (Stage == 12)
	{
		Check(TEXT("whole-stack-move"), InCell(Board, 0).Num() == 3 && InCell(Board, 17).IsEmpty() &&
		                                    Board.Population == 6 && SameEconomy(BeforeEconomies[0], Economy));
	}
	if (Stage == 15)
	{
		FLDUnitRow Row;
		const TArray<uint64> Created = InCell(Board, 17);
		const FLDPlacedUnit* Result =
		    Board.Units.FindByPredicate([](const FLDPlacedUnit& Unit) { return Unit.CellId == 17; });
		Check(TEXT("merge-three-to-one-rare-first-empty17"),
		           Board.Population == 4 && Created.Num() == 1 && InCell(Board, 11).IsEmpty() && Result &&
		               Mode.GetGameData()->TryGetUnitRow(Result->UnitId, Row) &&
		               Row.Grade == TEXT("Rare") && Economy.PaidSummonCount == 7 && Economy.Gold == 36);
	}
	if (Stage == 18 || Stage == 19)
	{
		const int32 Player = Probe.ActingPlayer;
		Check(FString::Printf(
		    TEXT("recreated-hud-single-command-p%d"), Player),
		    Probe.Boards[Player].Population == BeforeBoards[Player].Population + 1 &&
		        Probe.Economies[Player].Gold == BeforeEconomies[Player].Gold - BeforeEconomies[Player].NextSummonGold &&
		        Probe.Economies[Player].PaidSummonCount == BeforeEconomies[Player].PaidSummonCount + 1);
	}
	if (Stage >= 8)
	{
		for (const FLDBoardSnapshot& Current : Probe.Boards)
		{
			for (const FLDPlacedUnit& Unit : Current.Units)
			{
				if (!AttackTimes.Contains(Unit.InstanceId))
				{
					continue;
				}
				double NextAt = 0;
				ALDUnitActor* Actor = nullptr;
				Check(FString::Printf(TEXT("stage%d-preserve-id%llu-actor-and-timer"), Stage, Unit.InstanceId),
				                      Mode.GetBoardManager()->TryGetCommittedUnitActor(Unit.InstanceId, Actor) &&
				                          Actor == UnitIdentities[Unit.InstanceId].Get() &&
				                          Mode.GetCombatService()->TryGetUnitAttackState(Unit.InstanceId, NextAt) &&
				                          NextAt == AttackTimes[Unit.InstanceId]);
			}
		}
	}
	Check(FString::Printf(TEXT("stage%d-registration-count"), Stage),
	                      Mode.GetCombatService()->GetRegisteredUnitCount() ==
	                          Probe.Boards[0].Population + Probe.Boards[1].Population);
	Probe.bCheckpoint = true;
	Probe.ForceNetUpdate();
	CheckpointAt = FPlatformTime::Seconds();
}

void ULDG2ProbeSubsystem::TickAuthority(ALDGameMode& Mode)
{
	ALDG2ProbeState& Probe = *State.Get();
	if (CheckpointAt >= 0)
	{
		if (bFailed)
		{
			Finish();
			return;
		}
		if (FPlatformTime::Seconds() - CheckpointAt > 2.0)
		{
			BeginStage(Mode, Probe.Stage + 1);
		}
		return;
	}
	if (Probe.Stage == 20)
	{
		if (FPlatformTime::Seconds() - StageStartedAt > 6)
		{
			Finish();
		}
		return;
	}
	if (Probe.Stage == 17)
	{
		if (FPlatformTime::Seconds() - StageStartedAt > 1)
		{
			Checkpoint(Mode);
		}
		return;
	}
	if (Probe.Stage == 1)
	{
		if (FirstEnemy.IsValid() && !FirstEnemy->IsCombatAlive())
		{
			Checkpoint(Mode);
		}
		return;
	}
	if (Probe.Stage == 7)
	{
		FarmDead = 0;
		int32 Alive = 0;
		for (const TWeakObjectPtr<ALDEnemyActor>& Enemy : FarmEnemies)
		{
			if (Enemy.IsValid() && Enemy->IsCombatAlive())
			{
				++Alive;
			}
			else
			{
				++FarmDead;
			}
		}
		while (FarmSpawned < 100 && Alive < 12)
		{
			FarmEnemies.Add(SpawnEnemy(Mode, 1));
			++FarmSpawned;
			++Alive;
		}
		if (FarmDead == 100)
		{
			Checkpoint(Mode);
		}
		return;
	}
	const double Elapsed = FPlatformTime::Seconds() - StageStartedAt;
	if ((Probe.Stage == 16 && Elapsed > 1) ||
	    (Elapsed > 0.5 && Mode.GetCommandProcessor()->GetCachedResultCount(Probe.ActingPlayer) > BeforeCacheCount))
	{
		Checkpoint(Mode);
	}
}

void ULDG2ProbeSubsystem::TickLocal(ALDPlayerController& Controller)
{
	ALDG2ProbeState& Probe = *State.Get();
	if (LocalStage != Probe.Stage)
	{
		Check(FString::Printf(TEXT("stage%d-response-completed-before-transition"), LocalStage),
		                      !bWaitingResult && !bActionPending);
		LocalStage = Probe.Stage;
		bWaitingResult = false;
		bActionPending = false;
		if (LocalStage == 17)
		{
			for (TObjectIterator<ULDGameplayWidget> It; It; ++It)
			{
				if (It->GetWorld() == GetWorld() && It->GetOwningPlayer() == &Controller && It->IsInViewport())
				{
					RemovedWidget = *It;
					It->RemoveFromParent();
					break;
				}
			}
			Check(TEXT("remove-live-hud-for-lifecycle-check"), RemovedWidget.IsValid());
		}
		if (IsCommandStage(LocalStage) && LocalPlayer == Probe.ActingPlayer)
		{
			if (LocalStage == 16)
			{
				Check(TEXT("completed-retransmit-uses-observed-result"),
				           ResultStages.Contains(15) && LastMerge.RequestId > 0);
				Controller.ServerRequestCommand(LastMerge);
				FLDCommand Conflict = LastMerge;
				++Conflict.ExpectedBoardRevision;
				Controller.ServerRequestCommand(Conflict);
			}
			else
			{
				LastSent = Probe.Command;
				PreviousResultId = Controller.GetLastResult().RequestId;
				bActionPending = true;
				LocalActionAt = FPlatformTime::Seconds() + .15;
				if (LocalStage == 11 || LocalStage == 15)
				{
					FVector2D CellPosition;
					Check(TEXT("select-action-stack"),
					           Controller.ProjectCellToScreen(LocalStage == 11 ? 17 : 11, CellPosition) &&
					               Controller.InputScreenPosition(CellPosition));
				}
			}
		}
	}
	if (LocalStage == 17 && !bWidgetRecreated)
	{
		for (TObjectIterator<ULDGameplayWidget> It; It; ++It)
		{
			if (It->GetWorld() == GetWorld() && It->GetOwningPlayer() == &Controller && It->IsInViewport() &&
			    *It != RemovedWidget.Get())
			{
				bWidgetRecreated = true;
				Check(TEXT("new-hud-restored-after-remove"), true);
				break;
			}
		}
	}
	if (bActionPending && FPlatformTime::Seconds() >= LocalActionAt)
	{
		bActionPending = false;
		const bool bThroughUI = LastSent.CommandType == ELDCommandType::Summon || LocalStage == 11 || LocalStage == 15;
		Check(FString::Printf(
		    TEXT("stage%d-submit-%s"), LocalStage, bThroughUI ? TEXT("slate-button") : TEXT("owned-controller")),
		         bThroughUI ? ClickAction(Controller, LastSent.CommandType) : Controller.SubmitLocalCommand(LastSent));
		bWaitingResult = true;
		if (LocalStage == 2)
		{
			Check(TEXT("remote-pending-retransmit"), Controller.RetryPendingCommand());
		}
	}
	if (bWaitingResult && !Controller.HasPendingCommand() && Controller.GetLastResult().RequestId != PreviousResultId)
	{
		bWaitingResult = false;
		ResultStages.Add(LocalStage);
		const FLDCommandResult& Result = Controller.GetLastResult();
		const ELDCommandResultCode Expected = LocalStage == 6    ? ELDCommandResultCode::InsufficientResource
		                                      : LocalStage == 13 ? ELDCommandResultCode::NotOwner
		                                      : LocalStage == 14 ? ELDCommandResultCode::MissingInstance
		                                                         : ELDCommandResultCode::Success;
		Check(FString::Printf(TEXT("stage%d-result"), LocalStage), Result.ResultCode == Expected,
		                      FString::Printf(TEXT("actual=%d expected=%d"), static_cast<int32>(Result.ResultCode),
		                                           static_cast<int32>(Expected)));
		if (LocalStage == 6)
		{
			FScreenshotRequest::RequestScreenshot(OutputDirectory / TEXT("stage-6-rejected.png"), true, false);
		}
		if (LocalStage == 15)
		{
			LastMerge = LastSent;
			LastMerge.ConnectionEpoch = Result.ConnectionEpoch;
			LastMerge.RequestId = Result.RequestId;
			LastMergeResult = Result;
		}
	}
	if (Probe.bCheckpoint && InspectedStage != LocalStage && Probe.Boards.Num() == 2 && Probe.Economies.Num() == 2 &&
	    Controller.GetBoardSnapshot().BoardRevision == Probe.Boards[LocalPlayer].BoardRevision &&
	    Controller.GetEconomySnapshot().EconomyRevision == Probe.Economies[LocalPlayer].EconomyRevision)
	{
		bool bActorsAgree = true;
		TSet<uint64> ExpectedIDs;
		for (const FLDBoardSnapshot& Board : Probe.Boards)
		{
			for (const FLDPlacedUnit& Expected : Board.Units)
			{
				ExpectedIDs.Add(Expected.InstanceId);
				bool bFound = false;
				for (TActorIterator<ALDUnitActor> It(GetWorld()); It; ++It)
				{
					const FLDPlacedUnit& Actual = It->GetPlacement();
					bFound |= It->IsCommitted() && Actual.InstanceId == Expected.InstanceId &&
					          Actual.CellId == Expected.CellId && Actual.UnitId == Expected.UnitId;
				}
				bActorsAgree &= bFound;
			}
		}
		int32 CommittedCount = 0;
		TSet<uint64> ActualIDs;
		for (TActorIterator<ALDUnitActor> It(GetWorld()); It; ++It)
		{
			if (It->IsCommitted())
			{
				++CommittedCount;
				ActualIDs.Add(It->GetPlacement().InstanceId);
			}
		}
		bActorsAgree &= CommittedCount == ExpectedIDs.Num() && ActualIDs.Num() == ExpectedIDs.Num();
		for (uint64 ID : ActualIDs)
		{
			bActorsAgree &= ExpectedIDs.Contains(ID);
		}
		// Actor replication and the owner snapshot may arrive in either order; wait for agreement until timeout.
		if (bActorsAgree)
		{
			InspectedStage = LocalStage;
			InspectedStages.Add(LocalStage);
			Check(FString::Printf(TEXT("stage%d-owner-snapshot-and-both-boards-agree"), LocalStage),
			                      SameBoard(Controller.GetBoardSnapshot(), Probe.Boards[LocalPlayer]) &&
			                          SameEconomy(Controller.GetEconomySnapshot(), Probe.Economies[LocalPlayer]));
			if (LocalStage == 0 || LocalStage == 10 || LocalStage == 15 || LocalStage == 19)
			{
				FScreenshotRequest::RequestScreenshot(OutputDirectory /
				                                      FString::Printf(TEXT("stage-%d.png"), LocalStage), true, false);
			}
		}
	}
	if (LocalStage == 20 && GetWorld()->GetNetMode() == NM_Client)
	{
		Finish();
	}
}

bool ULDG2ProbeSubsystem::ClickAction(ALDPlayerController& Controller, ELDCommandType Type)
{
	FBox2D Rect;
	if (!Controller.GetActionScreenRect(Type, Rect) || !FSlateApplication::IsInitialized() ||
	    !GetWorld()->GetGameViewport() || !GetWorld()->GetGameViewport()->GetWindow())
	{
		return false;
	}
	FVector2D Absolute;
	USlateBlueprintLibrary::ScreenToWidgetAbsolute(this, Rect.GetCenter(), Absolute);
	const TSet<FKey> Pressed = {EKeys::LeftMouseButton};
	const FPointerEvent Down(0, Absolute, Absolute, Pressed, EKeys::LeftMouseButton, 0, FModifierKeysState());
	const FPointerEvent Up(0, Absolute, Absolute, TSet<FKey>(), EKeys::LeftMouseButton, 0, FModifierKeysState());
	const int32 SelectionBefore = Controller.GetSelectedCellId();
	FSlateApplication& Slate = FSlateApplication::Get();
	Slate.ProcessMouseMoveEvent(Down, true);
	Slate.ProcessMouseButtonDownEvent(GetWorld()->GetGameViewport()->GetWindow()->GetNativeWindow(), Down);
	Slate.ProcessMouseButtonUpEvent(Up);
	Check(FString::Printf(TEXT("stage%d-hud-click-does-not-select-board"), LocalStage),
	                      Controller.GetSelectedCellId() == SelectionBefore);
	return Controller.HasPendingCommand() || Controller.GetLastResult().RequestId != PreviousResultId;
}

void ULDG2ProbeSubsystem::Tick(float DeltaTime)
{
	if (bFinished || !GetWorld()->HasBegunPlay())
	{
		return;
	}
	if (FPlatformTime::Seconds() - CreatedAt > 190)
	{
		Check(TEXT("runtime-timeout"), false, FString::Printf(TEXT("stage=%d"), LocalStage));
		Finish();
		return;
	}
	ALDPlayerController* Controller = Cast<ALDPlayerController>(GetWorld()->GetFirstPlayerController());
	if (!Controller || !Controller->IsLocalBoardReady() || !Controller->IsGameplaySnapshotReady())
	{
		return;
	}
	LocalPlayer = Controller->GetLocalParticipantIndex();
	ALDGameMode* Mode = GetWorld()->GetAuthGameMode<ALDGameMode>();
	if (!State.IsValid())
	{
		for (TActorIterator<ALDG2ProbeState> It(GetWorld()); It; ++It)
		{
			State = *It;
		}
		if (!State.IsValid() && Mode && Mode->CanAcceptCommands())
		{
			State = GetWorld()->SpawnActor<ALDG2ProbeState>();
			BeginStage(*Mode, 0);
		}
	}
	if (!State.IsValid() || State->Stage < 0)
	{
		return;
	}
	FrameMilliseconds.Add(DeltaTime * 1000);
	TickLocal(*Controller);
	if (Mode)
	{
		TickAuthority(*Mode);
	}
}

void ULDG2ProbeSubsystem::Finish()
{
	if (bFinished)
	{
		return;
	}
	bFinished = true;
	Check(TEXT("all-stages-complete"), LocalStage == 20 && InspectedStage == 19 && InspectedStages.Num() == 20,
	           FString::Printf(TEXT("inspected=%d/20"), InspectedStages.Num()));
	Check(TEXT("hud-recreation-observed"), bWidgetRecreated);
	for (int32 Stage = 0; Stage < 20; ++Stage)
	{
		const int32 Player = Stage == 2 || Stage == 13 || Stage == 19 ? 1 : 0;
		if (IsCommandStage(Stage) && Stage != 16 && Player == LocalPlayer)
		{
			Check(FString::Printf(TEXT("required-command-result-stage%d"), Stage), ResultStages.Contains(Stage));
		}
	}
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("result"), bFailed ? TEXT("Fail") : TEXT("Pass"));
	Result->SetStringField(TEXT("scope"),
	                            TEXT("Rendered G2 two-process fixture: owned RPC commands, stationary HP70 target, 100 HP1 funding targets; not ten-wave balance or physical input"));
	Result->SetStringField(TEXT("engine"), FEngineVersion::Current().ToString());
	Result->SetNumberField(TEXT("localPlayerIndex"), LocalPlayer);
	Result->SetArrayField(TEXT("checks"), Checks);
	FrameMilliseconds.Sort();
	if (!FrameMilliseconds.IsEmpty())
	{
		Result->SetNumberField(TEXT("frameCount"), FrameMilliseconds.Num());
		Result->SetNumberField(TEXT("frameMsP95"),
		                            FrameMilliseconds[FMath::FloorToInt((FrameMilliseconds.Num() - 1) * .95)]);
	}
	FString Json;
	FJsonSerializer::Serialize(Result.ToSharedRef(), TJsonWriterFactory<>::Create(&Json));
	FFileHelper::SaveStringToFile(Json, *(OutputDirectory / TEXT("result.json")));
	FPlatformMisc::RequestExit(false);
}

void ULDG2ProbeSubsystem::Deinitialize()
{
	if (ALDGameMode* Mode = GetWorld()->GetAuthGameMode<ALDGameMode>(); Mode && Mode->GetCombatService())
	{
		Mode->GetCombatService()->OnEnemyDeathCommitted.Remove(DuplicateDeathHandle);
	}
	State.Reset();
	FirstEnemy.Reset();
	FarmEnemies.Reset();
	Super::Deinitialize();
}
