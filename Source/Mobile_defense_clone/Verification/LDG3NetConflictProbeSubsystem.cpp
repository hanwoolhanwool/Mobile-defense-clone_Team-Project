#include "Verification/LDG3NetConflictProbeSubsystem.h"

#include "Battle/LDCombatService.h"
#include "Battle/LDUnitActor.h"
#include "Board/LDBoardManager.h"
#include "Core/LDGameMode.h"
#include "Core/LDGameState.h"
#include "Core/LDPlayerController.h"
#include "Core/LDPlayerState.h"
#include "CoreGlobals.h"
#include "Data/LDGameData.h"
#include "Economy/LDEconomyService.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "Misc/EngineVersion.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Net/UnrealNetwork.h"
#include "Network/LDCommandProcessor.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogLDG3NetConflict, Log, All);

namespace
{
	bool IsNetConflictEnabled()
	{
#if UE_BUILD_SHIPPING
		return false;
#else
		FString Probe;
		return FParse::Value(FCommandLine::Get(), TEXT("P0Probe="), Probe) && Probe == TEXT("G3NetConflict");
#endif
	}

	template <typename ValueType> bool Same(const ValueType& A, const ValueType& B)
	{
		return ValueType::StaticStruct()->CompareScriptStruct(&A, &B, 0);
	}

	bool SameIds(TArray<uint64> A, TArray<uint64> B)
	{
		A.Sort();
		B.Sort();
		return A == B;
	}

	TArray<uint64> BoardIds(const FLDBoardSnapshot& Board)
	{
		TArray<uint64> Ids;
		for (const FLDPlacedUnit& Unit : Board.Units)
		{
			Ids.Add(Unit.InstanceId);
		}
		return Ids;
	}

	TArray<TSharedPtr<FJsonValue>> JsonIds(const TArray<uint64>& Ids)
	{
		TArray<TSharedPtr<FJsonValue>> Result;
		for (uint64 Id : Ids)
		{
			Result.Add(MakeShared<FJsonValueString>(LexToString(Id)));
		}
		return Result;
	}
} // namespace

ALDG3NetConflictPeer::ALDG3NetConflictPeer()
{
	bReplicates = true;
	bOnlyRelevantToOwner = true;
	SetNetUpdateFrequency(30);
}

void ALDG3NetConflictPeer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALDG3NetConflictPeer, Phase);
	DOREPLIFETIME(ALDG3NetConflictPeer, MatchId);
	DOREPLIFETIME(ALDG3NetConflictPeer, BeforeBoard);
	DOREPLIFETIME(ALDG3NetConflictPeer, BeforeEconomy);
	DOREPLIFETIME(ALDG3NetConflictPeer, AfterBoard);
	DOREPLIFETIME(ALDG3NetConflictPeer, AfterEconomy);
	DOREPLIFETIME(ALDG3NetConflictPeer, First);
	DOREPLIFETIME(ALDG3NetConflictPeer, Second);
	DOREPLIFETIME(ALDG3NetConflictPeer, ServerResponses);
	DOREPLIFETIME(ALDG3NetConflictPeer, bServerPassed);
	DOREPLIFETIME(ALDG3NetConflictPeer, bServerCleaned);
}

void ALDG3NetConflictPeer::ServerReportObservation_Implementation(int32 ObservedPhase, FGuid ObservedMatch,
                                                                  int32 BoardRevision, bool bPass)
{
	if (!IsNetConflictEnabled() || ObservedMatch != MatchId || ObservedPhase != Phase ||
	    BoardRevision != (Phase <= 2 ? BeforeBoard.BoardRevision : AfterBoard.BoardRevision))
	{
		return;
	}
	bClientPassed &= bPass;
	if (Phase == 1)
	{
		bClientReady = true;
	}
	else if (Phase == 3)
	{
		bClientVerified = true;
	}
	else if (Phase == 4)
	{
		bClientCleaned = true;
	}
	else if (Phase == 5)
	{
		bClientFinished = true;
	}
}

bool ULDG3NetConflictProbeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return IsNetConflictEnabled() && Super::ShouldCreateSubsystem(Outer);
}

void ULDG3NetConflictProbeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	CreatedAt = FPlatformTime::Seconds();
	FParse::Value(FCommandLine::Get(), TEXT("P0ProbeOutput="), OutputDirectory);
	FParse::Value(FCommandLine::Get(), TEXT("P0TimeoutSeconds="), TimeoutSeconds);
	TimeoutSeconds = FMath::Clamp(TimeoutSeconds, 15, 180);
	if (OutputDirectory.IsEmpty())
	{
		OutputDirectory = FPaths::ProjectSavedDir() / TEXT("P0Runs/G3NetConflict") / FGuid::NewGuid().ToString();
	}
	OutputDirectory = FPaths::ConvertRelativePathToFull(OutputDirectory);
	// The runner may already have opened stdout/stderr here. Preserve previous owned evidence explicitly.
	if (FPaths::FileExists(OutputDirectory / TEXT("result.json")) ||
	                       FPaths::FileExists(OutputDirectory / TEXT("progress.json")))
	{
		bFailed = true;
		UE_LOG(LogLDG3NetConflict, Error, TEXT("Earlier fixture evidence already exists: %s"), *OutputDirectory);
		ExitAt = CreatedAt + .25;
		return;
	}
	bOutputAvailable = IFileManager::Get().MakeDirectory(*OutputDirectory, true);
	if (!bOutputAvailable)
	{
		bFailed = true;
		ExitAt = CreatedAt + .25;
	}
}

bool ULDG3NetConflictProbeSubsystem::IsTickable() const
{
	return !IsTemplate() && !bFinished && IsNetConflictEnabled();
}

TStatId ULDG3NetConflictProbeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(ULDG3NetConflictProbeSubsystem, STATGROUP_Tickables);
}

UWorld* ULDG3NetConflictProbeSubsystem::GetTickableGameObjectWorld() const
{
	return GetWorld();
}

void ULDG3NetConflictProbeSubsystem::Check(const FString& Name, bool bPass, const FString& Detail)
{
	bFailed |= !bPass;
	TSharedPtr<FJsonObject> Item = MakeShared<FJsonObject>();
	Item->SetStringField(TEXT("name"), Name);
	Item->SetBoolField(TEXT("pass"), bPass);
	Item->SetStringField(TEXT("detail"), Detail);
	Item->SetNumberField(TEXT("wallSeconds"), FPlatformTime::Seconds() - CreatedAt);
	Checks.Add(MakeShared<FJsonValueObject>(Item));
	UE_LOG(LogLDG3NetConflict, Display, TEXT("%s %s %s"), bPass ? TEXT("PASS") : TEXT("FAIL"), *Name, *Detail);
}

bool ULDG3NetConflictProbeSubsystem::InstallObserver(ALDPlayerController& Controller)
{
#if !UE_BUILD_SHIPPING
	if (AActor::ProcessEventDelegate.IsBound())
	{
		Check(TEXT("exclusive-read-only-RPC-observer"), false, TEXT("Existing observer left untouched"));
		return false;
	}
	ObservedController = &Controller;
	AActor::ProcessEventDelegate.BindUObject(this, &ULDG3NetConflictProbeSubsystem::ObserveEvent);
	EventHandle = AActor::ProcessEventDelegate.GetHandle();
	bEventObserverInstalled = true;
	bObserversRestored = false;
	return true;
#else
	return false;
#endif
}

bool ULDG3NetConflictProbeSubsystem::ObserveEvent(AActor* Actor, UFunction* Function, void* Parameters)
{
	if (Actor != ObservedController.Get() || !Function || !Parameters || !bActivated)
	{
		return false;
	}
	const bool bRequest = Function->GetFName() == GET_FUNCTION_NAME_CHECKED(ALDPlayerController, ServerRequestCommand);
	const bool bResponse = Function->GetFName() == GET_FUNCTION_NAME_CHECKED(ALDPlayerController, ClientCommandResult);
	if ((!bRequest && !bResponse) || (bRequest && LocalPlayerIndex != 0))
	{
		return false;
	}
	const FStructProperty* Property =
	    FindFProperty<FStructProperty>(Function, bRequest ? TEXT("Command") : TEXT("Result"));
	if (!Property || Property->Struct != (bRequest ? FLDCommand::StaticStruct() : FLDCommandResult::StaticStruct()))
	{
		Check(TEXT("RPC-parameter-shape"), false);
		return false;
	}
	TSharedPtr<FJsonObject> Item = MakeShared<FJsonObject>();
	Item->SetStringField(TEXT("kind"), bRequest ? TEXT("server-arrival")
	                                            : LocalPlayerIndex == 0
	                          ? TEXT("server-response-dispatch")
	                          : TEXT("client-response-arrival"));
	Item->SetStringField(TEXT("frame"), LexToString(GFrameCounter));
	Item->SetNumberField(TEXT("worldSeconds"), Actor->GetWorld()->GetTimeSeconds());
	Item->SetNumberField(TEXT("wallSeconds"), FPlatformTime::Seconds() - CreatedAt);
	if (bRequest)
	{
		const FLDCommand& Command = *Property->ContainerPtrToValuePtr<FLDCommand>(Parameters);
		if (Requests.Num() >= 2)
		{
			Check(TEXT("no-unexpected-extra-command"), false);
			return false;
		}
		Requests.Add(Command);
		RequestFrames.Add(GFrameCounter);
		Item->SetNumberField(TEXT("requestId"), Command.RequestId);
		Item->SetStringField(TEXT("epoch"), LexToString(Command.ConnectionEpoch));
		Item->SetNumberField(TEXT("expectedBoardRevision"), Command.ExpectedBoardRevision);
		Item->SetArrayField(TEXT("materials"), JsonIds({Command.ConsumedInstanceId0, Command.ConsumedInstanceId1,
		                                                Command.ConsumedInstanceId2}));
	}
	else
	{
		const FLDCommandResult& Result = *Property->ContainerPtrToValuePtr<FLDCommandResult>(Parameters);
		if (Responses.Num() >= 2)
		{
			Check(TEXT("no-unexpected-extra-response"), false);
			return false;
		}
		Responses.Add(Result);
		Item->SetStringField(TEXT("matchId"), Result.MatchId.ToString());
		Item->SetNumberField(TEXT("requestId"), Result.RequestId);
		Item->SetStringField(TEXT("epoch"), LexToString(Result.ConnectionEpoch));
		Item->SetNumberField(TEXT("code"), static_cast<int32>(Result.ResultCode));
		Item->SetNumberField(TEXT("boardRevision"), Result.NewBoardRevision);
		Item->SetNumberField(TEXT("economyRevision"), Result.EconomyRevision);
		Item->SetStringField(TEXT("eventId"), LexToString(Result.EventId));
		Item->SetArrayField(TEXT("created"), JsonIds(Result.CreatedInstanceIds));
		Item->SetArrayField(TEXT("removed"), JsonIds(Result.RemovedInstanceIds));
	}
	Wire.Add(MakeShared<FJsonValueObject>(Item));
	return false; // Never intercept, consume, replace, or manufacture a product RPC/response.
}

bool ULDG3NetConflictProbeSubsystem::PrepareFixture(ALDGameMode& Mode, ALDPlayerController& Remote)
{
	ULDBoardManager* Board = Mode.GetBoardManager();
	ULDEconomyService* Economy = Mode.GetEconomyService();
	const ALDPlayerState* PlayerState = Remote.GetPlayerState<ALDPlayerState>();
	if (!Board || !Economy || !PlayerState || !Mode.GetWaveDirector() || !Mode.CanAcceptCommands())
	{
		return false;
	}
	const FLDParticipantContext Context = PlayerState->GetParticipantContext();
	Check(TEXT("actual-remote-owned-participant"), Context.IsValid() && Context.PlayerIndex == 1 &&
	                                                   !Remote.IsLocalController() &&
	                                                   Remote.GetNetConnection() != nullptr);
	Check(TEXT("fresh-empty-boards-and-caches"), Board->GetSnapshot(0).Population == 0 &&
	                                                 Board->GetSnapshot(1).Population == 0 &&
	                                                 Mode.GetCommandProcessor()->GetCachedResultCount(0) == 0 &&
	                                                 Mode.GetCommandProcessor()->GetCachedResultCount(1) == 0);
	if (bFailed)
	{
		return false;
	}
	MatchId = Context.MatchId;
	HostBeforeBoard = Board->GetSnapshot(0);
	HostBeforeEconomy = Economy->GetSnapshot(0);
	HostRngBefore = Economy->GetRandomState(0);
	const FLDEconomySnapshot InitialEconomy = Economy->GetSnapshot(1);
	RngBefore = Economy->GetRandomState(1);
	for (int32 Index = 0; Index < 3; ++Index)
	{
		FLDCommand Command;
		Command.ConnectionEpoch = Context.ConnectionEpoch;
		Command.RequestId = Index + 1;
		Command.ExpectedBoardRevision = Board->GetSnapshot(1).BoardRevision;
		FLDBoardPlan Plan;
		if (Board->TryPrepare(Context, Command,
		                      TEXT("C01"), GetWorld()->GetTimeSeconds(), Plan) != ELDCommandResultCode::Success ||
		                      !Board->ValidatePrepared(Plan))
		{
			Board->CancelPrepared(Plan);
			Check(TEXT("free-C01-board-fixture-preparation"), false);
			return false;
		}
		Board->CommitPrepared(Plan);
		Board->PublishPrepared(Plan);
	}
	Check(TEXT("explicit-free-fixture-no-economy-RNG-or-command-history"),
	    Same(InitialEconomy, Economy->GetSnapshot(1)) && RngBefore == Economy->GetRandomState(1) &&
	        Mode.GetCommandProcessor()->GetCachedResultCount(1) == 0,
	    TEXT("Three C01 units authored using public Board prepare/commit/publish; no paid summons or learner actions"));
	FActorSpawnParameters Params;
	Params.Owner = &Remote;
	ALDG3NetConflictPeer* Evidence = GetWorld()->SpawnActor<ALDG3NetConflictPeer>(Params);
	if (!Evidence)
	{
		return false;
	}
	Peer = Evidence;
	Evidence->MatchId = MatchId;
	Evidence->BeforeBoard = Board->GetSnapshot(1);
	Evidence->BeforeEconomy = InitialEconomy;
	const TArray<uint64> Materials = BoardIds(Evidence->BeforeBoard);
	Check(TEXT("three-distinct-committed-C01-materials"),
	           Materials.Num() == 3 && SameIds(Materials, {1, 2, 3}) && Evidence->BeforeBoard.Population == 3 &&
	               Evidence->BeforeBoard.BoardRevision == 3 && ActorSetMatches(Evidence->BeforeBoard));
	for (const FLDPlacedUnit& Unit : Evidence->BeforeBoard.Units)
	{
		Check(TEXT("fixture-C01-same-owner-stack"),
		           Unit.UnitId == TEXT("C01") && Unit.PlayerIndex == 1 &&
		                               Unit.CellId == Evidence->BeforeBoard.Units[0].CellId);
	}
	if (bFailed || Materials.Num() != 3)
	{
		return false;
	}
	Evidence->First.ConnectionEpoch = Context.ConnectionEpoch;
	Evidence->First.RequestId = 1;
	Evidence->First.CommandType = ELDCommandType::Merge;
	Evidence->First.ExpectedBoardRevision = 3;
	Evidence->First.InstanceId = Materials[0];
	Evidence->First.ConsumedInstanceId0 = Materials[0];
	Evidence->First.ConsumedInstanceId1 = Materials[1];
	Evidence->First.ConsumedInstanceId2 = Materials[2];
	Evidence->Second = Evidence->First;
	Evidence->Second.RequestId = 2;
	// Independent contract oracle: C01 merges into one of exactly R01..R04 using ONE RNG draw.
	TArray<FName> RareTypes;
	for (const auto& Pair : Mode.GetGameData()->GetUnits())
	{
		if (Pair.Value.bEnabledInP0 && Pair.Value.Grade == TEXT("Rare"))
		{
			RareTypes.Add(Pair.Key);
		}
	}
	RareTypes.Sort(FNameLexicalLess());
	Check(TEXT("independent-P0-rare-candidate-contract"),
	           RareTypes == TArray<FName>({TEXT("R01"), TEXT("R02"), TEXT("R03"), TEXT("R04")}));
	if (RareTypes.Num() != 4)
	{
		return false;
	}
	FRandomStream Expected(RngBefore);
	const int32 Draw = Expected.RandRange(0, 3);
	ExpectedRare = RareTypes[Draw];
	ExpectedRngAfter = Expected.GetCurrentSeed();
	ObservedBoard = Board;
	BoardHandle = Board->OnBoardCommitted.AddUObject(this, &ULDG3NetConflictProbeSubsystem::OnBoardCommitted);
	bObserversRestored = false;
	bFixturePrepared = true;
	Evidence->ForceNetUpdate();
	return !bFailed;
}

void ULDG3NetConflictProbeSubsystem::OnBoardCommitted(const FLDBoardCommit& Commit)
{
	if (!bFixturePrepared || Commit.MatchId != MatchId)
	{
		return;
	}
	if (MergeCommits.Num() < 3)
	{
		MergeCommits.Add(Commit);
	}
	else
	{
		Check(TEXT("no-unexpected-board-publications"), false);
	}
}

bool ULDG3NetConflictProbeSubsystem::ActorSetMatches(const FLDBoardSnapshot& Board) const
{
	TArray<uint64> Actual;
	for (TActorIterator<ALDUnitActor> It(GetWorld()); It; ++It)
	{
		if (It->IsCommitted())
		{
			if (It->GetPlacement().PlayerIndex != 1)
			{
				return false; // The host deliberately has no units in this isolated scenario.
			}
			const FLDPlacedUnit* Expected = Board.Units.FindByPredicate(
			    [&](const FLDPlacedUnit& Unit) { return Unit.InstanceId == It->GetPlacement().InstanceId; });
			if (!Expected || !Same(*Expected, It->GetPlacement()))
			{
				return false;
			}
			Actual.Add(It->GetPlacement().InstanceId);
		}
	}
	return SameIds(Actual, BoardIds(Board)) && Actual.Num() == Board.Population;
}

void ULDG3NetConflictProbeSubsystem::VerifyServer(ALDGameMode& Mode)
{
	ALDG3NetConflictPeer& Evidence = *Peer;
	Check(TEXT("two-actual-owned-RPC-arrivals"),
	           Requests.Num() == 2 && Same(Requests[0], Evidence.First) && Same(Requests[1], Evidence.Second));
	Check(TEXT("one-Success-one-StaleBoard-response"), Responses.Num() == 2 && Responses[0].RequestId == 1 &&
	                                                       Responses[0].ResultCode == ELDCommandResultCode::Success &&
	                                                       Responses[1].RequestId == 2 &&
	                                                       Responses[1].ResultCode == ELDCommandResultCode::StaleBoard);
	Evidence.AfterBoard = Mode.GetBoardManager()->GetSnapshot(1);
	Evidence.AfterEconomy = Mode.GetEconomyService()->GetSnapshot(1);
	Evidence.ServerResponses = Responses;
	Check(TEXT("population-three-to-one-and-single-board-revision"), Evidence.AfterBoard.Population == 1 &&
	                                                                     Evidence.AfterBoard.Units.Num() == 1 &&
	                                                                     Evidence.AfterBoard.BoardRevision == 4);
	FLDEconomySnapshot ExpectedEconomy = Evidence.BeforeEconomy;
	++ExpectedEconomy.EconomyRevision;
	Check(TEXT("gold-stars-paid-count-price-unchanged-single-economy-revision"),
	           Same(ExpectedEconomy, Evidence.AfterEconomy));
	Check(TEXT("exactly-one-independent-RNG-draw"), Mode.GetEconomyService()->GetRandomState(1) == ExpectedRngAfter,
	           FString::Printf(TEXT("before=%d expected=%d actual=%d"), RngBefore, ExpectedRngAfter,
	                                Mode.GetEconomyService()->GetRandomState(1)));
	Check(TEXT("single-created-rare-ID4"), Evidence.AfterBoard.Units.Num() == 1 &&
	                                           Evidence.AfterBoard.Units[0].InstanceId == 4 &&
	                                           Evidence.AfterBoard.Units[0].UnitId == ExpectedRare);
	Check(TEXT("one-merge-publication-exact-materials"),
	           MergeCommits.Num() == 1 && MergeCommits[0].PlayerIndex == 1 &&
	               MergeCommits[0].ChangeReason == ELDBoardChangeReason::Merge && MergeCommits[0].BoardRevision == 4 &&
	               SameIds(MergeCommits[0].RemovedInstanceIds, {1, 2, 3}) &&
	               MergeCommits[0].AddedOrUpdatedUnits.Num() == 1 &&
	               MergeCommits[0].AddedOrUpdatedUnits[0].InstanceId == 4);
	if (Responses.Num() == 2)
	{
		Check(TEXT("wire-single-effect-and-failure-empty-effects"),
		           Responses[0].EventId != 0 && SameIds(Responses[0].CreatedInstanceIds, {4}) &&
		               SameIds(Responses[0].RemovedInstanceIds, {1, 2, 3}) && Responses[0].MovedInstanceIds.IsEmpty() &&
		               Responses[1].EventId == 0 && Responses[1].CreatedInstanceIds.IsEmpty() &&
		               Responses[1].RemovedInstanceIds.IsEmpty() && Responses[1].MovedInstanceIds.IsEmpty());
		for (const FLDCommandResult& Result : Responses)
		{
			Check(TEXT("wire-context-and-final-revisions"),
			           Result.MatchId == MatchId && Result.ConnectionEpoch == Evidence.First.ConnectionEpoch &&
			               Result.NewBoardRevision == 4 && Result.EconomyRevision == ExpectedEconomy.EconomyRevision);
		}
	}
	Check(TEXT("server-exact-remaining-actor-and-combat-registration"),
	           ActorSetMatches(Evidence.AfterBoard) && Mode.GetCombatService()->GetRegisteredUnitCount() == 1);
	Check(TEXT("two-cached-results-no-host-side-effect"),
	           Mode.GetCommandProcessor()->GetCachedResultCount(1) == 2 &&
	               Mode.GetCommandProcessor()->GetCachedResultCount(0) == 0 &&
	               Same(HostBeforeBoard, Mode.GetBoardManager()->GetSnapshot(0)) &&
	               Same(HostBeforeEconomy, Mode.GetEconomyService()->GetSnapshot(0)) &&
	               HostRngBefore == Mode.GetEconomyService()->GetRandomState(0));
	Check(TEXT("scenario-completed-in-real-Preparing"),
	           GetWorld()->GetGameState<ALDGameState>()->GetPhase() == ELDMatchPhase::Preparing);
	Evidence.bServerPassed = !bFailed;
	Evidence.Phase = 3;
	Evidence.ForceNetUpdate();
}

void ULDG3NetConflictProbeSubsystem::VerifyClient(ALDPlayerController& Controller)
{
	const ALDG3NetConflictPeer& Evidence = *Peer;
	Check(TEXT("two-distinct-request-IDs-sent-same-client-tick"),
	           FirstSendFrame != 0 && FirstSendFrame == SecondSendFrame && Evidence.First.RequestId == 1 &&
	               Evidence.Second.RequestId == 2);
	Check(TEXT("both-actual-client-response-arrivals-match-server-in-all-fields"),
	           Responses.Num() == 2 && Evidence.ServerResponses.Num() == 2 &&
	               Same(Responses[0], Evidence.ServerResponses[0]) && Same(Responses[1], Evidence.ServerResponses[1]));
	Check(TEXT("client-one-success-one-stale-board"), Responses.Num() == 2 &&
	                                                      Responses[0].ResultCode == ELDCommandResultCode::Success &&
	                                                      Responses[1].ResultCode == ELDCommandResultCode::StaleBoard);
	Check(TEXT("client-owner-snapshots-converged"), Same(Controller.GetBoardSnapshot(), Evidence.AfterBoard) &&
	                                                    Same(Controller.GetEconomySnapshot(), Evidence.AfterEconomy));
	Check(TEXT("client-three-old-actors-replaced-by-exact-one"), ActorSetMatches(Evidence.AfterBoard));
	Check(TEXT("server-independent-state-oracles-passed"), Evidence.bServerPassed);
	Check(TEXT("raw-burst-does-not-pretend-to-test-Pending-UI"), !Controller.HasPendingCommand(),
	           TEXT("Fixture calls owned ServerRequestCommand directly; unrequested replies are observed before the normal client guard"));
}

void ULDG3NetConflictProbeSubsystem::TickAuthority(ALDGameMode& Mode)
{
	if (!Peer.IsValid())
	{
		return;
	}
	ALDG3NetConflictPeer& Evidence = *Peer;
	if (!Evidence.bClientPassed)
	{
		FailAndExit(TEXT("remote-observation-failed"));
		return;
	}
	if (Evidence.Phase == 1 && Evidence.bClientReady)
	{
		Evidence.Phase = 2;
		Evidence.ForceNetUpdate();
	}
	else if (Evidence.Phase == 2 && Requests.Num() == 2 && Responses.Num() == 2)
	{
		VerifyServer(Mode);
	}
	else if (Evidence.Phase == 3 && Evidence.bClientVerified)
	{
		Mode.AbortMatch(TEXT("G3NetConflict fixture complete: explicit service cleanup"));
		Check(TEXT("explicit-abort-stops-admission-timer-combat-and-clock-delegates"),
		           !Mode.CanAcceptCommands() && !Mode.IsLogicTimerActive() &&
		               Mode.GetCombatService()->GetRegisteredUnitCount() == 0 &&
		               !Mode.GetCommandProcessor()->BeforeExternalCommand.IsBound() &&
		               !Mode.GetCommandProcessor()->AfterExternalCommandClock.IsBound());
		Check(TEXT("cleanup-does-not-rewrite-merge-result-state"),
		           Same(Evidence.AfterBoard, Mode.GetBoardManager()->GetSnapshot(1)) &&
		               Same(Evidence.AfterEconomy, Mode.GetEconomyService()->GetSnapshot(1)) &&
		               Mode.GetEconomyService()->GetRandomState(1) == ExpectedRngAfter &&
		               Mode.GetCommandProcessor()->GetCachedResultCount(1) == 2);
		Evidence.bServerCleaned = !bFailed;
		Evidence.Phase = 4;
		Evidence.ForceNetUpdate();
	}
	else if (Evidence.Phase == 4 && Evidence.bClientCleaned)
	{
		Check(TEXT("exact-two-requests-and-responses-through-cleanup"), Requests.Num() == 2 && Responses.Num() == 2);
		Check(TEXT("observer-handles-restored"), RestoreObservers());
		Evidence.bServerPassed = !bFailed;
		Evidence.Phase = 5;
		Evidence.ForceNetUpdate();
	}
	else if (Evidence.Phase == 5 && Evidence.bClientFinished)
	{
		Check(TEXT("client-verified-cleaned-and-acknowledged-server-finish"),
		           Evidence.bClientReady && Evidence.bClientVerified && Evidence.bClientCleaned &&
		               Evidence.bClientPassed);
		Evidence.Phase = 6;
		Evidence.ForceNetUpdate();
		WriteResult(true);
		ExitAt = FPlatformTime::Seconds() + 2;
	}
}

void ULDG3NetConflictProbeSubsystem::TickClient(ALDPlayerController& Controller)
{
	if (!Peer.IsValid())
	{
		for (TActorIterator<ALDG3NetConflictPeer> It(GetWorld()); It; ++It)
		{
			if (It->GetOwner() == &Controller && It->MatchId == MatchId)
			{
				Peer = *It;
				break;
			}
		}
	}
	if (!Peer.IsValid())
	{
		return;
	}
	ALDG3NetConflictPeer& Evidence = *Peer;
	if (LocalPhase == 0 && Evidence.Phase == 1 && Same(Controller.GetBoardSnapshot(), Evidence.BeforeBoard) &&
	    Same(Controller.GetEconomySnapshot(), Evidence.BeforeEconomy) && ActorSetMatches(Evidence.BeforeBoard))
	{
		Check(TEXT("remote-baseline-sees-three-C01-and-unchanged-economy"),
		           Evidence.BeforeBoard.Population == 3 && Evidence.BeforeBoard.BoardRevision == 3 &&
		               Evidence.BeforeEconomy.Gold == 100 && Evidence.BeforeEconomy.Stars == 0 &&
		               Evidence.BeforeEconomy.PaidSummonCount == 0 && Evidence.First.IsValidPayload() &&
		               Evidence.Second.IsValidPayload() && Evidence.First.ExpectedBoardRevision == 3 &&
		               Evidence.Second.ExpectedBoardRevision == 3);
		Evidence.ServerReportObservation(1, MatchId, 3, !bFailed);
		LocalPhase = 1;
	}
	else if (LocalPhase == 1 && Evidence.Phase == 2)
	{
		Check(TEXT("burst-starts-on-actual-owned-remote-Preparing-controller"),
		           GetWorld()->GetNetMode() == NM_Client && Controller.IsLocalController() &&
		               Controller.GetLocalParticipantIndex() == 1 && !Controller.HasAuthority() &&
		               GetWorld()->GetGameState<ALDGameState>()->GetPhase() == ELDMatchPhase::Preparing);
		if (bFailed)
		{
			FailAndExit(TEXT("remote-burst-precondition"));
			return;
		}
		BurstSentAt = FPlatformTime::Seconds() - CreatedAt;
		FirstSendFrame = GFrameCounter;
		Controller.ServerRequestCommand(Evidence.First);
		SecondSendFrame = GFrameCounter;
		Controller.ServerRequestCommand(Evidence.Second);
		LocalPhase = 2;
	}
	else if (LocalPhase == 2 && Evidence.Phase == 3 && Responses.Num() == 2 && Evidence.ServerResponses.Num() == 2 &&
	         Same(Controller.GetBoardSnapshot(), Evidence.AfterBoard) &&
	         Same(Controller.GetEconomySnapshot(), Evidence.AfterEconomy) && ActorSetMatches(Evidence.AfterBoard))
	{
		VerifyClient(Controller);
		Evidence.ServerReportObservation(3, MatchId, Evidence.AfterBoard.BoardRevision, !bFailed);
		LocalPhase = 3;
	}
	else if (LocalPhase == 3 && Evidence.Phase == 4 &&
	         GetWorld()->GetGameState<ALDGameState>()->GetPhase() == ELDMatchPhase::Aborted)
	{
		Check(TEXT("replicated-explicit-cleanup-retains-final-owner-state"),
		           Evidence.bServerCleaned && Same(Controller.GetBoardSnapshot(), Evidence.AfterBoard) &&
		               Same(Controller.GetEconomySnapshot(), Evidence.AfterEconomy) &&
		               !Controller.HasPendingCommand() && !Controller.CanUseGameplayActions());
		Check(TEXT("observer-handles-restored"), RestoreObservers());
		Evidence.ServerReportObservation(4, MatchId, Evidence.AfterBoard.BoardRevision, !bFailed);
		LocalPhase = 4;
	}
	else if (LocalPhase == 4 && Evidence.Phase == 5)
	{
		Check(TEXT("server-confirms-client-cleanup-acknowledgment"), Evidence.bServerPassed && Evidence.bServerCleaned);
		Evidence.ServerReportObservation(5, MatchId, Evidence.AfterBoard.BoardRevision, !bFailed);
		LocalPhase = 5;
	}
	else if (LocalPhase == 5 && Evidence.Phase == 6)
	{
		Check(TEXT("server-received-final-client-acknowledgment"), Evidence.bServerPassed);
		WriteResult(true);
		ExitAt = FPlatformTime::Seconds() + .25;
		LocalPhase = 6;
	}
}

bool ULDG3NetConflictProbeSubsystem::RestoreObservers()
{
	bool bEventRestored = !bEventObserverInstalled;
#if !UE_BUILD_SHIPPING
	if (bEventObserverInstalled)
	{
		if (AActor::ProcessEventDelegate.GetHandle() == EventHandle)
		{
			AActor::ProcessEventDelegate.Unbind();
			bEventObserverInstalled = false;
			bEventRestored = true;
		}
		// Otherwise preserve the replacement and report failure, while still cleaning the board observer.
	}
#endif
	if (ObservedBoard.IsValid() && BoardHandle.IsValid())
	{
		ObservedBoard->OnBoardCommitted.Remove(BoardHandle);
	}
	BoardHandle.Reset();
	bObserversRestored = bEventRestored;
	return bObserversRestored;
}

void ULDG3NetConflictProbeSubsystem::FailAndExit(const FString& Reason)
{
	if (ExitAt > 0)
	{
		return;
	}
	Check(Reason, false);
	Check(TEXT("failure-path-restores-own-observers"), RestoreObservers());
	WriteResult(false);
	ExitAt = FPlatformTime::Seconds() + .25;
}

void ULDG3NetConflictProbeSubsystem::Tick(float DeltaTime)
{
	const double Now = FPlatformTime::Seconds();
	if (ExitAt > 0)
	{
		if (Now >= ExitAt)
		{
			bFinished = true;
			FPlatformMisc::RequestExit(false);
		}
		return;
	}
	if (Now - CreatedAt > TimeoutSeconds)
	{
		FailAndExit(TEXT("suite-timeout-no-missing-observation-can-pass"));
		return;
	}
	UWorld* World = GetWorld();
	ALDGameState* State = World ? World->GetGameState<ALDGameState>() : nullptr;
	if (!World || !State || !State->GetMatchContext().IsValid())
	{
		return;
	}
	if (bActivated && (World != MatchWorld.Get() || State->GetMatchContext().MatchId != MatchId))
	{
		FailAndExit(TEXT("unexpected-world-or-match-replacement"));
		return;
	}
	ALDPlayerController* Local = nullptr;
	ALDPlayerController* Remote = nullptr;
	int32 ParticipantCount = 0;
	for (TActorIterator<ALDPlayerController> It(World); It; ++It)
	{
		const ALDPlayerState* Player = It->GetPlayerState<ALDPlayerState>();
		if (Player && Player->GetParticipantContext().IsValid())
		{
			++ParticipantCount;
			if (It->IsLocalController())
			{
				Local = *It;
			}
			else if (Player->GetParticipantContext().PlayerIndex == 1)
			{
				Remote = *It;
			}
		}
	}
	if (!Local || !Local->IsGameplaySnapshotReady() || !Local->IsLocalBoardReady())
	{
		return;
	}
	if (!bActivated)
	{
		if (State->GetPhase() != ELDMatchPhase::Preparing)
		{
			return;
		}
		if (World->GetNetMode() == NM_ListenServer && (!Remote || ParticipantCount != 2))
		{
			return;
		}
		if (World->GetNetMode() != NM_ListenServer && World->GetNetMode() != NM_Client)
		{
			FailAndExit(TEXT("requires-actual-listen-and-remote-client-processes"));
			return;
		}
		LocalPlayerIndex = Local->GetLocalParticipantIndex();
		Check(TEXT("role-matches-real-local-owner"),
		           LocalPlayerIndex == (World->GetNetMode() == NM_ListenServer ? 0 : 1));
		if (bFailed)
		{
			FailAndExit(TEXT("unexpected-local-owner"));
			return;
		}
		MatchId = State->GetMatchContext().MatchId;
		MatchWorld = World;
		bActivated = true;
		if (!InstallObserver(Remote ? *Remote : *Local))
		{
			FailAndExit(TEXT("observer-installation-failed"));
			return;
		}
		if (ALDGameMode* Mode = World->GetAuthGameMode<ALDGameMode>())
		{
			if (!Remote || !PrepareFixture(*Mode, *Remote))
			{
				FailAndExit(TEXT("fixed-material-fixture-failed"));
				return;
			}
		}
	}
	if (ALDGameMode* Mode = World->GetAuthGameMode<ALDGameMode>())
	{
		TickAuthority(*Mode);
	}
	else
	{
		TickClient(*Local);
	}
}

bool ULDG3NetConflictProbeSubsystem::WriteResult(bool bHandshake)
{
	if (bWritten || !bOutputAvailable)
	{
		return false;
	}
	bFailed |= !bHandshake || !bObserversRestored;
	TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("result"), bFailed ? TEXT("Fail") : TEXT("Pass"));
	Result->SetStringField(
	    TEXT("scope"), TEXT("Development G3NetConflict fixture. Three free C01 units on remote owner1 via public Board preparation; two different IDs sent to actual owned ServerRequestCommand in one client tick. Actual server dispatch and client receipt observed without interception. No natural summon, Pending UI, two-remote-client concurrency, or physical-input claim."));
	Result->SetStringField(TEXT("engine"), FEngineVersion::Current().ToString());
	Result->SetNumberField(TEXT("processId"), FPlatformProcess::GetCurrentProcessId());
	Result->SetNumberField(TEXT("localPlayerIndex"), LocalPlayerIndex);
	Result->SetStringField(TEXT("matchId"), MatchId.ToString());
	Result->SetBoolField(TEXT("completionHandshakeConfirmed"), bHandshake);
	Result->SetBoolField(TEXT("observersRestored"), bObserversRestored);
	Result->SetBoolField(TEXT("authorityMetricsAvailable"), LocalPlayerIndex == 0);
	Result->SetNumberField(TEXT("serverRequestCount"), Requests.Num());
	Result->SetNumberField(TEXT("observedResponseCount"), Responses.Num());
	Result->SetNumberField(TEXT("mergePublicationCount"), MergeCommits.Num());
	Result->SetStringField(TEXT("firstSendFrame"), LexToString(FirstSendFrame));
	Result->SetStringField(TEXT("secondSendFrame"), LexToString(SecondSendFrame));
	Result->SetNumberField(TEXT("burstSentWallSeconds"), BurstSentAt);
	if (LocalPlayerIndex == 0)
	{
		Result->SetBoolField(TEXT("serverArrivalsSameFrame"),
		                          RequestFrames.Num() == 2 && RequestFrames[0] == RequestFrames[1]);
		Result->SetNumberField(TEXT("randomBefore"), RngBefore);
		Result->SetNumberField(TEXT("expectedRandomAfterOneDraw"), ExpectedRngAfter);
		Result->SetStringField(TEXT("expectedRareUnit"), ExpectedRare.ToString());
	}
	Result->SetArrayField(TEXT("checks"), Checks);
	Result->SetArrayField(TEXT("wire"), Wire);
	FString Json;
	const bool bSaved = FJsonSerializer::Serialize(Result, TJsonWriterFactory<>::Create(&Json)) &&
	                    FFileHelper::SaveStringToFile(Json, *(OutputDirectory / TEXT("result.json")));
	bWritten = bSaved;
	if (!bSaved)
	{
		bFailed = true;
		UE_LOG(LogLDG3NetConflict, Error, TEXT("Cannot save result: %s"), *OutputDirectory);
	}
	return bSaved;
}

void ULDG3NetConflictProbeSubsystem::Deinitialize()
{
	const bool bRestored = RestoreObservers();
	if (bActivated && !bWritten)
	{
		Check(TEXT("unexpected-shutdown-before-complete-proof"), false);
		Check(TEXT("shutdown-restores-own-observers"), bRestored);
		WriteResult(false);
	}
	Super::Deinitialize();
}
