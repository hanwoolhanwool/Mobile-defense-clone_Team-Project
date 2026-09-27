#include "Verification/LDG3EntryProbeSubsystem.h"

#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Board/LDBoardManager.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Core/LDEntryGameMode.h"
#include "Core/LDEntryPlayerController.h"
#include "Core/LDGameInstance.h"
#include "Core/LDGameMode.h"
#include "Core/LDGameState.h"
#include "Core/LDPlayerController.h"
#include "Core/LDPlayerState.h"
#include "Economy/LDEconomyService.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/NetConnection.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Net/UnrealNetwork.h"
#include "Network/LDCommandProcessor.h"
#include "Serialization/JsonSerializer.h"
#include "UI/LDEntryWidget.h"
#include "UI/LDGameplayWidget.h"
#include "UI/LDResultWidget.h"
#include "UObject/GarbageCollection.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"
#include "UnrealClient.h"
#include "Widgets/SWindow.h"

DEFINE_LOG_CATEGORY_STATIC(LogLDG3EntryProbe, Log, All);

namespace
{
	template <typename T> T* VisibleWidget(APlayerController& Owner, int32& Count)
	{
		T* Found = nullptr;
		Count = 0;
		for (TObjectIterator<T> It; It; ++It)
		{
			if (It->GetWorld() == Owner.GetWorld() && It->GetOwningPlayer() == &Owner && It->IsInViewport())
			{
				Found = *It;
				++Count;
			}
		}
		return Found;
	}
	bool SaveJson(const FString& Path, const TSharedPtr<FJsonObject>& Object)
	{
		FString Json;
		return FJsonSerializer::Serialize(Object.ToSharedRef(), TJsonWriterFactory<>::Create(&Json)) &&
		       FFileHelper::SaveStringToFile(Json, *Path);
	}
	TSharedPtr<FJsonObject> BattleJson(const FLDBattleSnapshot& Battle)
	{
		TSharedPtr<FJsonObject> Item = MakeShared<FJsonObject>();
		Item->SetStringField(TEXT("matchId"), Battle.MatchId.ToString());
		Item->SetNumberField(TEXT("revision"), Battle.Revision);
		Item->SetNumberField(TEXT("phase"), static_cast<int32>(Battle.Phase));
		Item->SetNumberField(TEXT("result"), static_cast<int32>(Battle.Result));
		Item->SetNumberField(TEXT("reason"), static_cast<int32>(Battle.ResultReason));
		Item->SetNumberField(TEXT("loadingDeadlineServerSeconds"), Battle.LoadingDeadlineServerSeconds);
		Item->SetNumberField(TEXT("resultServerSeconds"), Battle.ResultServerSeconds);
		Item->SetNumberField(TEXT("wave"), Battle.WaveIndex);
		Item->SetNumberField(TEXT("activeEnemies"), Battle.ActiveEnemyCount);
		return Item;
	}
	bool SameBattle(const FLDBattleSnapshot& A, const FLDBattleSnapshot& B)
	{
		return FLDBattleSnapshot::StaticStruct()->CompareScriptStruct(&A, &B, 0);
	}
	bool InViewport(const FBox2D& Rect, const FIntPoint& Size)
	{
		return Rect.GetArea() > 0 && Rect.Min.X >= 0 && Rect.Min.Y >= 0 && Rect.Max.X <= Size.X + 1 &&
		       Rect.Max.Y <= Size.Y + 1;
	}
} // namespace

ALDG3EntryProbePeer::ALDG3EntryProbePeer()
{
	bReplicates = true;
	bOnlyRelevantToOwner = true;
	SetNetUpdateFrequency(10);
}

void ALDG3EntryProbePeer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALDG3EntryProbePeer, TimeoutBattle);
}

void ALDG3EntryProbePeer::ServerObserveTerminal_Implementation(FGuid MatchId, int32 Revision, bool bPass)
{
	if (!bAcknowledged && MatchId == TimeoutBattle.MatchId && Revision == TimeoutBattle.Revision)
	{
		bAcknowledged = true;
		bClientPassed = bPass;
	}
}

bool ULDG3EntryProbeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
	return false;
#else
	FString Probe;
	return FParse::Value(FCommandLine::Get(), TEXT("P0Probe="), Probe) &&
	                     Probe == TEXT("G3Entry") && Super::ShouldCreateSubsystem(Outer);
#endif
}

void ULDG3EntryProbeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	StartedAt = FPlatformTime::Seconds();
	FParse::Value(FCommandLine::Get(), TEXT("P0ProbeOutput="), OutputDirectory);
	FParse::Value(FCommandLine::Get(), TEXT("P0Role="), Role);
	FParse::Value(FCommandLine::Get(), TEXT("P0PeerAddress="), PeerAddress);
	FParse::Value(FCommandLine::Get(), TEXT("P0EntryRunId="), RunId);
	FParse::Value(FCommandLine::Get(), TEXT("P0TimeoutSeconds="), TimeoutSeconds);
	if (OutputDirectory.IsEmpty() || RunId.IsEmpty() || (Role != TEXT("host") && Role != TEXT("client")))
	{
		UE_LOG(LogLDG3EntryProbe, Error, TEXT("G3Entry needs fresh P0ProbeOutput, P0Role and shared P0EntryRunId"));
		bFinished = true;
		FPlatformMisc::RequestExit(false);
		return;
	}
	OutputDirectory = FPaths::ConvertRelativePathToFull(OutputDirectory);
	FPaths::NormalizeDirectoryName(OutputDirectory);
	PeerOutputDirectory = FPaths::GetPath(OutputDirectory) / (Role == TEXT("host") ? TEXT("client") : TEXT("host"));
	if (IFileManager::Get().FileExists(*(OutputDirectory / TEXT("progress.json"))) ||
	                                   IFileManager::Get().FileExists(*(OutputDirectory / TEXT("result.json"))))
	{
		UE_LOG(LogLDG3EntryProbe, Error, TEXT("G3Entry refuses to overwrite prior evidence: %s"), *OutputDirectory);
		bFinished = true;
		FPlatformMisc::RequestExit(false);
		return;
	}
	bOutputReady = IFileManager::Get().MakeDirectory(*OutputDirectory, true);
	if (GEngine)
	{
		NetworkFailureHandle =
		    GEngine->OnNetworkFailure().AddUObject(this, &ULDG3EntryProbeSubsystem::ObserveNetworkFailure);
	}
	WriteProgress(TEXT("initializing"));
}

void ULDG3EntryProbeSubsystem::Deinitialize()
{
	if (GEngine)
	{
		GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
	}
	NetworkFailureHandle.Reset();
	Super::Deinitialize();
}

bool ULDG3EntryProbeSubsystem::IsTickable() const
{
	return !IsTemplate() && !bFinished;
}
TStatId ULDG3EntryProbeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(ULDG3EntryProbeSubsystem, STATGROUP_Tickables);
}
UWorld* ULDG3EntryProbeSubsystem::GetTickableGameObjectWorld() const
{
	return GetWorld();
}

void ULDG3EntryProbeSubsystem::Check(const FString& Name, bool bPass, const FString& Detail)
{
	bFailed |= !bPass;
	TSharedPtr<FJsonObject> Item = MakeShared<FJsonObject>();
	Item->SetStringField(TEXT("name"), Name);
	Item->SetBoolField(TEXT("pass"), bPass);
	Item->SetStringField(TEXT("detail"), Detail);
	Item->SetNumberField(TEXT("wallSeconds"), FPlatformTime::Seconds() - StartedAt);
	Checks.Add(MakeShared<FJsonValueObject>(Item));
	UE_LOG(LogLDG3EntryProbe, Display, TEXT("%s %s %s"), bPass ? TEXT("PASS") : TEXT("FAIL"), *Name, *Detail);
}

void ULDG3EntryProbeSubsystem::WriteProgress(const FString& Stage)
{
	CurrentStage = Stage;
	if (!bOutputReady)
		return;
	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("runId"), RunId);
	Root->SetStringField(TEXT("role"), Role);
	Root->SetStringField(TEXT("stage"), Stage);
	Root->SetNumberField(TEXT("wallSeconds"), FPlatformTime::Seconds() - StartedAt);
	Root->SetNumberField(TEXT("worldSeconds"), GetWorld() ? GetWorld()->GetTimeSeconds() : -1);
	Root->SetObjectField(TEXT("timeoutSnapshot"), BattleJson(TimeoutBattle));
	SaveJson(OutputDirectory / TEXT("progress.json"), Root);
	LastProgressAt = FPlatformTime::Seconds();
}

bool ULDG3EntryProbeSubsystem::ReadPeerStage(const FString& Filename, const FString& Stage) const
{
	FString Text;
	TSharedPtr<FJsonObject> Object;
	if (!FFileHelper::LoadFileToString(Text, *(PeerOutputDirectory / Filename)) ||
	    !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Object) || !Object.IsValid())
		return false;
	FString OtherRun;
	FString OtherStage;
	FString OtherRole;
	return Object->TryGetStringField(TEXT("runId"), OtherRun) && OtherRun == RunId &&
	                                 Object->TryGetStringField(TEXT("stage"), OtherStage) && OtherStage == Stage &&
	                                                           Object->TryGetStringField(TEXT("role"), OtherRole) &&
	                                                                                     OtherRole != Role;
}

void ULDG3EntryProbeSubsystem::Capture(const FString& Name)
{
	FScreenshotRequest::RequestScreenshot(OutputDirectory / (Name + TEXT(".png")), true, false);
}

bool ULDG3EntryProbeSubsystem::Click(const FBox2D& Rect)
{
	if (!FSlateApplication::IsInitialized() || !GetWorld() || !GetWorld()->GetGameViewport() ||
	    !GetWorld()->GetGameViewport()->GetWindow())
		return false;
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

bool ULDG3EntryProbeSubsystem::InspectEntry(ALDEntryPlayerController& Controller, bool bReturned)
{
	int32 Count = 0;
	ULDEntryWidget* Widget = VisibleWidget<ULDEntryWidget>(Controller, Count);
	FBox2D HostRect;
	FBox2D JoinRect;
	FBox2D AddressRect;
	UGameViewportClient* Viewport = GetWorld()->GetGameViewport();
	if (!Widget || !Widget->WidgetTree || !Viewport || !Viewport->Viewport ||
	    !Controller.GetEntryActionScreenRect(true, HostRect) || !Controller.GetEntryActionScreenRect(false, JoinRect) ||
	    !Widget->GetAddressScreenRect(AddressRect))
		return false;
	const FIntPoint Size = Viewport->Viewport->GetSizeXY();
	Check(bReturned
	      ? TEXT("returned-entry-visible")
	      : TEXT("initial-entry-visible"),
	             Count == 1 && Widget->IsVisible() && InViewport(HostRect, Size) && InViewport(JoinRect, Size) &&
	                 InViewport(AddressRect, Size),
	             FString::Printf(TEXT("viewport=%dx%d host=(%.1f,%.1f)-(%.1f,%.1f) join=(%.1f,%.1f)-(%.1f,%.1f)"),
	                                  Size.X, Size.Y, HostRect.Min.X, HostRect.Min.Y, HostRect.Max.X, HostRect.Max.Y,
	                                  JoinRect.Min.X, JoinRect.Min.Y, JoinRect.Max.X, JoinRect.Max.Y));
	const UButton* Host = Cast<UButton>(Widget->WidgetTree->FindWidget(TEXT("Host")));
	const UButton* Join = Cast<UButton>(Widget->WidgetTree->FindWidget(TEXT("Join")));
	Check(TEXT("entry-buttons-enabled"),
	           Host && Join && Host->GetIsEnabled() && Join->GetIsEnabled() && !Controller.IsEntryBusy());
	Check(TEXT("entry-has-no-match-state-or-match-mode"),
	           GetWorld()->GetGameState<ALDGameState>() == nullptr &&
	               GetWorld()->GetAuthGameMode<ALDGameMode>() == nullptr &&
	               GetWorld()->GetAuthGameMode<ALDEntryGameMode>() != nullptr);
	if (bReturned && Role == TEXT("client"))
	{
		const UTextBlock* Feedback = Cast<UTextBlock>(Widget->WidgetTree->FindWidget(TEXT("Feedback")));
		const FString Expected = Controller.GetEntryFeedback().ToString();
		Check(TEXT("network-error-visible-in-entry"), Feedback && Feedback->IsVisible() && !Expected.IsEmpty() &&
		                                                  Feedback->GetText().ToString() == Expected &&
		                                                  Expected.Contains(TEXT("연결")), Expected);
	}
	return true;
}

void ULDG3EntryProbeSubsystem::TickEntry(ALDEntryPlayerController& Controller, double Now)
{
	if (EntryAt == 0)
	{
		if (!InspectEntry(Controller, false))
			return;
		EntryAt = Now;
		Capture(TEXT("entry-initial"));
		WriteProgress(TEXT("entry-idle"));
	}
	if (GetWorld()->GetGameState<ALDGameState>() || GetWorld()->GetAuthGameMode<ALDGameMode>() ||
	    Controller.IsEntryBusy())
	{
		Fail(TEXT("entry-unexpected-match-or-travel-before-button"));
		return;
	}
	EntryIdleSeconds = Now - EntryAt;
	if (!bIdleVerified && EntryIdleSeconds >= 35)
	{
		if (!InspectEntry(Controller, false))
			return;
		bIdleVerified = true;
		Check(TEXT("entry-idle-at-least-35-seconds-without-match-timeout"), true,
		           FString::SanitizeFloat(EntryIdleSeconds));
		Capture(TEXT("entry-idle35"));
		WriteProgress(TEXT("entry-idle-verified"));
		return; // Keep this frame available for the pre-travel screenshot.
	}
	if (!bIdleVerified || EntryIdleSeconds < 36)
		return;
	if (Role == TEXT("client") && !ReadPeerStage(TEXT("progress.json"), TEXT("late-join-ready")))
		return;
	if (Role == TEXT("client"))
		Controller.SetJoinAddressText(PeerAddress);
	FBox2D Rect;
	if (!Controller.GetEntryActionScreenRect(Role == TEXT("host"), Rect))
		return;
	bTravelClicked = true;
	Check(Role == TEXT("host") ? TEXT("host-slate-button") : TEXT("late-join-slate-button"), Click(Rect));
	WriteProgress(TEXT("travel-clicked"));
}

void ULDG3EntryProbeSubsystem::ObservePhase(const FLDBattleSnapshot& Battle)
{
	if (LastPhase == static_cast<int32>(Battle.Phase) && LastRevision == Battle.Revision)
		return;
	TSharedPtr<FJsonObject> Item = BattleJson(Battle);
	Item->SetNumberField(TEXT("localWorldSeconds"), GetWorld()->GetTimeSeconds());
	Item->SetNumberField(TEXT("wallSeconds"), FPlatformTime::Seconds() - StartedAt);
	PhaseObservations.Add(MakeShared<FJsonValueObject>(Item));
	LastPhase = static_cast<int32>(Battle.Phase);
	LastRevision = Battle.Revision;
}

bool ULDG3EntryProbeSubsystem::InspectTerminal(ALDPlayerController& Controller, const FLDBattleSnapshot& Battle)
{
	int32 Count = 0;
	ULDResultWidget* Result = VisibleWidget<ULDResultWidget>(Controller, Count);
	FBox2D Rect;
	if (!Result || !Result->WidgetTree || !Controller.GetReturnButtonScreenRect(Rect))
		return false;
	const UTextBlock* Label = Cast<UTextBlock>(Result->WidgetTree->FindWidget(TEXT("ResultText")));
	const FString Text = Label ? Label->GetText().ToString() : FString();
	const FIntPoint Size = GetWorld()->GetGameViewport()->Viewport->GetSizeXY();
	Check(TEXT("loading-timeout-result-visible"),
	           Count == 1 && Result->IsVisible() && InViewport(Rect, Size) &&
	               Text.Contains(TEXT("매치 종료")) && Text.Contains(TEXT("참가자 접속 대기 시간 초과")), Text);
	const FLDBoardSnapshot BeforeBoard = Controller.GetBoardSnapshot();
	const FLDEconomySnapshot BeforeEconomy = Controller.GetEconomySnapshot();
	const FLDCommandResult BeforeResult = Controller.GetLastResult();
	Check(TEXT("terminal-intents-closed"), !Controller.CanUseGameplayActions() && !Controller.HasPendingCommand() &&
	                                           !Controller.RequestSummon() && !Controller.RequestMergeSelection() &&
	                                           !Controller.RequestSellSelection() && !Controller.RequestMove(1, 0) &&
	                                           !Controller.SubmitLocalCommand(FLDCommand()));
	Check(
	    TEXT("terminal-local-state-unchanged"),
	         FLDBoardSnapshot::StaticStruct()->CompareScriptStruct(&BeforeBoard, &Controller.GetBoardSnapshot(), 0) &&
	             FLDEconomySnapshot::StaticStruct()->CompareScriptStruct(&BeforeEconomy,
	                                                                     &Controller.GetEconomySnapshot(), 0) &&
	             FLDCommandResult::StaticStruct()->CompareScriptStruct(&BeforeResult, &Controller.GetLastResult(), 0));
	int32 HUDCount = 0;
	if (ULDGameplayWidget* HUD = VisibleWidget<ULDGameplayWidget>(Controller, HUDCount))
	{
		bool bDisabled = true;
		for (const FName Name : {FName(TEXT("Summon")), FName(TEXT("Merge")), FName(TEXT("Sell"))})
		{
			const UButton* Button = HUD->WidgetTree ? Cast<UButton>(HUD->WidgetTree->FindWidget(Name)) : nullptr;
			bDisabled &= Button && !Button->GetIsEnabled();
		}
		Check(TEXT("present-gameplay-buttons-disabled"), bDisabled);
	}
	TerminalObservation = BattleJson(Battle);
	TerminalObservation->SetStringField(TEXT("renderedText"), Text);
	TerminalObservation->SetNumberField(TEXT("visibleResults"), Count);
	TerminalObservation->SetNumberField(TEXT("visibleGameplayHUDs"), HUDCount);
	const ALDPlayerState* Player = Controller.GetPlayerState<ALDPlayerState>();
	TerminalObservation->SetStringField(
	    TEXT("connectionEpoch"),
	         FString::Printf(TEXT("%llu"), Player ? Player->GetParticipantContext().ConnectionEpoch : uint64(0)));
	TerminalObservation->SetBoolField(TEXT("participantContextValid"),
	                                       Player && Player->GetParticipantContext().IsValid());
	OldResult = Result;
	return true;
}

void ULDG3EntryProbeSubsystem::TickMatch(ALDPlayerController& Controller, const FLDBattleSnapshot& Battle, double Now)
{
	if (!bTravelClicked)
	{
		Fail(TEXT("match-opened-without-entry-slate-click"));
		return;
	}
	ObservePhase(Battle);
	ALDGameMode* Mode = GetWorld()->GetAuthGameMode<ALDGameMode>();
	if (MatchObservedAt == 0)
	{
		MatchObservedAt = Now;
		OldMatchWorld = GetWorld();
		OldController = &Controller;
		Check(TEXT("native-match-controller-and-role"),
		           Controller.IsLocalController() &&
		               (Role == TEXT("host") ? Mode != nullptr && GetWorld()->GetNetMode() == NM_ListenServer
		                                     : Mode == nullptr && GetWorld()->GetNetMode() == NM_Client));
	}
	if (Battle.Phase == ELDMatchPhase::Loading)
	{
		bSawLoading = true;
		if (Role == TEXT("client"))
		{
			Fail(TEXT("late-client-resumed-loading"));
			return;
		}
		if (Now - LastProgressAt >= 1)
			WriteProgress(TEXT("host-loading"));
		return;
	}
	if (!Battle.IsTerminal())
	{
		Fail(TEXT("late-join-unexpected-preparing-or-running"));
		return;
	}
	if (TerminalAt == 0)
	{
		TerminalAt = Now;
		TimeoutBattle = Battle;
		Check(TEXT("actual-loading-timeout-boundary"),
		    Battle.MatchId.IsValid() && Battle.Phase == ELDMatchPhase::Aborted &&
		        Battle.Result == ELDMatchResult::Aborted && Battle.ResultReason == ELDResultReason::LoadingTimeout &&
		        Battle.LoadingDeadlineServerSeconds >= 30 &&
		        FMath::IsNearlyEqual(Battle.ResultServerSeconds, Battle.LoadingDeadlineServerSeconds, 0.000001) &&
		        Battle.WaveIndex == 0 && Battle.ActiveEnemyCount == 0);
		if (Role == TEXT("host"))
			Check(TEXT("host-observed-real-loading-before-timeout"), bSawLoading);
	}
	if (!SameBattle(Battle, TimeoutBattle))
	{
		Fail(TEXT("terminal-snapshot-changed-after-late-join"));
		return;
	}
	if (!bTerminalVerified && Now - TerminalAt >= 1)
	{
		if (!InspectTerminal(Controller, Battle))
		{
			if (Now - TerminalAt > 15)
				Fail(TEXT("terminal-result-ui-missing"));
			return;
		}
		bTerminalVerified = true;
		TerminalVerifiedAt = Now;
		Capture(Role == TEXT("host") ? TEXT("loading-timeout") : TEXT("late-terminal"));
	}
	if (Role == TEXT("host"))
	{
		if (GetWorld()->GetTimeSeconds() >= Battle.LoadingDeadlineServerSeconds + 1 && bTerminalVerified &&
		    !Peer.IsValid())
		{
			if (Now - LastProgressAt >= 1 || CurrentStage != TEXT("late-join-ready"))
				WriteProgress(TEXT("late-join-ready"));
			for (TActorIterator<ALDPlayerController> It(GetWorld()); It; ++It)
			{
				if (It->IsLocalController() || !It->GetNetConnection())
					continue;
				FActorSpawnParameters Params;
				Params.Owner = *It;
				ALDG3EntryProbePeer* Spawned =
				    GetWorld()->SpawnActor<ALDG3EntryProbePeer>(ALDG3EntryProbePeer::StaticClass(), Params);
				if (!Spawned)
				{
					Fail(TEXT("observation-peer-spawn-failed"));
					return;
				}
				Spawned->TimeoutBattle = TimeoutBattle;
				Spawned->ForceNetUpdate();
				Peer = Spawned;
				break;
			}
		}
		if (Peer.IsValid() && Peer->bAcknowledged && !bReturnClicked)
		{
			if (PeerAcknowledgedAt == 0)
			{
				PeerAcknowledgedAt = Now;
				Capture(TEXT("late-terminal"));
				return;
			}
			if (Now - PeerAcknowledgedAt < 1)
				return;
			Check(TEXT("late-client-real-network-terminal-ack"), Peer->bClientPassed);
			Check(TEXT("host-terminal-immutable-after-late-client"), SameBattle(Battle, TimeoutBattle));
			Check(TEXT("host-gameplay-services-stopped-and-state-pristine"),
			           Mode && !Mode->CanAcceptCommands() && !Mode->IsLogicTimerActive() && Mode->GetBoardManager() &&
			               Mode->GetEconomyService() && Mode->GetCommandProcessor() &&
			               Mode->GetBoardManager()->GetSnapshot(0).Population == 0 &&
			               Mode->GetEconomyService()->GetSnapshot(0).Gold == 100 &&
			               Mode->GetEconomyService()->GetSnapshot(0).PaidSummonCount == 0 &&
			               Mode->GetCommandProcessor()->GetCachedResultCount(0) == 0);
			FBox2D Rect;
			if (!Controller.GetReturnButtonScreenRect(Rect))
			{
				Fail(TEXT("host-return-rect-missing"));
				return;
			}
			bReturnClicked = true;
			// Both events are delivered before the product GI's deferred travel ticker runs.
			Check(TEXT("host-first-result-return-slate"), Click(Rect));
			++ReturnClicks;
			const ULDGameInstance* Instance = Cast<ULDGameInstance>(GetGameInstance());
			Check(TEXT("first-return-started-deferred-travel"), Instance && Instance->IsTravelPending());
			Check(TEXT("host-second-result-return-slate"), Click(Rect));
			++ReturnClicks;
			Check(TEXT("duplicate-return-intent-refused"), !Controller.RequestReturnToEntry());
			WriteProgress(TEXT("host-return-requested"));
		}
	}
	else if (bTerminalVerified && Now - TerminalVerifiedAt >= 2)
	{
		if (!Peer.IsValid())
		{
			for (TActorIterator<ALDG3EntryProbePeer> It(GetWorld()); It; ++It)
			{
				if (It->GetOwner() == &Controller && It->TimeoutBattle.MatchId.IsValid())
				{
					Peer = *It;
					break;
				}
			}
		}
		if (Peer.IsValid() && CurrentStage != TEXT("client-terminal-acknowledged"))
		{
			Check(TEXT("client-terminal-matches-authority-over-network"), SameBattle(Battle, Peer->TimeoutBattle));
			Peer->ServerObserveTerminal(Battle.MatchId, Battle.Revision, !bFailed);
			WriteProgress(TEXT("client-terminal-acknowledged"));
		}
	}
}

void ULDG3EntryProbeSubsystem::ObserveNetworkFailure(UWorld* World, UNetDriver* Driver, ENetworkFailure::Type Type,
                                                     const FString& Error)
{
	if (!World || World->GetGameInstance() != GetGameInstance() || bFinished)
		return;
	++NetworkFailures;
	const bool bInMatch = World == OldMatchWorld.Get() &&
	                      Cast<ALDPlayerController>(GetGameInstance()->GetFirstLocalPlayerController()) != nullptr;
	bSawNetworkFailureInMatch |= bInMatch;
	TSharedPtr<FJsonObject> Item = MakeShared<FJsonObject>();
	Item->SetNumberField(TEXT("type"), static_cast<int32>(Type));
	Item->SetStringField(TEXT("error"), Error);
	Item->SetStringField(TEXT("world"), World->GetPathName());
	Item->SetBoolField(TEXT("stillInMatchInsideFailureCallback"), bInMatch);
	Item->SetNumberField(TEXT("wallSeconds"), FPlatformTime::Seconds() - StartedAt);
	NetworkObservations.Add(MakeShared<FJsonValueObject>(Item));
}

void ULDG3EntryProbeSubsystem::TickReturned(ALDEntryPlayerController& Controller, double Now)
{
	if (ReturnedAt == 0)
	{
		ReturnedAt = Now;
		Check(TEXT("returned-after-observed-terminal"), bTerminalVerified && (Role != TEXT("host") || bReturnClicked));
		if (Role == TEXT("client"))
		{
			Check(TEXT("peer-return-caused-real-network-failure"), NetworkFailures > 0 && bSawNetworkFailureInMatch);
			Check(TEXT("client-never-requested-return"), ReturnClicks == 0 && !bReturnClicked);
		}
		WriteProgress(TEXT("returned-settling"));
	}
	if (Now - ReturnedAt < 3)
		return;
	if (!bReturnedVerified)
	{
		if (!InspectEntry(Controller, true))
			return;
		if (OldResult.IsValid())
			Check(TEXT("old-result-return-subscription-removed"), !OldResult->OnReturnRequested.IsBound());
		// Observe normal game travel cleanup only; do not root, manually clean, or destroy the former World.
		CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
		CleanupObservation = MakeShared<FJsonObject>();
		const bool bWorldCollected = OldMatchWorld.IsStale(false, true);
		const bool bControllerCollected = OldController.IsStale(false, true);
		const bool bResultCollected = OldResult.IsStale(false, true);
		Check(TEXT("old-world-controller-result-collected"),
		           bWorldCollected && bControllerCollected && bResultCollected);
		CleanupObservation->SetBoolField(TEXT("oldWorldCollected"), bWorldCollected);
		CleanupObservation->SetBoolField(TEXT("oldControllerCollected"), bControllerCollected);
		CleanupObservation->SetBoolField(TEXT("oldResultCollected"), bResultCollected);
		CleanupObservation->SetNumberField(TEXT("stableEntrySeconds"), Now - ReturnedAt);
		CleanupObservation->SetStringField(TEXT("gcPolicy"),
		                                        TEXT("GARBAGE_COLLECTION_KEEPFLAGS; no manual World teardown"));
		bReturnedVerified = true;
		Capture(TEXT("entry-returned"));
		WriteProgress(TEXT("entry-returned"));
		TSharedPtr<FJsonObject> Marker = MakeShared<FJsonObject>();
		Marker->SetStringField(TEXT("runId"), RunId);
		Marker->SetStringField(TEXT("role"), Role);
		Marker->SetStringField(TEXT("stage"), TEXT("entry-returned"));
		SaveJson(OutputDirectory / TEXT("entry-returned.json"), Marker);
		return;
	}
	if (Now - ReturnedAt >= 5 && ReadPeerStage(TEXT("entry-returned.json"), TEXT("entry-returned")))
		Finish();
}

void ULDG3EntryProbeSubsystem::Tick(float DeltaTime)
{
	if (bFinished)
		return;
	const double Now = FPlatformTime::Seconds();
	if (Now - StartedAt > TimeoutSeconds)
	{
		Fail(TEXT("entry-suite-timeout"));
		return;
	}
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld() || !GetGameInstance())
		return;
	if (!bOutputReady)
	{
		Fail(TEXT("output-directory-unavailable"));
		return;
	}
	APlayerController* Local = GetGameInstance()->GetFirstLocalPlayerController();
	if (Local && Local->GetWorld() != World)
		return;
	if (ALDEntryPlayerController* Entry = Cast<ALDEntryPlayerController>(Local))
	{
		if (MatchObservedAt > 0)
			TickReturned(*Entry, Now);
		else if (!bTravelClicked)
			TickEntry(*Entry, Now);
		return;
	}
	ALDPlayerController* Controller = Cast<ALDPlayerController>(Local);
	ALDGameState* State = World->GetGameState<ALDGameState>();
	if (Controller && State && State->GetBattleSnapshot().MatchId.IsValid())
		TickMatch(*Controller, State->GetBattleSnapshot(), Now);
}

void ULDG3EntryProbeSubsystem::Fail(const FString& Reason)
{
	Check(Reason, false);
	Finish();
}

void ULDG3EntryProbeSubsystem::Finish()
{
	if (bFinished)
		return;
	Check(TEXT("entry-terminal-return-sequence-complete"),
	           bIdleVerified && bTravelClicked && bTerminalVerified && bReturnedVerified);
	bFinished = true;
	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("runId"), RunId);
	Root->SetStringField(TEXT("role"), Role);
	Root->SetStringField(TEXT("probe"), TEXT("G3Entry"));
	Root->SetNumberField(TEXT("processId"), FPlatformProcess::GetCurrentProcessId());
	Root->SetStringField(TEXT("status"), bFailed ? TEXT("Fail") : TEXT("Pass"));
	Root->SetStringField(TEXT("scope"),
	    TEXT("Native Entry idle / actual late Join / natural LoadingTimeout / host-first Slate return / actual client network failure. No wave, outcome, clock or participant injection. Coordination files schedule only."));
	Root->SetStringField(TEXT("lastStage"), CurrentStage);
	Root->SetNumberField(TEXT("wallSeconds"), FPlatformTime::Seconds() - StartedAt);
	Root->SetNumberField(TEXT("entryIdleSeconds"), EntryIdleSeconds);
	Root->SetNumberField(TEXT("returnSlateClicks"), ReturnClicks);
	Root->SetNumberField(TEXT("networkFailureCount"), NetworkFailures);
	Root->SetArrayField(TEXT("checks"), Checks);
	Root->SetArrayField(TEXT("phaseObservations"), PhaseObservations);
	Root->SetArrayField(TEXT("networkFailure"), NetworkObservations);
	Root->SetObjectField(TEXT("timeoutSnapshot"), BattleJson(TimeoutBattle));
	if (TerminalObservation)
		Root->SetObjectField(TEXT("terminalView"), TerminalObservation);
	if (CleanupObservation)
		Root->SetObjectField(TEXT("cleanup"), CleanupObservation);
	for (const FString& Name :
	     {FString(TEXT("entry-initial")), FString(TEXT("entry-idle35")), FString(TEXT("late-terminal")),
	                                                                             FString(TEXT("entry-returned"))})
	{
		Check(TEXT("screenshot-written-") + Name,
		           IFileManager::Get().FileSize(*(OutputDirectory / (Name + TEXT(".png")))) > 0);
	}
	Root->SetArrayField(TEXT("checks"), Checks);
	Root->SetStringField(TEXT("status"), bFailed ? TEXT("Fail") : TEXT("Pass"));
	Root->SetStringField(TEXT("result"), bFailed ? TEXT("Fail") : TEXT("Pass"));
	Root->SetBoolField(TEXT("passed"), !bFailed);
	if (bOutputReady)
		SaveJson(OutputDirectory / TEXT("result.json"), Root);
	FPlatformMisc::RequestExit(false);
}
