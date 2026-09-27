// A distinct filter keeps actual GPU PIE out of the LD.P0 NullRHI suite.
#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Battle/LDCombatService.h"
#include "Core/LDGameMode.h"
#include "Core/LDGameState.h"
#include "Core/LDPlayerController.h"
#include "Core/LDPlayerState.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
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

	struct FPIEProof
	{
		FAutomationTestBase* Test = nullptr;
		ULevelEditorPlaySettings* Original = nullptr;
		FString OriginalConfig;
		FString OutputDirectory;
		TArray<TWeakObjectPtr<UWorld>> Worlds;
		TArray<TSharedPtr<FJsonValue>> Observations;
		bool bRestored = false;

		~FPIEProof()
		{
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
		bool Capture(ALDPlayerController& Player, const FString& Name)
		{
			ULocalPlayer* LocalPlayer = Player.GetLocalPlayer();
			UGameViewportClient* Viewport = LocalPlayer ? LocalPlayer->ViewportClient.Get() : nullptr;
			const TSharedPtr<SWindow> Window = Viewport ? Viewport->GetWindow() : nullptr;
			if (!Window.IsValid())
			{
				Test->AddError(TEXT("Actual PIE viewport window is missing"));
				return false;
			}
			TArray<FColor> Pixels;
			FIntVector Size;
			if (!FSlateApplication::Get().TakeScreenshot(Window.ToSharedRef(), Pixels, Size) || Pixels.IsEmpty())
			{
				Test->AddError(TEXT("Actual PIE Slate window capture failed"));
				return false;
			}
			TArray64<uint8> PNG;
			FImageUtils::PNGCompressImageArray(Size.X, Size.Y, Pixels, PNG);
			const FString Path = OutputDirectory / (Name + TEXT(".png"));
			const bool bSaved = FFileHelper::SaveArrayToFile(PNG, *Path);
			Test->TestTrue(TEXT("PIE screenshot saved"), bSaved);
			Record(TEXT("capture"), FString::Printf(TEXT("%s %dx%d"), *Path, Size.X, Size.Y));
			return bSaved;
		}
		void Save()
		{
			TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
			Root->SetStringField(TEXT("kind"), TEXT("actual-editor-PIE-listen-and-client"));
			Root->SetStringField(TEXT("result"), Test->HasAnyErrors() ? TEXT("Fail") : TEXT("Pass"));
			Root->SetNumberField(TEXT("processId"), FPlatformProcess::GetCurrentProcessId());
			Root->SetBoolField(TEXT("settingsRestored"), bRestored);
			Root->SetArrayField(TEXT("observations"), Observations);
			FString Json;
			FJsonSerializer::Serialize(Root, TJsonWriterFactory<>::Create(&Json));
			FFileHelper::SaveStringToFile(Json, *(OutputDirectory / TEXT("pie-proof.json")));
		}
	};

	class FVerifyP0PIE final : public IAutomationLatentCommand
	{
	public:
		explicit FVerifyP0PIE(TSharedRef<FPIEProof> InProof) : Proof(InProof) {}
		virtual bool Update() override
		{
			if (StartedAt == 0)
			{
				StartedAt = FPlatformTime::Seconds();
			}
			if (FPlatformTime::Seconds() - StartedAt > 45)
			{
				Proof->Test->AddError(FString::Printf(TEXT("PIE verification timed out at stage%d"), Stage));
				return true;
			}
			ALDPlayerController* Host = nullptr;
			ALDPlayerController* Client = nullptr;
			ALDGameMode* Mode = nullptr;
			int32 PIEWorldCount = 0;
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				UWorld* World = Context.World();
				if (Context.WorldType != EWorldType::PIE || !World)
				{
					continue;
				}
				++PIEWorldCount;
				for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
				{
					ALDPlayerController* PC = Cast<ALDPlayerController>(It->Get());
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
			if (!Host || !Client || !Mode || PIEWorldCount != 2)
			{
				return false;
			}
			ALDGameState* HostState = Host->GetWorld()->GetGameState<ALDGameState>();
			ALDGameState* ClientState = Client->GetWorld()->GetGameState<ALDGameState>();
			if (!HostState || !ClientState)
			{
				return false;
			}
			if (Stage == 0)
			{
				if (HostState->GetPhase() != ELDMatchPhase::Preparing ||
				    ClientState->GetPhase() != ELDMatchPhase::Preparing || !Host->IsGameplaySnapshotReady() ||
				    !Client->IsGameplaySnapshotReady() || !Host->IsLocalBoardReady() || !Client->IsLocalBoardReady())
				{
					return false;
				}
				Proof->Test->TestTrue(TEXT("Actual distinct PIE Worlds"), Host->GetWorld() != Client->GetWorld());
				Proof->Test->TestEqual(TEXT("Actual PIE server/client share match"),
				                            HostState->GetBattleSnapshot().MatchId,
				                            ClientState->GetBattleSnapshot().MatchId);
				Proof->Test->TestEqual(TEXT("Owned host board index"), Host->GetLocalParticipantIndex(), 0);
				Proof->Test->TestEqual(TEXT("Owned remote board index"), Client->GetLocalParticipantIndex(), 1);
				Proof->Worlds = {Host->GetWorld(), Client->GetWorld()};
				Proof->Record(TEXT("worlds"), FString::Printf(TEXT("host=%s type=PIE net=ListenServer localPlayer=0; client=%s type=PIE net=Client localPlayer=1; match=%s"),
				                                  *Host->GetWorld()->GetPathName(), *Client->GetWorld()->GetPathName(),
				                                  *HostState->GetBattleSnapshot().MatchId.ToString()));
				Proof->Test->TestTrue(TEXT("Host actual preparing summon intent"), Host->RequestSummon());
				Proof->Test->TestTrue(TEXT("Client actual preparing RPC summon intent"), Client->RequestSummon());
				Stage = 1;
				return false;
			}
			if (Stage == 1)
			{
				if (Host->HasPendingCommand() || Client->HasPendingCommand() ||
				    Host->GetBoardSnapshot().Population != 1 || Client->GetBoardSnapshot().Population != 1)
				{
					return false;
				}
				Proof->Test->TestEqual(TEXT("Host actual purchase response"), Host->GetLastResult().ResultCode,
				                            ELDCommandResultCode::Success);
				Proof->Test->TestEqual(TEXT("Client actual RPC response"), Client->GetLastResult().ResultCode,
				                            ELDCommandResultCode::Success);
				Proof->Test->TestEqual(TEXT("Host first purchase gold80"), Host->GetEconomySnapshot().Gold, 80);
				Proof->Test->TestEqual(TEXT("Remote first purchase gold80"), Client->GetEconomySnapshot().Gold, 80);
				Proof->Record(TEXT("owned-commands"),
				                   TEXT("both population1 gold80; remote reliable RPC completed; pending cleared"));
				Stage = 2;
				return false;
			}
			if (Stage == 2)
			{
				if (HostState->GetPhase() != ELDMatchPhase::Running ||
				    ClientState->GetPhase() != ELDMatchPhase::Running ||
				    HostState->GetBattleSnapshot().WaveIndex != 1 || ClientState->GetBattleSnapshot().WaveIndex != 1 ||
				    ClientState->GetBattleSnapshot().ActiveEnemyCount < 2)
				{
					return false;
				}
				Proof->Capture(*Host, TEXT("host-running"));
				Proof->Capture(*Client, TEXT("client-running"));
				Proof->Record(TEXT("running"), FString::Printf(TEXT("host/client wave1, counts%d/%d, server time%.3f"),
				                                                    HostState->GetBattleSnapshot().ActiveEnemyCount,
				                                                    ClientState->GetBattleSnapshot().ActiveEnemyCount,
				                                                    HostState->GetServerWorldTimeSeconds()));
				Proof->Test->AddExpectedError(TEXT("Match aborted: PIE automation requested service shutdown"),
				                                   EAutomationExpectedErrorFlags::Contains, 1);
				Mode->AbortMatch(TEXT("PIE automation requested service shutdown"));
				Stage = 3;
				return false;
			}
			if (HostState->GetPhase() != ELDMatchPhase::Aborted || ClientState->GetPhase() != ELDMatchPhase::Aborted)
			{
				return false;
			}
			Proof->Test->TestFalse(TEXT("Actual PIE terminal closes server admission"), Mode->CanAcceptCommands());
			Proof->Test->TestEqual(TEXT("Actual PIE terminal clears combat units"),
			                            Mode->GetCombatService()->GetRegisteredUnitCount(), 0);
			Proof->Test->TestEqual(TEXT("Client replicated result matches"), ClientState->GetBattleSnapshot().Result,
			                            ELDMatchResult::Aborted);
			Proof->Record(
			    TEXT("terminal"),
			         TEXT("server/client Aborted; server command admission closed and combat registrations cleared"));
			return true;
		}

	private:
		TSharedRef<FPIEProof> Proof;
		double StartedAt = 0;
		int32 Stage = 0;
	};

	class FRestoreP0PIE final : public IAutomationLatentCommand
	{
	public:
		explicit FRestoreP0PIE(TSharedRef<FPIEProof> InProof) : Proof(InProof) {}
		virtual bool Update() override
		{
			if (StartedAt == 0)
			{
				StartedAt = FPlatformTime::Seconds();
			}
			bool bPIERemains = false;
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				bPIERemains |= Context.WorldType == EWorldType::PIE;
			}
			if (bPIERemains && FPlatformTime::Seconds() - StartedAt < 15)
			{
				return false;
			}
			Proof->Test->TestFalse(TEXT("FEndPlayMapCommand removed actual PIE worlds"), bPIERemains);
			Proof->Restore();
			Proof->Test->TestTrue(TEXT("Original Editor play config restored"),
			                           ExportPlayConfig(*GetDefault<ULevelEditorPlaySettings>()) ==
			                               Proof->OriginalConfig);
			Proof->Record(TEXT("cleanup"), FString::Printf(TEXT("PIE worlds remaining=%d; original config restored=%d"),
			                                                    bPIERemains, Proof->bRestored));
			Proof->Save();
			return true;
		}

	private:
		TSharedRef<FPIEProof> Proof;
		double StartedAt = 0;
	};
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDP0ActualPIETest, "LD.PIE.P0.Session",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLDP0ActualPIETest::RunTest(const FString& Parameters)
{
	if (!GEditor || GEditor->PlayWorld || !FSlateApplication::IsInitialized())
	{
		AddError(TEXT("PIE test requires an idle GPU Editor with Slate; no existing session may be replaced"));
		return false;
	}
	TSharedRef<FPIEProof> Proof = MakeShared<FPIEProof>();
	Proof->Test = this;
	Proof->Original =
	    DuplicateObject<ULevelEditorPlaySettings>(GetMutableDefault<ULevelEditorPlaySettings>(), GetTransientPackage());
	Proof->Original->AddToRoot();
	Proof->OriginalConfig = ExportPlayConfig(*Proof->Original);
	FString RunId = TEXT("G3-PIE-") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
	FParse::Value(FCommandLine::Get(), TEXT("P0PIERun="), RunId);
	if (RunId.Contains(TEXT("/")) || RunId.Contains(TEXT("\\")) || RunId.Contains(TEXT("..")))
	{
		AddError(TEXT("PIE evidence RunId must be one new directory name"));
		return false;
	}
	Proof->OutputDirectory = FPaths::ProjectSavedDir() / TEXT("P0Runs") / RunId;
	if (IFileManager::Get().DirectoryExists(*Proof->OutputDirectory))
	{
		AddError(TEXT("PIE evidence directory exists; previous evidence will not be overwritten"));
		return false;
	}
	IFileManager::Get().MakeDirectory(*Proof->OutputDirectory, true);
	ULevelEditorPlaySettings* Settings =
	    DuplicateObject<ULevelEditorPlaySettings>(Proof->Original, GetTransientPackage());
	Settings->SetPlayNetMode(EPlayNetMode::PIE_ListenServer);
	Settings->SetRunUnderOneProcess(true);
	Settings->SetPlayNumberOfClients(2);
	Settings->NewWindowWidth = 540;
	Settings->NewWindowHeight = 1170;
	Settings->SetClientWindowSize(FIntPoint(540, 1170));
	Settings->GameGetsMouseControl = false;
	Settings->AddToRoot(); // FStartPIEForAutomationCommand owns removal on every start outcome.
	FRequestPlaySessionParams Request;
	Request.SessionDestination = EPlaySessionDestinationType::InProcess;
	Request.WorldType = EPlaySessionWorldType::PlayInEditor;
	Request.EditorPlaySettings = Settings;
	Request.GlobalMapOverride = TEXT("/Game/LD/Maps/L_P0");
	Request.GameModeOverride = ALDGameMode::StaticClass();
	Request.bAllowOnlineSubsystem = false;
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIEForAutomationCommand(Request));
	ADD_LATENT_AUTOMATION_COMMAND(FVerifyP0PIE(Proof));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(FRestoreP0PIE(Proof));
	return true;
}

#endif
