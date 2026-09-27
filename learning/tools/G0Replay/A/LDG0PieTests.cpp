// Provided G0 replay verification only. Product Core/Data remain the independent A 4cc3e0f sources.
#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Core/LDGameMode.h"
#include "Core/LDGameState.h"
#include "Core/LDPlayerState.h"
#include "Data/LDGameData.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/NetConnection.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "ImageUtils.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Tests/AutomationEditorCommon.h"
#include "TimerManager.h"
#include "UObject/ObjectKey.h"
#include "UObject/UnrealType.h"
#include "Widgets/SWindow.h"

namespace
{
	FString ExportPlayConfig(const ULevelEditorPlaySettings& Settings)
	{
		FString Result;
		for (TFieldIterator<FProperty> Property(Settings.GetClass()); Property; ++Property)
		{
			if (Property->HasAnyPropertyFlags(CPF_Config))
			{
				FString Value;
				Property->ExportText_InContainer(0, Value, &Settings, nullptr, nullptr, PPF_None);
				Result += Property->GetName() + TEXT("=") + Value + TEXT("\n");
			}
		}
		return Result;
	}

	struct FG0PIEProof : public TSharedFromThis<FG0PIEProof>
	{
		FAutomationTestBase* Test = nullptr;
		ULevelEditorPlaySettings* Original = nullptr;
		FString OriginalConfig;
		FString OutputDirectory;
		TArray<TSharedPtr<FJsonValue>> Observations;
		TWeakObjectPtr<UWorld> HostWorld;
		TWeakObjectPtr<UWorld> ClientWorld;
		TWeakObjectPtr<ALDGameMode> Mode;
		TWeakObjectPtr<APlayerController> Host;
		TWeakObjectPtr<APlayerController> RemoteAuthority;
		TWeakObjectPtr<APlayerController> Third;
		FObjectKey ThirdKey;
		FDelegateHandle PostLoginHandle;
		FDelegateHandle LogoutHandle;
		FDelegateHandle NetworkFailureHandle;
		FDelegateHandle ClientStateHandle;
		FGuid FirstMatchId;
		FLDParticipantContext OriginalParticipants[2];
		int32 ClientNotifications = 0;
		int32 CompletedSessions = 0;
		bool bThirdRequested = false;
		bool bThirdPostLogin = false;
		bool bThirdLogout = false;
		bool bThirdHadNetConnection = false;
		bool bThirdRejected = false;
		bool bThirdJoinWindow = false;
		bool bTeardownRequested = false;
		bool bMissingData = false;
		bool bRestored = false;

		~FG0PIEProof()
		{
			RemoveParticipantObservers();
			if (GEngine)
			{
				GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
			}
			Restore();
			if (Original)
			{
				Original->RemoveFromRoot();
			}
		}
		void Record(const FString& Stage, const FString& Detail)
		{
			TSharedPtr<FJsonObject> Item = MakeShared<FJsonObject>();
			Item->SetStringField(TEXT("stage"), Stage);
			Item->SetStringField(TEXT("detail"), Detail);
			Item->SetNumberField(TEXT("wallSeconds"), FPlatformTime::Seconds());
			Observations.Add(MakeShared<FJsonValueObject>(Item));
			Test->AddInfo(Stage + TEXT(": ") + Detail);
		}
		void RemoveParticipantObservers()
		{
			FGameModeEvents::OnGameModePostLoginEvent().Remove(PostLoginHandle);
			FGameModeEvents::OnGameModeLogoutEvent().Remove(LogoutHandle);
			if (ClientWorld.IsValid())
			{
				if (ALDGameState* State = ClientWorld->GetGameState<ALDGameState>())
				{
					State->OnMatchStateChanged.Remove(ClientStateHandle);
				}
			}
			PostLoginHandle.Reset();
			LogoutHandle.Reset();
			ClientStateHandle.Reset();
		}
		void ObserveNetworkLifetime()
		{
			const TWeakPtr<FG0PIEProof> Weak = AsShared();
			NetworkFailureHandle = GEngine->OnNetworkFailure().AddLambda(
			    [Weak](UWorld* World, UNetDriver* Driver, ENetworkFailure::Type Type, const FString& Error)
			    {
				    const TSharedPtr<FG0PIEProof> P = Weak.Pin();
				    if (!P)
				    {
					    return;
				    }
				    const bool bDisconnect =
				        Type == ENetworkFailure::ConnectionLost || Type == ENetworkFailure::FailureReceived;
				    const bool bPIEWorld = World && World->WorldType == EWorldType::PIE;
				    const bool bExpectedThird = P->bThirdJoinWindow && P->bThirdPostLogin && bPIEWorld &&
				                                World != P->HostWorld.Get() && World != P->ClientWorld.Get() &&
				                                bDisconnect;
				    const bool bExpectedTeardown = P->bTeardownRequested && bPIEWorld && bDisconnect;
				    P->Test->TestTrue(
				        TEXT("Network failure belongs only to intentional third rejection or PIE teardown"),
				             bExpectedThird || bExpectedTeardown);
				    P->Record(
				        TEXT("network-failure"),
				             FString::Printf(TEXT("expectedThird=%d expectedTeardown=%d world=%s driver=%s type=%s %s"),
				                                  bExpectedThird, bExpectedTeardown, *GetNameSafe(World),
				                                  *GetNameSafe(Driver), ENetworkFailure::ToString(Type), *Error));
			    });
			// BroadcastNetworkFailure logs before broadcasting. Ignore only these two log forms, then validate
			// every corresponding event for the whole proof lifetime, including startup and the second PIE.
			Test->AddExpectedError(
			    TEXT("UEngine::BroadcastNetworkFailure: FailureType = (ConnectionLost|FailureReceived),"),
			         EAutomationExpectedErrorFlags::Contains, -1);
		}
		void ObserveThirdJoin()
		{
			bThirdRequested = true;
			bThirdJoinWindow = true;
			const TWeakPtr<FG0PIEProof> Weak = AsShared();
			PostLoginHandle = FGameModeEvents::OnGameModePostLoginEvent().AddLambda(
			    [Weak](AGameModeBase* InMode, APlayerController* Player)
			    {
				    const TSharedPtr<FG0PIEProof> P = Weak.Pin();
				    if (P && InMode == P->Mode.Get() && Player != P->Host.Get() && Player != P->RemoteAuthority.Get())
				    {
					    P->Third = Player;
					    P->ThirdKey = FObjectKey(Player);
					    P->bThirdPostLogin = true;
					    P->bThirdHadNetConnection = Cast<UNetConnection>(Player->Player) != nullptr;
					    P->Record(TEXT("third-postlogin"),
					                   FString::Printf(TEXT("actual remote=%d actor=%s"), P->bThirdHadNetConnection,
					                                        *Player->GetPathName()));
				    }
			    });
			LogoutHandle = FGameModeEvents::OnGameModeLogoutEvent().AddLambda(
			    [Weak](AGameModeBase* InMode, AController* Player)
			    {
				    const TSharedPtr<FG0PIEProof> P = Weak.Pin();
				    if (!P || InMode != P->Mode.Get())
				    {
					    return;
				    }
				    if (P->bThirdPostLogin && FObjectKey(Player) == P->ThirdKey)
				    {
					    P->bThirdLogout = true;
					    P->Record(TEXT("third-logout"), TEXT("rejected remote Controller entered actual Logout"));
				    }
				    else if (Player == P->Host.Get() || Player == P->RemoteAuthority.Get())
				    {
					    P->Test->AddError(TEXT("Third admission displaced an original participant"));
				    }
			    });
			GEditor->RequestLateJoin();
		}
		void Restore()
		{
			if (!bRestored && Original)
			{
				ULevelEditorPlaySettings* Settings = GetMutableDefault<ULevelEditorPlaySettings>();
				for (TFieldIterator<FProperty> Property(Settings->GetClass()); Property; ++Property)
				{
					if (Property->HasAnyPropertyFlags(CPF_Config))
					{
						Property->CopyCompleteValue_InContainer(Settings, Original);
					}
				}
				Settings->SaveConfig();
				bRestored = true;
			}
		}
		void Capture(APlayerController& Player, const FString& Name)
		{
			ULocalPlayer* LocalPlayer = Player.GetLocalPlayer();
			UGameViewportClient* Viewport = LocalPlayer ? LocalPlayer->ViewportClient.Get() : nullptr;
			const TSharedPtr<SWindow> Window = Viewport ? Viewport->GetWindow() : nullptr;
			if (!Window.IsValid())
			{
				Test->AddError(TEXT("Actual G0 PIE viewport window is missing"));
				return;
			}
			TArray<FColor> Pixels;
			FIntVector Size;
			if (!FSlateApplication::Get().TakeScreenshot(Window.ToSharedRef(), Pixels, Size) || Pixels.IsEmpty())
			{
				Test->AddError(TEXT("Actual G0 PIE window capture failed"));
				return;
			}
			TArray64<uint8> PNG;
			FImageUtils::PNGCompressImageArray(Size.X, Size.Y, Pixels, PNG);
			const FString Path = OutputDirectory / (Name + TEXT(".png"));
			Test->TestTrue(TEXT("G0 actual viewport screenshot saved"), FFileHelper::SaveArrayToFile(PNG, *Path));
			Record(TEXT("capture"), FString::Printf(TEXT("%s %dx%d; G0 has no gameplay HUD"), *Path, Size.X, Size.Y));
		}
		void Save()
		{
			const bool bExpectedErrorsMet = Test->HasMetExpectedErrors();
			TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
			Root->SetStringField(TEXT("kind"), TEXT("actual-editor-PIE-independent-G0-A"));
			Root->SetStringField(TEXT("productSource"), TEXT("4cc3e0fd63d074df2d2e4568cbc0889cd0ecc2a6"));
			Root->SetStringField(TEXT("result"),
			                          Test->HasAnyErrors() || !bExpectedErrorsMet ? TEXT("Fail") : TEXT("Pass"));
			Root->SetBoolField(TEXT("expectedErrorsMet"), bExpectedErrorsMet);
			Root->SetStringField(TEXT("passScope"), TEXT("Proof observations and expected errors at save time only; final Automation report success is also required"));
			Root->SetNumberField(TEXT("processId"), FPlatformProcess::GetCurrentProcessId());
			Root->SetNumberField(TEXT("completedSessions"), CompletedSessions);
			Root->SetBoolField(TEXT("settingsRestored"), bRestored);
			Root->SetBoolField(TEXT("missingDataFixture"), bMissingData);
			Root->SetBoolField(TEXT("thirdRemoteRejected"), bThirdRejected);
			Root->SetStringField(TEXT("map"), TEXT("/Game/TopDown/Lvl_TopDown"));
			Root->SetStringField(TEXT("gameMode"), TEXT("native ALDGameMode; no generated Blueprint validation"));
			Root->SetArrayField(TEXT("observations"), Observations);
			FString Json;
			FJsonSerializer::Serialize(Root, TJsonWriterFactory<>::Create(&Json));
			Test->TestTrue(TEXT("PIE proof JSON saved"),
			                    FFileHelper::SaveStringToFile(Json, *(OutputDirectory / TEXT("pie-proof.json"))));
		}
	};

	bool FindPair(APlayerController*& Host, APlayerController*& Client, ALDGameMode*& Mode)
	{
		int32 Count = 0;
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			UWorld* World = Context.World();
			if (Context.WorldType != EWorldType::PIE || !World)
			{
				continue;
			}
			++Count;
			for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
			{
				APlayerController* PC = It->Get();
				if (!PC || !PC->IsLocalController())
				{
					continue;
				}
				if (World->GetNetMode() == NM_ListenServer)
				{
					Host = PC;
					Mode = World->GetAuthGameMode<ALDGameMode>();
				}
				else if (World->GetNetMode() == NM_Client)
				{
					Client = PC;
				}
			}
		}
		return Count == 2 && Host && Client && Mode;
	}

	class FVerifyG0APIE final : public IAutomationLatentCommand
	{
	public:
		FVerifyG0APIE(TSharedRef<FG0PIEProof> InProof, int32 InRound) : Proof(InProof), Round(InRound) {}
		virtual bool Update() override
		{
			if (Proof->Test->HasAnyErrors())
			{
				Proof->Record(
				    TEXT("verification-stopped"),
				         FString::Printf(TEXT("round%d retained prior failure; proceed to engine cleanup"), Round));
				return true;
			}
			const double Now = FPlatformTime::Seconds();
			if (StartedAt == 0)
			{
				StartedAt = Now;
			}
			if (Now - StartedAt > 45)
			{
				Proof->Test->AddError(
				    FString::Printf(TEXT("Independent A PIE timed out: round%d stage%d"), Round, Stage));
				return true;
			}
			APlayerController* Host = Proof->Host.Get();
			APlayerController* Client = LocalClient.Get();
			ALDGameMode* Mode = Proof->Mode.Get();
			if (Stage == 0)
			{
				Host = nullptr;
				Client = nullptr;
				Mode = nullptr;
				if (!FindPair(Host, Client, Mode))
				{
					return false;
				}
			}
			if (!Host || !Client || !Mode)
			{
				Proof->Test->AddError(TEXT("Original host/client disappeared during G0 PIE verification"));
				return true;
			}
			ALDGameState* ServerState = Host->GetWorld()->GetGameState<ALDGameState>();
			ALDGameState* ClientState = Client->GetWorld()->GetGameState<ALDGameState>();
			if (!ServerState || !ClientState)
			{
				return false;
			}
			if (Stage == 0)
			{
				const ELDMatchPhase Expected = Proof->bMissingData ? ELDMatchPhase::Aborted : ELDMatchPhase::Preparing;
				if (ServerState->GetPhase() != Expected || ClientState->GetPhase() != Expected)
				{
					return false;
				}
				Proof->HostWorld = Host->GetWorld();
				Proof->ClientWorld = Client->GetWorld();
				Proof->Host = Host;
				Proof->Mode = Mode;
				LocalClient = Client;
				Proof->Test->TestTrue(TEXT("Actual independent G0 server/client Worlds differ"),
				                           Host->GetWorld() != Client->GetWorld());
				Proof->Test->TestTrue(TEXT("Native independent A mode selected"),
				                           Mode->GetClass() == ALDGameMode::StaticClass());
				Proof->Test->TestTrue(TEXT("Independent A still uses the base PlayerController"),
				                           Host->GetClass() == APlayerController::StaticClass() &&
				                               Client->GetClass() == APlayerController::StaticClass());
				Proof->Test->TestFalse(TEXT("Independent A Stub never opens command admission"),
				                            Mode->CanAcceptCommands());
				if (Proof->bMissingData)
				{
					if (ClientState->GetReadinessReason() != ServerState->GetReadinessReason())
					{
						return false;
					}
					Proof->Test->TestTrue(TEXT("Missing input error identifies GameRules.json"),
					                           ServerState->GetReadinessReason().Contains(TEXT("GameRules.json")));
					Proof->Test->TestTrue(TEXT("Failed loader never publishes a validated snapshot"),
					                           Mode->GetGameData() && !Mode->GetGameData()->IsLoaded());
					Proof->Test->TestFalse(TEXT("Missing-data Abort refuses Running reentry"),
					                            ServerState->SetPhase(ELDMatchPhase::Running));
					Proof->Record(TEXT("missing-data-abort"), ServerState->GetReadinessReason());
					TerminalPhase = ELDMatchPhase::Aborted;
					Stage = 4;
					StageAt = Now;
					return false;
				}
				ALDPlayerState* Owner0 = Host->GetPlayerState<ALDPlayerState>();
				ALDPlayerState* Owner1 = Client->GetPlayerState<ALDPlayerState>();
				if (!Owner0 || !Owner1 || !Owner0->GetParticipantContext().IsValid() ||
				    !Owner1->GetParticipantContext().IsValid() ||
				    !ServerState->GetReadinessReason().Contains(TEXT("2/2")) || ClientState->GetReadinessReason() !=
				                                                                    ServerState->GetReadinessReason())
				{
					return false;
				}
				Proof->Test->TestEqual(TEXT("Actual replicated MatchId"), ClientState->GetMatchContext().MatchId,
				                            ServerState->GetMatchContext().MatchId);
				Proof->Test->TestEqual(TEXT("Owned host index0"), Owner0->GetPlayerIndex(), 0);
				Proof->Test->TestEqual(TEXT("Owned client index1"), Owner1->GetPlayerIndex(), 1);
				Proof->Test->TestTrue(TEXT("Participant epochs differ"),
				                           Owner0->GetParticipantContext().ConnectionEpoch !=
				                               Owner1->GetParticipantContext().ConnectionEpoch);
				ALDPlayerState* PublicHostOnClient = nullptr;
				for (APlayerState* Player : ClientState->PlayerArray)
				{
					ALDPlayerState* Candidate = Cast<ALDPlayerState>(Player);
					if (Candidate && Candidate->GetPlayerIndex() == 0)
					{
						PublicHostOnClient = Candidate;
					}
				}
				if (!PublicHostOnClient)
				{
					return false;
				}
				Proof->Test->TestFalse(TEXT("Remote participant private epoch is not replicated to the other owner"),
				                            PublicHostOnClient->GetParticipantContext().IsValid());
				Proof->Test->TestFalse(TEXT("Client cannot mutate server phase"),
				                            ClientState->SetPhase(ELDMatchPhase::Aborted));
				Proof->OriginalParticipants[0] = Owner0->GetParticipantContext();
				Proof->OriginalParticipants[1] = Owner1->GetParticipantContext();
				for (FConstPlayerControllerIterator It = Host->GetWorld()->GetPlayerControllerIterator(); It; ++It)
				{
					APlayerController* PC = It->Get();
					const ALDPlayerState* PS = PC ? PC->GetPlayerState<ALDPlayerState>() : nullptr;
					if (PS && PS->GetPlayerIndex() == 1)
					{
						Proof->RemoteAuthority = PC;
					}
				}
				if (!Proof->RemoteAuthority.IsValid())
				{
					return false;
				}
				const FGuid MatchId = ServerState->GetMatchContext().MatchId;
				if (Round == 0)
				{
					Proof->FirstMatchId = MatchId;
				}
				else
				{
					Proof->Test->TestTrue(TEXT("Restarted PIE owns a fresh match identity"),
					                           MatchId != Proof->FirstMatchId);
				}
				const ULDGameData* BeforeData = Mode->GetGameData();
				Mode->InitGameState();
				Mode->PostLogin(Host);
				Mode->PostLogin(Proof->RemoteAuthority.Get());
				Proof->Test->TestTrue(TEXT("Repeated InitGameState retains the same loader UObject"),
				                           Mode->GetGameData() == BeforeData);
				Proof->Test->TestTrue(TEXT("Same match initialization is idempotent"),
				                           ServerState->InitializeMatch(ServerState->GetMatchContext()));
				FLDMatchContext Other = ServerState->GetMatchContext();
				Other.MatchId = FGuid::NewGuid();
				Proof->Test->TestFalse(TEXT("Different match cannot overwrite the initialized state"),
				                            ServerState->InitializeMatch(Other));
				ALDPlayerState* ServerPlayers[2] = {Owner0, Proof->RemoteAuthority->GetPlayerState<ALDPlayerState>()};
				for (int32 Index = 0; Index < 2; ++Index)
				{
					Proof->Test->TestTrue(
					    TEXT("Same participant initialization is idempotent"),
					         ServerPlayers[Index]->InitializeParticipant(Proof->OriginalParticipants[Index]));
					Proof->Test->TestEqual(TEXT("Duplicate PostLogin keeps original epoch"),
					                            ServerPlayers[Index]->GetParticipantContext().ConnectionEpoch,
					                            Proof->OriginalParticipants[Index].ConnectionEpoch);
					FLDParticipantContext Changed = Proof->OriginalParticipants[Index];
					++Changed.ConnectionEpoch;
					Proof->Test->TestFalse(TEXT("Different participant cannot overwrite initialized identity"),
					                            ServerPlayers[Index]->InitializeParticipant(Changed));
				}
				Proof->ClientNotifications = 0;
				const TWeakPtr<FG0PIEProof> Weak = Proof;
				Proof->ClientStateHandle = ClientState->OnMatchStateChanged.AddLambda(
				    [Weak]()
				    {
					    if (const TSharedPtr<FG0PIEProof> P = Weak.Pin())
					    {
						    ++P->ClientNotifications;
					    }
				    });
				Marker =
				    FString::Printf(TEXT("G0 A independent PIE round%d Preparing 2/2; command Stub closed"), Round);
				ServerState->SetReadinessReason(Marker);
				Proof->Record(TEXT("worlds"),
				    FString::Printf(TEXT("round%d host=%s ListenServer owner0; client=%s Client owner1; match=%s; epochs=%llu/%llu"),
				        Round, *Host->GetWorld()->GetPathName(), *Client->GetWorld()->GetPathName(),
				        *MatchId.ToString(), Proof->OriginalParticipants[0].ConnectionEpoch,
				        Proof->OriginalParticipants[1].ConnectionEpoch));
				Stage = 1;
				StageAt = Now;
				return false;
			}
			if (Stage == 1)
			{
				if (ClientState->GetReadinessReason() != Marker || Proof->ClientNotifications == 0 ||
				    Now - StageAt < .5)
				{
					return false;
				}
				Proof->Capture(*Host, FString::Printf(TEXT("round%d-host-preparing"), Round));
				Proof->Capture(*Client, FString::Printf(TEXT("round%d-client-preparing"), Round));
				Proof->Record(
				    TEXT("replication"),
				         TEXT("Preparing/readiness observed by actual client OnRep; no G0 HUD or successful commands"));
				if (Round == 0)
				{
					Proof->ObserveThirdJoin();
					Stage = 2;
					StageAt = Now;
					return false;
				}
				Stage = 3;
			}
			if (Stage == 2)
			{
				if (!Proof->bThirdPostLogin || !Proof->bThirdLogout || Proof->Third.IsValid() || Now - StageAt < 1)
				{
					return false;
				}
				Proof->Test->TestTrue(TEXT("Third join used an actual remote NetConnection"),
				                           Proof->bThirdHadNetConnection);
				Proof->Test->TestEqual(TEXT("Third admission cannot overwrite match"),
				                            ServerState->GetMatchContext().MatchId, Proof->FirstMatchId);
				int32 Admitted = 0;
				for (FConstPlayerControllerIterator It = Host->GetWorld()->GetPlayerControllerIterator(); It; ++It)
				{
					APlayerController* PC = It->Get();
					const ALDPlayerState* Player = PC ? PC->GetPlayerState<ALDPlayerState>() : nullptr;
					if (Player && Player->GetParticipantContext().IsValid())
					{
						++Admitted;
						const int32 Index = Player->GetPlayerIndex();
						if (Proof->Test->TestTrue(TEXT("Only original participant indices remain"),
						                               Index == 0 || Index == 1))
						{
							Proof->Test->TestEqual(TEXT("Third rejection preserves original epoch"),
							                            Player->GetParticipantContext().ConnectionEpoch,
							                            Proof->OriginalParticipants[Index].ConnectionEpoch);
						}
					}
				}
				Proof->Test->TestEqual(TEXT("Exactly two admitted participants after real third rejection"), Admitted,
				                            2);
				Proof->bThirdRejected = Proof->bThirdHadNetConnection && Admitted == 2;
				Proof->bThirdJoinWindow = false;
				Proof->Record(
				    TEXT("third-rejected"),
				         TEXT("remote PostLogin followed by Logout/Destroy; original slots/epochs preserved"));
				Stage = 3;
			}
			if (Stage == 3)
			{
				// Explicit phase fixture, not combat victory: this G0 source has no battle services.
				TerminalPhase = Round == 0 ? ELDMatchPhase::Result : ELDMatchPhase::Aborted;
				if (Round == 0)
				{
					Proof->Test->TestTrue(TEXT("Public server phase enters Running for terminal fixture"),
					                           ServerState->SetPhase(ELDMatchPhase::Running));
				}
				Proof->Test->TestTrue(TEXT("Public server phase enters requested terminal fixture"),
				                           ServerState->SetPhase(TerminalPhase));
				Stage = 4;
				StageAt = Now;
				return false;
			}
			if (ClientState->GetPhase() != TerminalPhase || Now - StageAt < .25)
			{
				return false;
			}
			Proof->Test->TestFalse(TEXT("Terminal state rejects Preparing reentry"),
			                            ServerState->SetPhase(ELDMatchPhase::Preparing));
			Proof->Test->TestFalse(TEXT("Terminal state rejects Running reentry"),
			                            ServerState->SetPhase(ELDMatchPhase::Running));
			Proof->Test->TestTrue(TEXT("Identical terminal request is idempotent"),
			                           ServerState->SetPhase(TerminalPhase));
			Proof->Capture(*Host, FString::Printf(TEXT("round%d-host-terminal"), Round));
			Proof->Capture(*Client, FString::Printf(TEXT("round%d-client-terminal"), Round));
			Proof->RemoveParticipantObservers();
			// A test-owned timer bound to this real Mode proves ClearAllTimersForObject in its unchanged EndPlay.
			// The missing-data path has already called StopMatchServices; never add new work after that closure.
			FTimerHandle OwnedTimer;
			if (!Proof->bMissingData)
			{
				Mode->GetWorldTimerManager().SetTimer(OwnedTimer, Mode, &ALDGameMode::InitGameState, 60.f, true);
				Proof->Test->TestTrue(TEXT("Fixture owns a real Mode-bound timer before EndPlay"),
				                           Mode->GetWorldTimerManager().TimerExists(OwnedTimer));
			}
			Mode->EndPlay(EEndPlayReason::EndPlayInEditor);
			Mode->EndPlay(EEndPlayReason::EndPlayInEditor);
			Proof->Test->TestFalse(TEXT("Repeated EndPlay clears Mode-bound timer"),
			                            Mode->GetWorldTimerManager().TimerExists(OwnedTimer));
			Proof->Test->TestFalse(TEXT("Ended Mode never accepts commands"), Mode->CanAcceptCommands());
			Proof->Test->TestEqual(TEXT("Repeated EndPlay does not reverse terminal phase"), ServerState->GetPhase(),
			                            TerminalPhase);
			Proof->Record(TEXT("terminal"),
			                   FString::Printf(TEXT("round%d actual client phase=%d; repeat EndPlay; owned timer absent (installed=%d); phase unchanged"),
			                                        Round, static_cast<int32>(TerminalPhase), !Proof->bMissingData));
			++Proof->CompletedSessions;
			return true;
		}

	private:
		TSharedRef<FG0PIEProof> Proof;
		TWeakObjectPtr<APlayerController> LocalClient;
		int32 Round = 0;
		int32 Stage = 0;
		double StartedAt = 0;
		double StageAt = 0;
		FString Marker;
		ELDMatchPhase TerminalPhase = ELDMatchPhase::Result;
	};

	class FFinishG0APIE final : public IAutomationLatentCommand
	{
	public:
		FFinishG0APIE(TSharedRef<FG0PIEProof> InProof, bool bInFinal) : Proof(InProof), bFinal(bInFinal) {}
		virtual bool Update() override
		{
			if (StartedAt == 0)
			{
				StartedAt = FPlatformTime::Seconds();
			}
			int32 Remaining = 0;
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				Remaining += Context.WorldType == EWorldType::PIE ? 1 : 0;
			}
			if (Remaining && FPlatformTime::Seconds() - StartedAt < 15)
			{
				return false;
			}
			Proof->Test->TestEqual(TEXT("FEndPlayMap removes all actual PIE Worlds including rejected client"),
			                            Remaining, 0);
			Proof->RemoveParticipantObservers();
			Proof->Record(TEXT("cleanup"), FString::Printf(TEXT("PIE worlds remaining=%d"), Remaining));
			if (bFinal)
			{
				Proof->Restore();
				Proof->Test->TestTrue(TEXT("Original Editor play config restored"),
				                           ExportPlayConfig(*GetDefault<ULevelEditorPlaySettings>()) ==
				                               Proof->OriginalConfig);
				Proof->Test->TestEqual(TEXT("All intended independent PIE sessions completed"),
				                            Proof->CompletedSessions, Proof->bMissingData ? 1 : 2);
				Proof->Save();
			}
			return true;
		}

	private:
		TSharedRef<FG0PIEProof> Proof;
		bool bFinal = false;
		double StartedAt = 0;
	};

	FRequestPlaySessionParams MakeRequest(const FG0PIEProof& Proof)
	{
		ULevelEditorPlaySettings* Settings =
		    DuplicateObject<ULevelEditorPlaySettings>(Proof.Original, GetTransientPackage());
		Settings->SetPlayNetMode(EPlayNetMode::PIE_ListenServer);
		Settings->SetRunUnderOneProcess(true);
		Settings->SetPlayNumberOfClients(2);
		Settings->NewWindowWidth = 540;
		Settings->NewWindowHeight = 720;
		Settings->SetClientWindowSize(FIntPoint(540, 720));
		Settings->GameGetsMouseControl = false;
		Settings->AddToRoot(); // FStartPIEForAutomationCommand releases this on success/failure.
		FRequestPlaySessionParams Request;
		Request.SessionDestination = EPlaySessionDestinationType::InProcess;
		Request.WorldType = EPlaySessionWorldType::PlayInEditor;
		Request.EditorPlaySettings = Settings;
		Request.GlobalMapOverride = TEXT("/Game/TopDown/Lvl_TopDown");
		Request.GameModeOverride = ALDGameMode::StaticClass();
		Request.bAllowOnlineSubsystem = false;
		return Request;
	}

	class FDeferredStartG0APIE final : public IAutomationLatentCommand
	{
	public:
		explicit FDeferredStartG0APIE(TSharedRef<FG0PIEProof> InProof) : Proof(InProof) {}
		virtual bool Update() override
		{
			if (Proof->Test->HasAnyErrors())
			{
				Command.Reset();
				Proof->Record(TEXT("start-stopped"),
				                   TEXT("retained prior failure; no new PIE session; proceed to cleanup"));
				return true;
			}
			if (!Command)
			{
				StartedAt = FPlatformTime::Seconds();
				Proof->bTeardownRequested = false;
				Proof->bThirdJoinWindow = false;
				// The engine command subscribes to global PIE delegates in its constructor. Construct it only
				// when this queue entry is active, so the next session cannot observe this session's events.
				Command = MakeUnique<FStartPIEForAutomationCommand>(MakeRequest(*Proof));
			}
			// InternalUpdate is private to the automation framework. Directly driving the nested command
			// does not initialize its StartTime, so bound the whole start/readiness wait here instead.
			if (FPlatformTime::Seconds() - StartedAt > 60.0)
			{
				Proof->Test->AddError(
				    TEXT("Independent A PIE startup/readiness exceeded wrapper timeout of 60 seconds"));
				Command.Reset();
				return true;
			}
			if (!Command->Update())
			{
				return false;
			}
			Command.Reset(); // Remove its delegate subscriptions and release its rooted settings immediately.
			return true;
		}

	private:
		TSharedRef<FG0PIEProof> Proof;
		TUniquePtr<FStartPIEForAutomationCommand> Command;
		double StartedAt = 0;
	};

	class FEndG0APIE final : public IAutomationLatentCommand
	{
	public:
		explicit FEndG0APIE(TSharedRef<FG0PIEProof> InProof) : Proof(InProof) {}
		virtual bool Update() override
		{
			// Mark only this harness's explicit engine teardown, including cleanup after a failed assertion.
			Proof->bTeardownRequested = true;
			Proof->bThirdJoinWindow = false;
			return Command.Update();
		}

	private:
		TSharedRef<FG0PIEProof> Proof;
		FEndPlayMapCommand Command;
	};

	TSharedPtr<FG0PIEProof> PrepareProof(FAutomationTestBase& Test, bool bMissingData)
	{
		if (!GEditor || GEditor->PlayWorld || !FSlateApplication::IsInitialized())
		{
			Test.AddError(TEXT("G0 PIE requires an idle GPU Editor; existing sessions must not be replaced"));
			return nullptr;
		}
		const FString RulesPath = FPaths::ProjectContentDir() / TEXT("LD/Data/GameRules.json");
		if (IFileManager::Get().FileExists(*RulesPath) == bMissingData)
		{
			Test.AddError(bMissingData ? TEXT("MissingData requires a separately assembled fixture with GameRules.json omitted; no file is modified")
			                           : TEXT("Normal G0 replay requires its unchanged GameRules.json input"));
			return nullptr;
		}
		TSharedRef<FG0PIEProof> Proof = MakeShared<FG0PIEProof>();
		Proof->Test = &Test;
		Proof->bMissingData = bMissingData;
		Proof->Original = DuplicateObject<ULevelEditorPlaySettings>(GetMutableDefault<ULevelEditorPlaySettings>(),
		                                                            GetTransientPackage());
		Proof->Original->AddToRoot();
		Proof->OriginalConfig = ExportPlayConfig(*Proof->Original);
		FString RunId = TEXT("G0-A-PIE-") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
		FParse::Value(FCommandLine::Get(), TEXT("P0PIERun="), RunId);
		if (RunId.IsEmpty() || RunId.Contains(TEXT("/")) || RunId.Contains(TEXT("\\")) || RunId.Contains(TEXT("..")))
		{
			Test.AddError(TEXT("PIE proof RunId must be one new directory name"));
			return nullptr;
		}
		Proof->OutputDirectory = FPaths::ProjectSavedDir() / TEXT("P0Runs") / RunId;
		if (IFileManager::Get().DirectoryExists(*Proof->OutputDirectory))
		{
			Test.AddError(TEXT("PIE proof directory exists; existing evidence is preserved"));
			return nullptr;
		}
		if (!IFileManager::Get().MakeDirectory(*Proof->OutputDirectory, true))
		{
			Test.AddError(TEXT("Unable to create new PIE proof directory"));
			return nullptr;
		}
		Proof->ObserveNetworkLifetime();
		Proof->Record(TEXT("scope"), TEXT("provided test fixture over unchanged independent A Core/Data; native mode; no B RPC, BP generation, package or Android validation"));
		return Proof;
	}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDG0AActualPIETest, "LD.PIE.G0.A.MatchLifecycle",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLDG0AActualPIETest::RunTest(const FString& Parameters)
{
	const TSharedPtr<FG0PIEProof> Proof = PrepareProof(*this, false);
	if (!Proof)
	{
		return false;
	}
	ADD_LATENT_AUTOMATION_COMMAND(FDeferredStartG0APIE(Proof.ToSharedRef()));
	ADD_LATENT_AUTOMATION_COMMAND(FVerifyG0APIE(Proof.ToSharedRef(), 0));
	ADD_LATENT_AUTOMATION_COMMAND(FEndG0APIE(Proof.ToSharedRef()));
	ADD_LATENT_AUTOMATION_COMMAND(FFinishG0APIE(Proof.ToSharedRef(), false));
	ADD_LATENT_AUTOMATION_COMMAND(FDeferredStartG0APIE(Proof.ToSharedRef()));
	ADD_LATENT_AUTOMATION_COMMAND(FVerifyG0APIE(Proof.ToSharedRef(), 1));
	ADD_LATENT_AUTOMATION_COMMAND(FEndG0APIE(Proof.ToSharedRef()));
	ADD_LATENT_AUTOMATION_COMMAND(FFinishG0APIE(Proof.ToSharedRef(), true));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDG0AMissingDataPIETest, "LD.PIE.G0.A.MissingData",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLDG0AMissingDataPIETest::RunTest(const FString& Parameters)
{
	const TSharedPtr<FG0PIEProof> Proof = PrepareProof(*this, true);
	if (!Proof)
	{
		return false;
	}
	AddExpectedError(TEXT("Match aborted:.*GameRules.json"), EAutomationExpectedErrorFlags::Contains, 1);
	ADD_LATENT_AUTOMATION_COMMAND(FDeferredStartG0APIE(Proof.ToSharedRef()));
	ADD_LATENT_AUTOMATION_COMMAND(FVerifyG0APIE(Proof.ToSharedRef(), 0));
	ADD_LATENT_AUTOMATION_COMMAND(FEndG0APIE(Proof.ToSharedRef()));
	ADD_LATENT_AUTOMATION_COMMAND(FFinishG0APIE(Proof.ToSharedRef(), true));
	return true;
}

#endif
