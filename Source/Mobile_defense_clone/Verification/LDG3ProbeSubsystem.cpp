#include "Verification/LDG3ProbeSubsystem.h"

#include "Battle/LDCombatService.h"
#include "Battle/LDUnitActor.h"
#include "Board/LDBoardManager.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Core/LDGameMode.h"
#include "Core/LDGameState.h"
#include "Core/LDEntryPlayerController.h"
#include "Core/LDPlayerController.h"
#include "Economy/LDEconomyService.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/NetConnection.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMemory.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/EngineVersion.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Net/UnrealNetwork.h"
#include "Serialization/JsonSerializer.h"
#include "UI/LDGameplayWidget.h"
#include "UI/LDResultWidget.h"
#include "UnrealClient.h"
#include "Widgets/SWindow.h"

DEFINE_LOG_CATEGORY_STATIC(LogLDG3Probe, Log, All);

namespace
{
	const int32 Seeds[] = {1776, 42, 1729, 2026, 9001};
	FString BoardSignature(const FLDBoardSnapshot& Board)
	{
		TArray<FLDPlacedUnit> Units = Board.Units;
		Units.Sort([](const FLDPlacedUnit& A, const FLDPlacedUnit& B) { return A.InstanceId < B.InstanceId; });
		FString Value = FString::Printf(TEXT("%s:%d:%d:%d"), *Board.MatchId.ToString(), Board.PlayerIndex,
		                                     Board.BoardRevision, Board.Population);
		for (const FLDPlacedUnit& Unit : Units)
		{
			Value += FString::Printf(TEXT("|%llu,%s,%d,%d,%.9f"), Unit.InstanceId, *Unit.UnitId.ToString(),
			                              Unit.PlayerIndex, Unit.CellId, Unit.MoveBlockedUntilServerSeconds);
		}
		return Value;
	}
	FString EconomySignature(const FLDEconomySnapshot& Economy)
	{
		return FString::Printf(TEXT("%s:%d:%d:%d:%d:%d:%d"), *Economy.MatchId.ToString(), Economy.PlayerIndex,
		                            Economy.EconomyRevision, Economy.Gold, Economy.Stars, Economy.PaidSummonCount,
		                            Economy.NextSummonGold);
	}
	void SaveJson(const FString& Path, const TSharedPtr<FJsonObject>& Object)
	{
		FString Json;
		FJsonSerializer::Serialize(Object.ToSharedRef(), TJsonWriterFactory<>::Create(&Json));
		FFileHelper::SaveStringToFile(Json, *Path);
	}
} // namespace

ALDG3ProbePeer::ALDG3ProbePeer()
{
	bReplicates = true;
	bOnlyRelevantToOwner = true;
	SetNetUpdateFrequency(10);
}

void ALDG3ProbePeer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALDG3ProbePeer, FinalBattle);
	DOREPLIFETIME(ALDG3ProbePeer, FinalBoards);
	DOREPLIFETIME(ALDG3ProbePeer, FinalEconomies);
	DOREPLIFETIME(ALDG3ProbePeer, EffectiveDamageByPlayer);
	DOREPLIFETIME(ALDG3ProbePeer, FirstMergeAtByPlayer);
	DOREPLIFETIME(ALDG3ProbePeer, bTerminalCaptured);
	DOREPLIFETIME(ALDG3ProbePeer, bFinishSuite);
	DOREPLIFETIME(ALDG3ProbePeer, bMayReturn);
}

void ALDG3ProbePeer::ServerAcknowledge_Implementation(FGuid MatchId, int32 Revision)
{
	if (bTerminalCaptured && FinalBattle.MatchId == MatchId && FinalBattle.Revision == Revision)
	{
		bAcknowledged = true;
	}
}

void ALDG3ProbePeer::ServerPing_Implementation(int32 Serial, double SentAt)
{
	ClientPong(Serial, SentAt);
}

void ALDG3ProbePeer::ClientPong_Implementation(int32 Serial, double SentAt)
{
	const double Elapsed = FPlatformTime::Seconds() - SentAt;
	if (Serial > 0 && Serial <= PingsSent && Elapsed >= 0 && Elapsed < 30)
	{
		++PongsReceived;
		RoundTripsMs.Add(Elapsed * 1000);
	}
}

bool ULDG3ProbeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
	return false;
#else
	FString Probe;
	return FParse::Value(FCommandLine::Get(), TEXT("P0Probe="), Probe) &&
	                     Probe == TEXT("G3") && Super::ShouldCreateSubsystem(Outer);
#endif
}

void ULDG3ProbeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	StartedAt = FPlatformTime::Seconds();
	LastTickAt = StartedAt;
	FParse::Value(FCommandLine::Get(), TEXT("P0ProbeOutput="), OutputDirectory);
	FParse::Value(FCommandLine::Get(), TEXT("P0Role="), Role);
	FParse::Value(FCommandLine::Get(), TEXT("P0PeerAddress="), PeerAddress);
	FParse::Value(FCommandLine::Get(), TEXT("P0Matches="), RequestedMatches);
	FParse::Value(FCommandLine::Get(), TEXT("P0MinimumSeconds="), MinimumSeconds);
	FParse::Value(FCommandLine::Get(), TEXT("P0TimeoutSeconds="), TimeoutSeconds);
	RequestedMatches = FMath::Clamp(RequestedMatches, 1, 20);
	OutputDirectory = FPaths::ConvertRelativePathToFull(OutputDirectory);
	IFileManager::Get().MakeDirectory(*OutputDirectory, true);
}

void ULDG3ProbeSubsystem::Deinitialize()
{
	if (ObservedCombat.IsValid())
	{
		ObservedCombat->OnDamageCommitted.Remove(DamageHandle);
	}
	if (ObservedBoard.IsValid())
	{
		ObservedBoard->OnBoardCommitted.Remove(BoardHandle);
	}
	Super::Deinitialize();
}

bool ULDG3ProbeSubsystem::IsTickable() const
{
	return !IsTemplate() && !bFinished;
}

TStatId ULDG3ProbeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(ULDG3ProbeSubsystem, STATGROUP_Tickables);
}

UWorld* ULDG3ProbeSubsystem::GetTickableGameObjectWorld() const
{
	return GetWorld();
}

void ULDG3ProbeSubsystem::Check(const FString& Name, bool bPass, const FString& Detail)
{
	bFailed |= !bPass;
	TSharedPtr<FJsonObject> Item = MakeShared<FJsonObject>();
	Item->SetStringField(TEXT("name"), Name);
	Item->SetBoolField(TEXT("pass"), bPass);
	Item->SetStringField(TEXT("detail"), Detail);
	Item->SetNumberField(TEXT("wallSeconds"), FPlatformTime::Seconds() - StartedAt);
	Item->SetStringField(TEXT("matchId"), CurrentMatch.ToString());
	Checks.Add(MakeShared<FJsonValueObject>(Item));
	UE_LOG(LogLDG3Probe, Display, TEXT("%s %s %s"), bPass ? TEXT("PASS") : TEXT("FAIL"), *Name, *Detail);
}

void ULDG3ProbeSubsystem::BeginMatch(ALDGameMode* Mode, const FLDBattleSnapshot& Battle)
{
	if (ObservedCombat.IsValid())
	{
		ObservedCombat->OnDamageCommitted.Remove(DamageHandle);
	}
	if (ObservedBoard.IsValid())
	{
		ObservedBoard->OnBoardCommitted.Remove(BoardHandle);
	}
	EffectiveDamage[0] = EffectiveDamage[1] = 0;
	FirstMergeAt[0] = FirstMergeAt[1] = -1;
	bWaitingForNewHUD = false;
	CurrentMatch = Battle.MatchId;
	CurrentWorld = GetWorld();
	LocalPeer.Reset();
	ServerPeers.Reset();
	TerminalAt = 0;
	EntryAt = 0;
	LastWave = -1;
	LastResultId = 0;
	SuccessfulCommands = 0;
	FailedCommands = 0;
	DuplicateRequests = 0;
	HUDRecreations = 0;
	bRecordedTerminal = false;
	bReturned = false;
	bSold = false;
	bMoved = false;
	Check(TEXT("new-match-context"), CurrentMatch.IsValid());
	if (Mode)
	{
		ObservedCombat = Mode->GetCombatService();
		ObservedBoard = Mode->GetBoardManager();
		DamageHandle = ObservedCombat->OnDamageCommitted.AddUObject(this, &ULDG3ProbeSubsystem::HandleDamage);
		BoardHandle = ObservedBoard->OnBoardCommitted.AddUObject(this, &ULDG3ProbeSubsystem::HandleBoardCommit);
		for (int32 Player = 0; Player < 2; ++Player)
		{
			const FLDEconomySnapshot& Economy = Mode->GetEconomyService()->GetSnapshot(Player);
			Check(TEXT("fresh-economy-and-board"), Economy.Gold == 100 && Economy.Stars == 0 &&
			                                           Economy.PaidSummonCount == 0 &&
			                                           Mode->GetBoardManager()->GetSnapshot(Player).Population == 0);
		}
	}
	WriteProgress();
}

void ULDG3ProbeSubsystem::HandleDamage(const FLDDamageEvent& Event, int32 PlayerIndex, int32 Damage)
{
	if (Event.MatchId == CurrentMatch && PlayerIndex >= 0 && PlayerIndex < 2)
	{
		EffectiveDamage[PlayerIndex] += Damage;
	}
}

void ULDG3ProbeSubsystem::HandleBoardCommit(const FLDBoardCommit& Commit)
{
	if (Commit.MatchId == CurrentMatch && Commit.PlayerIndex >= 0 && Commit.PlayerIndex < 2 &&
	    Commit.ChangeReason == ELDBoardChangeReason::Merge && FirstMergeAt[Commit.PlayerIndex] < 0)
	{
		FirstMergeAt[Commit.PlayerIndex] = Commit.CommitServerSeconds;
	}
}

void ULDG3ProbeSubsystem::TickAuthority(ALDGameMode& Mode)
{
	for (TActorIterator<ALDPlayerController> It(GetWorld()); It; ++It)
	{
		bool bFound = false;
		for (const TWeakObjectPtr<ALDG3ProbePeer>& Peer : ServerPeers)
		{
			bFound |= Peer.IsValid() && Peer->GetOwner() == *It;
		}
		if (!bFound)
		{
			FActorSpawnParameters Params;
			Params.Owner = *It;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ServerPeers.Add(GetWorld()->SpawnActor<ALDG3ProbePeer>(Params));
		}
	}
	const FLDBattleSnapshot& Battle = GetWorld()->GetGameState<ALDGameState>()->GetBattleSnapshot();
	if (!Battle.IsTerminal())
	{
		return;
	}
	bool bBothAcknowledged = ServerPeers.Num() == 2;
	for (const TWeakObjectPtr<ALDG3ProbePeer>& WeakPeer : ServerPeers)
	{
		ALDG3ProbePeer* Peer = WeakPeer.Get();
		if (!Peer)
		{
			bBothAcknowledged = false;
			continue;
		}
		if (!Peer->bTerminalCaptured)
		{
			Peer->FinalBattle = Battle;
			for (int32 Player = 0; Player < 2; ++Player)
			{
				Peer->FinalBoards.Add(Mode.GetBoardManager()->GetSnapshot(Player));
				Peer->FinalEconomies.Add(Mode.GetEconomyService()->GetSnapshot(Player));
				Peer->EffectiveDamageByPlayer.Add(EffectiveDamage[Player]);
				Peer->FirstMergeAtByPlayer.Add(FirstMergeAt[Player]);
			}
			Peer->bFinishSuite = CompletedMatches + 1 >= RequestedMatches && ConnectedGameplaySeconds >= MinimumSeconds;
			Peer->bTerminalCaptured = true;
			Peer->ForceNetUpdate();
		}
		bBothAcknowledged &= Peer->bAcknowledged;
	}
	if (bBothAcknowledged)
	{
		for (const TWeakObjectPtr<ALDG3ProbePeer>& Peer : ServerPeers)
		{
			Peer->bMayReturn = true;
			Peer->ForceNetUpdate();
		}
	}
}

bool ULDG3ProbeSubsystem::Click(const FBox2D& Rect)
{
	if (!FSlateApplication::IsInitialized() || !GetWorld()->GetGameViewport() ||
	    !GetWorld()->GetGameViewport()->GetWindow())
	{
		return false;
	}
	FVector2D Absolute;
	USlateBlueprintLibrary::ScreenToWidgetAbsolute(this, Rect.GetCenter(), Absolute);
	const FPointerEvent Down(0, Absolute, Absolute, {EKeys::LeftMouseButton}, EKeys::LeftMouseButton, 0,
	                         FModifierKeysState());
	const FPointerEvent Up(0, Absolute, Absolute, {}, EKeys::LeftMouseButton, 0, FModifierKeysState());
	FSlateApplication& Slate = FSlateApplication::Get();
	Slate.ProcessMouseMoveEvent(Down, true);
	Slate.ProcessMouseButtonDownEvent(GetWorld()->GetGameViewport()->GetWindow()->GetNativeWindow(), Down);
	Slate.ProcessMouseButtonUpEvent(Up);
	return true;
}

void ULDG3ProbeSubsystem::PlayAction(ALDPlayerController& Controller)
{
	const double Now = FPlatformTime::Seconds();
	if (Now - LastActionAt < 0.45 || !Controller.IsLocalBoardReady() || !Controller.IsGameplaySnapshotReady())
	{
		return;
	}
	if (Controller.HasPendingCommand())
	{
		return;
	}
	const FLDBoardSnapshot& Board = Controller.GetBoardSnapshot();
	FBox2D Rect;
	// Repeatable strategy: merge eligible complete stacks, then spend affordable gold.
	// No fixture changes rewards, probability, HP, speed, or the wave clock.
	for (const FLDPlacedUnit& Unit : Board.Units)
	{
		FVector2D Point;
		if (Controller.ProjectCellToScreen(Unit.CellId, Point))
		{
			Controller.InputScreenPosition(Point);
			if (Controller.CanMergeSelection() && Controller.GetActionScreenRect(ELDCommandType::Merge, Rect))
			{
				LastActionAt = Now;
				Click(Rect);
				if (Controller.RetryPendingCommand())
				{
					++DuplicateRequests;
				}
				return;
			}
		}
	}
	const FLDEconomySnapshot& Economy = Controller.GetEconomySnapshot();
	if (Economy.Gold >= Economy.NextSummonGold && Board.Population < 20 &&
	    Controller.GetActionScreenRect(ELDCommandType::Summon, Rect))
	{
		LastActionAt = Now;
		Click(Rect);
		if (Controller.RetryPendingCommand())
		{
			++DuplicateRequests;
		}
		return;
	}
	// One ordinary sale and one move per match exercise the same product requests under latency.
	if (!bSold && Board.Population >= 6 && !Board.Units.IsEmpty())
	{
		FVector2D Point;
		if (Controller.ProjectCellToScreen(Board.Units.Last().CellId, Point))
		{
			Controller.InputScreenPosition(Point);
			if (Controller.GetActionScreenRect(ELDCommandType::Sell, Rect))
			{
				bSold = true;
				LastActionAt = Now;
				Click(Rect);
				return;
			}
		}
	}
	if (!bMoved && Board.Population >= 4 && !Board.Units.IsEmpty())
	{
		const int32 Destination = LocalPlayer == 0 ? 9 : 27;
		bMoved = Controller.RequestMove(Board.Units[0].InstanceId, Destination);
		LastActionAt = Now;
	}
}

void ULDG3ProbeSubsystem::RecordMatch(ALDPlayerController& Controller, ALDG3ProbePeer& Peer)
{
	const FLDBattleSnapshot& Battle = GetWorld()->GetGameState<ALDGameState>()->GetBattleSnapshot();
	bool bBossesMatch = Battle.Bosses.Num() == Peer.FinalBattle.Bosses.Num();
	for (const FLDBossSnapshot& Boss : Battle.Bosses)
	{
		const FLDBossSnapshot* Other = Peer.FinalBattle.Bosses.FindByPredicate(
		    [&Boss](const FLDBossSnapshot& Candidate) { return Candidate.EnemyId == Boss.EnemyId; });
		bBossesMatch &= Other && Other->RouteIndex == Boss.RouteIndex && Other->HP == Boss.HP &&
		                Other->MaxHP == Boss.MaxHP && Other->bAlive == Boss.bAlive;
	}
	Check(TEXT("terminal-boss-and-clock-state-matches-server"),
	           bBossesMatch && Battle.BossDeadlineServerSeconds == Peer.FinalBattle.BossDeadlineServerSeconds &&
	               Battle.PreparationEndServerSeconds == Peer.FinalBattle.PreparationEndServerSeconds &&
	               Battle.WaveEndServerSeconds == Peer.FinalBattle.WaveEndServerSeconds &&
	               Battle.bFinalSpawnsComplete == Peer.FinalBattle.bFinalSpawnsComplete &&
	               Battle.FinalWave == Peer.FinalBattle.FinalWave &&
	               Battle.MaxEnemyCount == Peer.FinalBattle.MaxEnemyCount);
	Check(TEXT("terminal-battle-state-matches-server"),
	           Battle.MatchId == Peer.FinalBattle.MatchId && Battle.Revision == Peer.FinalBattle.Revision &&
	               Battle.Result == Peer.FinalBattle.Result && Battle.ResultReason == Peer.FinalBattle.ResultReason &&
	               Battle.WaveIndex == Peer.FinalBattle.WaveIndex &&
	               Battle.ActiveEnemyCount == Peer.FinalBattle.ActiveEnemyCount &&
	               Battle.ResultServerSeconds == Peer.FinalBattle.ResultServerSeconds);
	Check(TEXT("terminal-owner-board"),
	           BoardSignature(Controller.GetBoardSnapshot()) == BoardSignature(Peer.FinalBoards[LocalPlayer]));
	Check(TEXT("terminal-owner-economy"),
	           EconomySignature(Controller.GetEconomySnapshot()) == EconomySignature(Peer.FinalEconomies[LocalPlayer]));
	TMap<uint64, FLDPlacedUnit> Expected;
	for (const FLDBoardSnapshot& Board : Peer.FinalBoards)
	{
		for (const FLDPlacedUnit& Unit : Board.Units)
		{
			Expected.Add(Unit.InstanceId, Unit);
		}
	}
	int32 ActualCount = 0;
	bool bActorsMatch = true;
	TSet<uint64> SeenIds;
	for (TActorIterator<ALDUnitActor> It(GetWorld()); It; ++It)
	{
		if (!It->IsCommitted())
		{
			continue;
		}
		++ActualCount;
		const FLDPlacedUnit& Unit = It->GetPlacement();
		bActorsMatch &= !SeenIds.Contains(Unit.InstanceId);
		SeenIds.Add(Unit.InstanceId);
		const FLDPlacedUnit* Wanted = Expected.Find(Unit.InstanceId);
		bActorsMatch &= Wanted && Wanted->CellId == Unit.CellId && Wanted->PlayerIndex == Unit.PlayerIndex &&
		                Wanted->UnitId == Unit.UnitId;
	}
	Check(TEXT("both-boards-exact-committed-actor-set"),
	           bActorsMatch && ActualCount == Expected.Num() && SeenIds.Num() == Expected.Num());
	Check(TEXT("natural-result-not-aborted"),
	           Battle.Result == ELDMatchResult::Victory || Battle.Result == ELDMatchResult::Defeat);
	Check(TEXT("commands-observed"), SuccessfulCommands > 0 && (LocalPlayer == 0 || DuplicateRequests > 0));
	TSharedPtr<FJsonObject> Item = MakeShared<FJsonObject>();
	Item->SetStringField(TEXT("matchId"), CurrentMatch.ToString());
	Item->SetNumberField(TEXT("seed"), Seeds[CompletedMatches % UE_ARRAY_COUNT(Seeds)]);
	Item->SetNumberField(TEXT("result"), static_cast<int32>(Battle.Result));
	Item->SetNumberField(TEXT("reason"), static_cast<int32>(Battle.ResultReason));
	Item->SetNumberField(TEXT("wave"), Battle.WaveIndex);
	Item->SetNumberField(TEXT("remainingNormal"), Battle.ActiveEnemyCount);
	Item->SetNumberField(TEXT("resultServerSeconds"), Battle.ResultServerSeconds);
	Item->SetNumberField(TEXT("successfulCommands"), SuccessfulCommands);
	Item->SetNumberField(TEXT("failedCommands"), FailedCommands);
	Item->SetNumberField(TEXT("duplicateRetries"), DuplicateRequests);
	Item->SetNumberField(TEXT("hudRecreations"), HUDRecreations);
	TArray<TSharedPtr<FJsonValue>> DamageValues;
	for (int64 Value : Peer.EffectiveDamageByPlayer)
	{
		DamageValues.Add(MakeShared<FJsonValueNumber>(static_cast<double>(Value)));
	}
	Item->SetArrayField(TEXT("effectiveDamageByPlayer"), DamageValues);
	TArray<TSharedPtr<FJsonValue>> MergeValues;
	for (double Value : Peer.FirstMergeAtByPlayer)
	{
		MergeValues.Add(MakeShared<FJsonValueNumber>(Value));
	}
	Item->SetArrayField(TEXT("firstMergeServerSecondsByPlayer"), MergeValues);
	TArray<TSharedPtr<FJsonValue>> BossValues;
	for (const FLDBossSnapshot& Boss : Battle.Bosses)
	{
		TSharedPtr<FJsonObject> Value = MakeShared<FJsonObject>();
		Value->SetNumberField(TEXT("id"), static_cast<double>(Boss.EnemyId));
		Value->SetNumberField(TEXT("route"), Boss.RouteIndex);
		Value->SetNumberField(TEXT("hp"), Boss.HP);
		Value->SetNumberField(TEXT("maxHP"), Boss.MaxHP);
		Value->SetBoolField(TEXT("alive"), Boss.bAlive);
		BossValues.Add(MakeShared<FJsonValueObject>(Value));
	}
	Item->SetArrayField(TEXT("bosses"), BossValues);
	Item->SetStringField(TEXT("ownerBoard"), BoardSignature(Controller.GetBoardSnapshot()));
	Item->SetStringField(TEXT("ownerEconomy"), EconomySignature(Controller.GetEconomySnapshot()));
	TArray<TSharedPtr<FJsonValue>> RTT;
	for (double Value : Peer.RoundTripsMs)
	{
		RTT.Add(MakeShared<FJsonValueNumber>(Value));
	}
	Item->SetArrayField(TEXT("unreliableEchoRoundTripMs"), RTT);
	Item->SetNumberField(TEXT("pingsSent"), Peer.PingsSent);
	Item->SetNumberField(TEXT("pongsReceived"), Peer.PongsReceived);
	Matches.Add(MakeShared<FJsonValueObject>(Item));
	++CompletedMatches;
	bRecordedTerminal = true;
	FScreenshotRequest::RequestScreenshot(OutputDirectory /
	                                      FString::Printf(TEXT("match-%d-result.png"), CompletedMatches), true, false);
	Peer.ServerAcknowledge(Battle.MatchId, Battle.Revision);
	WriteProgress();
}

void ULDG3ProbeSubsystem::TickLocal(ALDPlayerController& Controller, const FLDBattleSnapshot& Battle)
{
	LocalPlayer = Controller.GetLocalParticipantIndex();
	if (LocalPlayer < 0 || !Controller.IsGameplaySnapshotReady())
	{
		return;
	}
	if (!LocalPeer.IsValid())
	{
		for (TActorIterator<ALDG3ProbePeer> It(GetWorld()); It; ++It)
		{
			if (It->GetOwner() == &Controller)
			{
				LocalPeer = *It;
				break;
			}
		}
	}
	const double Now = FPlatformTime::Seconds();
	if (LocalPeer.IsValid() && LocalPlayer == 1 && Now - LastPingAt >= 1)
	{
		LastPingAt = Now;
		LocalPeer->ServerPing(++LocalPeer->PingsSent, Now);
	}
	if (Controller.GetLastResult().RequestId != 0 && Controller.GetLastResult().RequestId != LastResultId)
	{
		LastResultId = Controller.GetLastResult().RequestId;
		if (Controller.GetLastResult().ResultCode == ELDCommandResultCode::Success)
		{
			++SuccessfulCommands;
		}
		else
		{
			++FailedCommands;
		}
	}
	if (Battle.WaveIndex != LastWave)
	{
		LastWave = Battle.WaveIndex;
		TSharedPtr<FJsonObject> Item = MakeShared<FJsonObject>();
		Item->SetStringField(TEXT("matchId"), CurrentMatch.ToString());
		Item->SetNumberField(TEXT("wave"), LastWave);
		Item->SetNumberField(TEXT("serverSeconds"), GetWorld()->GetGameState()->GetServerWorldTimeSeconds());
		Item->SetNumberField(TEXT("gold"), Controller.GetEconomySnapshot().Gold);
		Item->SetNumberField(TEXT("population"), Controller.GetBoardSnapshot().Population);
		Item->SetNumberField(TEXT("normalCount"), Battle.ActiveEnemyCount);
		Samples.Add(MakeShared<FJsonValueObject>(Item));
		if (LastWave == 1 || LastWave == 10)
		{
			FScreenshotRequest::RequestScreenshot(
			    OutputDirectory / FString::Printf(TEXT("match-%d-wave-%d.png"), CompletedMatches + 1, LastWave), true,
			                                      false);
		}
	}
	if (!Battle.IsTerminal())
	{
		if (bWaitingForNewHUD)
		{
			int32 Count = 0;
			bool bNew = false;
			for (TObjectIterator<ULDGameplayWidget> It; It; ++It)
			{
				if (It->GetWorld() == GetWorld() && It->GetOwningPlayer() == &Controller && It->IsInViewport())
				{
					++Count;
					bNew |= *It != RemovedHUD.Get();
				}
			}
			if (bNew)
			{
				Check(TEXT("hud-recreated-single-new-instance"), Count == 1);
				++HUDRecreations;
				bWaitingForNewHUD = false;
			}
		}
		if (Battle.Phase == ELDMatchPhase::Preparing || Battle.Phase == ELDMatchPhase::Running)
		{
			if (!bWaitingForNewHUD && LastWave >= 2 + HUDRecreations && HUDRecreations < 3 &&
			    !Controller.HasPendingCommand())
			{
				for (TObjectIterator<ULDGameplayWidget> It; It; ++It)
				{
					if (It->GetWorld() == GetWorld() && It->GetOwningPlayer() == &Controller && It->IsInViewport())
					{
						RemovedHUD = *It;
						It->RemoveFromParent();
						bWaitingForNewHUD = true;
						break;
					}
				}
			}
			PlayAction(Controller);
		}
		return;
	}
	if (TerminalAt == 0)
	{
		TerminalAt = Now;
	}
	if (!LocalPeer.IsValid() || !LocalPeer->bTerminalCaptured || LocalPeer->FinalBoards.Num() != 2 ||
	    LocalPeer->FinalEconomies.Num() != 2)
	{
		return;
	}
	if (!bRecordedTerminal && Now - TerminalAt >= 3 && !Controller.HasPendingCommand())
	{
		RecordMatch(Controller, *LocalPeer);
	}
	if (!bRecordedTerminal || !LocalPeer->bMayReturn)
	{
		return;
	}
	if (LocalPeer->bFinishSuite)
	{
		if (FinishAt == 0)
		{
			FinishAt = Now + (LocalPlayer == 0 ? 7 : 1);
		}
		return;
	}
	if (!bReturned && Now - TerminalAt >= (LocalPlayer == 0 ? 8 : 5))
	{
		for (TObjectIterator<ULDResultWidget> It; It; ++It)
		{
			FBox2D Rect;
			if (It->GetWorld() == GetWorld() && It->GetOwningPlayer() == &Controller &&
			    It->GetReturnButtonScreenRect(Rect))
			{
				bReturned = Click(Rect);
				Check(TEXT("result-return-slate-click"), bReturned);
				break;
			}
		}
	}
}

void ULDG3ProbeSubsystem::Tick(float DeltaTime)
{
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld() || !World->HasBegunPlay() || bFinished)
	{
		return;
	}
	const double Now = FPlatformTime::Seconds();
	const double WallDelta = FMath::Max(0.0, Now - LastTickAt);
	LastTickAt = Now;
	if (Now - StartedAt > TimeoutSeconds)
	{
		Check(TEXT("suite-timeout"), false);
		Finish();
		return;
	}
	if (FinishAt > 0 && Now >= FinishAt)
	{
		Finish();
		return;
	}
	FrameMilliseconds.Add(DeltaTime * 1000);
	ALDGameState* State = World->GetGameState<ALDGameState>();
	ALDGameMode* Mode = World->GetAuthGameMode<ALDGameMode>();
	if (!State)
	{
		if (bFirstMatch)
		{
			if (EntryAt == 0)
			{
				EntryAt = Now;
			}
			if (ALDEntryPlayerController* PC =
			        Cast<ALDEntryPlayerController>(GetGameInstance()->GetFirstLocalPlayerController()))
			{
				FBox2D Rect;
				const bool bHost = Role == TEXT("host");
				if (!bHost)
				{
					PC->SetJoinAddressText(PeerAddress);
				}
				if (Now - EntryAt > (bHost ? 2 : 7) && PC->GetEntryActionScreenRect(bHost, Rect))
				{
					bFirstMatch = false;
					Check(TEXT("entry-slate-button"), Click(Rect));
				}
			}
		}
		// Returned entry world: use normal travel to start another fixed seed without restarting either process.
		if (CompletedMatches > 0 && bReturned)
		{
			if (EntryAt == 0)
			{
				EntryAt = Now;
			}
			if (Role == TEXT("host") && Now - EntryAt > 2)
			{
				bReturned = false;
				UGameplayStatics::OpenLevel(
				    this,
				    TEXT("/Game/LD/Maps/L_P0"), true,
				         FString::Printf(TEXT("listen?P0Seed=%d"), Seeds[CompletedMatches % UE_ARRAY_COUNT(Seeds)]));
			}
			else if (Role == TEXT("client") && Now - EntryAt > 7)
			{
				if (APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController())
				{
					bReturned = false;
					PC->ClientTravel(PeerAddress, TRAVEL_Absolute);
				}
			}
		}
		return;
	}
	const FLDBattleSnapshot Battle = State->GetBattleSnapshot();
	if ((Battle.Phase == ELDMatchPhase::Preparing || Battle.Phase == ELDMatchPhase::Running) && World->GetNetDriver() &&
	    (World->GetNetDriver()->ServerConnection || World->GetNetDriver()->ClientConnections.Num() > 0))
	{
		ConnectedGameplaySeconds += WallDelta;
	}
	if (!Battle.MatchId.IsValid())
	{
		return;
	}
	if (CurrentMatch != Battle.MatchId)
	{
		BeginMatch(Mode, Battle);
	}
	if (Mode && Mode->GetBoardManager() && Mode->GetEconomyService())
	{
		TickAuthority(*Mode);
	}
	if (ALDPlayerController* PC = Cast<ALDPlayerController>(GetGameInstance()->GetFirstLocalPlayerController()))
	{
		TickLocal(*PC, Battle);
	}
	if (Now - LastProgressAt >= 10)
	{
		WriteProgress();
	}
}

void ULDG3ProbeSubsystem::WriteProgress()
{
	LastProgressAt = FPlatformTime::Seconds();
	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("result"), bFinished ? (bFailed ? TEXT("Fail") : TEXT("Pass")) : TEXT("Running"));
	Root->SetStringField(
	    TEXT("scope"), TEXT("Two actual processes, product rules, automated UI decisions; no natural-play HP/gold/time fixture. Repeated result UI return and ordinary travel in the same processes."));
	Root->SetStringField(TEXT("engine"), FEngineVersion::Current().ToString());
	Root->SetStringField(TEXT("role"), Role);
	Root->SetNumberField(TEXT("completedMatches"), CompletedMatches);
	Root->SetNumberField(TEXT("elapsedWallSeconds"), LastProgressAt - StartedAt);
	Root->SetNumberField(TEXT("connectedGameplaySeconds"), ConnectedGameplaySeconds);
	Root->SetNumberField(TEXT("frameCount"), FrameMilliseconds.Num());
	Root->SetNumberField(TEXT("processPhysicalBytes"), static_cast<double>(FPlatformMemory::GetStats().UsedPhysical));
	Root->SetArrayField(TEXT("checks"), Checks);
	Root->SetArrayField(TEXT("matches"), Matches);
	Root->SetArrayField(TEXT("waveSamples"), Samples);
	if (bFinished && !FrameMilliseconds.IsEmpty())
	{
		FrameMilliseconds.Sort();
		Root->SetNumberField(TEXT("frameMsP95"),
		                          FrameMilliseconds[FMath::Clamp(FMath::CeilToInt(FrameMilliseconds.Num() * 0.95) - 1,
		                                                         0, FrameMilliseconds.Num() - 1)]);
	}
	SaveJson(OutputDirectory / (bFinished ? TEXT("result.json") : TEXT("progress.json")), Root);
}

void ULDG3ProbeSubsystem::Finish()
{
	if (bFinished)
	{
		return;
	}
	Check(TEXT("requested-match-count"), CompletedMatches >= RequestedMatches);
	Check(TEXT("minimum-connected-duration"), ConnectedGameplaySeconds + 1 >= MinimumSeconds);
	bFinished = true;
	WriteProgress();
	FPlatformMisc::RequestExit(false);
}
