// A distinct filter keeps actual GPU PIE out of the LD.P0 NullRHI suite.
#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Battle/LDCombatService.h"
#include "Blueprint/WidgetTree.h"
#include "Board/LDBoardManager.h"
#include "Components/Button.h"
#include "Components/InputComponent.h"
#include "Components/TextBlock.h"
#include "Core/LDGameMode.h"
#include "Core/LDGameState.h"
#include "Core/LDPlayerController.h"
#include "Core/LDPlayerState.h"
#include "Economy/LDEconomyService.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerInput.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "ImageUtils.h"
#include "InputKeyEventArgs.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Network/LDCommandProcessor.h"
#include "Serialization/JsonSerializer.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Tests/AutomationEditorCommon.h"
#include "UI/LDBattleStatusWidget.h"
#include "UI/LDResultWidget.h"
#include "UObject/GarbageCollection.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"
#include "Widgets/SWindow.h"

namespace
{
	template <typename WidgetType> int32 FindVisibleWidgets(ALDPlayerController& Player, WidgetType*& OutWidget)
	{
		int32 Count = 0;
		OutWidget = nullptr;
		for (TObjectIterator<WidgetType> It; It; ++It)
		{
			if (It->GetOwningPlayer() == &Player && It->GetWorld() == Player.GetWorld() && It->IsInViewport())
			{
				OutWidget = *It;
				++Count;
			}
		}
		return Count;
	}

	template <typename SnapshotType> bool SameSnapshot(const SnapshotType& Left, const SnapshotType& Right)
	{
		// Compare every reflected field, including IDs, revision, time, array membership and balances.
		return SnapshotType::StaticStruct()->CompareScriptStruct(&Left, &Right, 0);
	}

	FString WidgetText(const UUserWidget& Widget, FName Name)
	{
		const UTextBlock* Text = Widget.WidgetTree ? Cast<UTextBlock>(Widget.WidgetTree->FindWidget(Name)) : nullptr;
		return Text ? Text->GetText().ToString() : FString();
	}

	struct FTerminalViewBaseline
	{
		FLDBoardSnapshot ClientBoard;
		FLDEconomySnapshot ClientEconomy;
		FLDBattleSnapshot Battle;
		FLDBoardSnapshot ServerBoard;
		FLDEconomySnapshot ServerEconomy;
		FLDCommandResult LastResult;
		int32 RandomState = 0;
		int32 CachedResults = 0;
		int32 SelectedCell = INDEX_NONE;
		FVector2D TouchStart = FVector2D::ZeroVector;
		FVector2D TouchEnd = FVector2D::ZeroVector;
		TStrongObjectPtr<ULDResultWidget> RetiredResult;
		TStrongObjectPtr<ULDBattleStatusWidget> RetiredStatus;
		TWeakObjectPtr<ULDResultWidget> OldResult;
		TWeakObjectPtr<ULDBattleStatusWidget> OldStatus;
		TWeakObjectPtr<ULDResultWidget> NewResult;
		TWeakObjectPtr<ULDBattleStatusWidget> NewStatus;
	};

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
		TSharedRef<FJsonObject> TerminalUI = MakeShared<FJsonObject>();
		TWeakObjectPtr<UWorld> ObservedServerWorld;
		FDelegateHandle EventObserver;
		int32 TerminalServerRequests = 0;
		bool bEventObserverRestored = true;
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
			if (!bEventObserverRestored && AActor::ProcessEventDelegate.GetHandle() == EventObserver)
			{
				AActor::ProcessEventDelegate.Unbind();
				bEventObserverRestored = true;
			}
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
		bool WatchTerminalRPCs(UWorld& ServerWorld)
		{
			if (AActor::ProcessEventDelegate.IsBound())
			{
				Test->AddError(TEXT("PIE terminal observation requires an unbound Actor event observer"));
				return false;
			}
			ObservedServerWorld = &ServerWorld;
			AActor::ProcessEventDelegate.BindLambda(
			    [this](AActor* Actor, UFunction* Function, void*)
			    {
				    if (Actor && Actor->GetWorld() == ObservedServerWorld.Get() && Actor->IsA<ALDPlayerController>() &&
				        Function && Function->GetFName() == TEXT("ServerRequestCommand"))
				    {
					    ++TerminalServerRequests;
				    }
				    return false; // Observe actual dispatch; never consume or alter an RPC.
			    });
			EventObserver = AActor::ProcessEventDelegate.GetHandle();
			bEventObserverRestored = false;
			return true;
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
			Root->SetBoolField(TEXT("eventObserverRestored"), bEventObserverRestored);
			Root->SetObjectField(TEXT("terminalUI"), TerminalUI);
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
				if (RunningObservedAt == 0)
				{
					RunningObservedAt = FPlatformTime::Seconds();
					return false;
				}
				if (FPlatformTime::Seconds() - RunningObservedAt < .5)
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
			ALDPlayerController* Players[2] = {Host, Client};
			if (Stage == 3)
			{
				ULDResultWidget* Results[2] = {};
				ULDBattleStatusWidget* Statuses[2] = {};
				for (int32 Index = 0; Index < 2; ++Index)
				{
					const int32 ResultCount = FindVisibleWidgets(*Players[Index], Results[Index]);
					const int32 StatusCount = FindVisibleWidgets(*Players[Index], Statuses[Index]);
					if (ResultCount == 0 || StatusCount == 0)
					{
						return false; // Wait for the real Controller tick after replicated Aborted.
					}
					Proof->Test->TestEqual(TEXT("One initial Result widget per owner"), ResultCount, 1);
					Proof->Test->TestEqual(TEXT("One initial Status widget per owner"), StatusCount, 1);
				}
				Proof->Test->TestFalse(TEXT("Actual PIE terminal closes server admission"), Mode->CanAcceptCommands());
				Proof->Test->TestFalse(TEXT("Actual PIE terminal stops logic timer"), Mode->IsLogicTimerActive());
				Proof->Test->TestEqual(TEXT("Actual PIE terminal clears combat units"),
				                            Mode->GetCombatService()->GetRegisteredUnitCount(), 0);
				Proof->Test->TestEqual(TEXT("Client replicated result matches"),
				                            ClientState->GetBattleSnapshot().Result, ELDMatchResult::Aborted);
				if (!Proof->WatchTerminalRPCs(*Host->GetWorld()))
				{
					return true;
				}
				Proof->TerminalUI->SetStringField(
				    TEXT("scope"),
				         TEXT("Actual GPU PIE; Aborted Result/Status removal and normal Controller-tick recreation; "
				         "Engine InputKey/InputTouch and public intent APIs; no physical input or Entry travel"));
				Proof->TerminalUI->SetNumberField(TEXT("expectedVisibleResultsPerOwner"), 1);
				Proof->TerminalUI->SetNumberField(TEXT("expectedVisibleStatusesPerOwner"), 1);
				Proof->TerminalUI->SetNumberField(TEXT("expectedReturnOwnerBindings"), 1);
				Proof->TerminalUI->SetNumberField(TEXT("expectedNewServerRequests"), 0);
				Proof->TerminalUI->SetStringField(TEXT("expectedStateChange"),
				                                       TEXT("none: board/economy/battle/cache/RNG"));
				SourceMapAsset = FindObject<UWorld>(nullptr, TEXT("/Game/LD/Maps/L_P0.L_P0"));
				Proof->Test->TestTrue(TEXT("PIE source map is a standalone editor asset"),
				                           SourceMapAsset.IsValid() && SourceMapAsset->HasAnyFlags(RF_Standalone) &&
				                               SourceMapAsset->WorldType != EWorldType::PIE);
				Proof->TerminalUI->SetStringField(TEXT("gcKeepPolicy"),
				                                       TEXT("GARBAGE_COLLECTION_KEEPFLAGS: Editor RF_Standalone"));
				for (int32 Index = 0; Index < 2; ++Index)
				{
					ALDPlayerController& Player = *Players[Index];
					FTerminalViewBaseline& View = TerminalViews[Index];
					View.ClientBoard = Player.GetBoardSnapshot();
					View.ClientEconomy = Player.GetEconomySnapshot();
					View.Battle = Player.GetWorld()->GetGameState<ALDGameState>()->GetBattleSnapshot();
					View.ServerBoard = Mode->GetBoardManager()->GetSnapshot(Index);
					View.ServerEconomy = Mode->GetEconomyService()->GetSnapshot(Index);
					View.RandomState = Mode->GetEconomyService()->GetRandomState(Index);
					View.CachedResults = Mode->GetCommandProcessor()->GetCachedResultCount(Index);
					View.LastResult = Player.GetLastResult();
					View.SelectedCell = Player.GetSelectedCellId();
					Proof->Test->TestTrue(TEXT("Real Controller input objects exist"),
					                           Player.InputComponent && Player.PlayerInput);
					View.RetiredResult.Reset(Results[Index]);
					View.RetiredStatus.Reset(Statuses[Index]);
					View.OldResult = Results[Index];
					View.OldStatus = Statuses[Index];
					Proof->Test->TestFalse(TEXT("Retired widgets are not standalone editor assets or rooted"),
					                            Results[Index]->HasAnyFlags(RF_Standalone) || Results[Index]->IsRooted() ||
					                                Statuses[Index]->HasAnyFlags(RF_Standalone) || Statuses[Index]->IsRooted());
					Proof->Test->TestFalse(TEXT("Terminal begins without pending command"), Player.HasPendingCommand());
					Proof->Capture(Player, Index == 0 ? TEXT("host-terminal-before") : TEXT("client-terminal-before"));
					Results[Index]->RemoveFromParent();
					Statuses[Index]->RemoveFromParent();
				}
				Proof->Record(TEXT("terminal-ui-removed"),
				                   TEXT("Removed host/client Result and Status only; no private update, widget factory or state setter called"));
				TerminalStepAt = FPlatformTime::Seconds();
				Stage = 4;
				return false;
			}
			if (FPlatformTime::Seconds() - TerminalStepAt < .25)
			{
				return false;
			}
			if (Stage == 4)
			{
				for (int32 Index = 0; Index < 2; ++Index)
				{
					ULDResultWidget* Result = nullptr;
					ULDBattleStatusWidget* Status = nullptr;
					if (FindVisibleWidgets(*Players[Index], Result) == 0 ||
					    FindVisibleWidgets(*Players[Index], Status) == 0)
					{
						return false;
					}
				}
				TArray<TSharedPtr<FJsonValue>> Views;
				for (int32 Index = 0; Index < 2; ++Index)
				{
					ALDPlayerController& Player = *Players[Index];
					FTerminalViewBaseline& View = TerminalViews[Index];
					ULDResultWidget* Result = nullptr;
					ULDBattleStatusWidget* Status = nullptr;
					const int32 ResultCount = FindVisibleWidgets(Player, Result);
					const int32 StatusCount = FindVisibleWidgets(Player, Status);
					Proof->Test->TestEqual(TEXT("Exactly one replacement Result"), ResultCount, 1);
					Proof->Test->TestEqual(TEXT("Exactly one replacement Status"), StatusCount, 1);
					Proof->Test->TestTrue(TEXT("Result is a new instance"), Result != View.OldResult.Get());
					Proof->Test->TestTrue(TEXT("Status is a new instance"), Status != View.OldStatus.Get());
					Proof->Test->TestTrue(TEXT("Replacement Result and Status are visible"),
					                           Result->IsVisible() && Status->IsVisible());
					Proof->Test->TestFalse(TEXT("Old Result no longer in viewport"),
					                            View.RetiredResult->IsInViewport());
					Proof->Test->TestFalse(TEXT("Old Status no longer in viewport"),
					                            View.RetiredStatus->IsInViewport());
					Proof->Test->TestFalse(TEXT("Old Result return multicast unbound"),
					                            View.RetiredResult->OnReturnRequested.IsBound());
					const UButton* OldButton = View.RetiredResult->WidgetTree
					    ? Cast<UButton>(View.RetiredResult->WidgetTree->FindWidget(TEXT("ReturnButton")))
					    : nullptr;
					Proof->Test->TestTrue(TEXT("Old return button subscription removed"),
					                           OldButton && !OldButton->OnClicked.IsBound());
					// Count bindings on a COPY: observation must not unsubscribe the production widget.
					FOnLDReturnRequested ReturnBindings = Result->OnReturnRequested;
					const int32 OwnerBindings = ReturnBindings.RemoveAll(&Player);
					Proof->Test->TestEqual(TEXT("Replacement Result has exactly one owner return binding"),
					                            OwnerBindings, 1);
					Proof->Test->TestFalse(TEXT("Replacement Result has no extra return listeners"),
					                            ReturnBindings.IsBound());
					Proof->Test->TestTrue(TEXT("Original replacement binding left intact"),
					                           Result->OnReturnRequested.IsBoundToObject(&Player));
					FBox2D ReturnRect;
					Proof->Test->TestTrue(TEXT("Replacement result has a visible return action"),
					                           Player.GetReturnButtonScreenRect(ReturnRect));
					const FString ResultText = WidgetText(*Result, TEXT("ResultText"));
					const FString StatusText = WidgetText(*Status, TEXT("WaveStatus"));
					Proof->Test->TestTrue(TEXT("Replacement Result displays aborted title and wave"),
					                           ResultText.Contains(TEXT("매치 종료")) &&
					                                               ResultText.Contains(TEXT("WAVE 1 / 10")));
					Proof->Test->TestTrue(TEXT("Replacement Status displays stopped clock and phase"),
					                           StatusText.Contains(TEXT("00:00")) &&
					                                               StatusText.Contains(TEXT("전투 종료")));
					View.NewResult = Result;
					View.NewStatus = Status;
					TSharedPtr<FJsonObject> Item = MakeShared<FJsonObject>();
					Item->SetStringField(TEXT("role"), Index == 0 ? TEXT("host") : TEXT("client"));
					Item->SetNumberField(TEXT("visibleResults"), ResultCount);
					Item->SetNumberField(TEXT("visibleStatuses"), StatusCount);
					Item->SetNumberField(TEXT("ownerReturnBindings"), OwnerBindings);
					Item->SetStringField(TEXT("oldResult"), View.RetiredResult->GetName());
					Item->SetStringField(TEXT("newResult"), Result->GetName());
					Item->SetStringField(TEXT("oldStatus"), View.RetiredStatus->GetName());
					Item->SetStringField(TEXT("newStatus"), Status->GetName());
					Item->SetStringField(TEXT("resultText"), ResultText);
					Item->SetStringField(TEXT("statusText"), StatusText);
					Item->SetBoolField(TEXT("oldReturnUnbound"), !View.RetiredResult->OnReturnRequested.IsBound());
					Views.Add(MakeShared<FJsonValueObject>(Item));
					Proof->Capture(Player,
					               Index == 0 ? TEXT("host-terminal-recreated") : TEXT("client-terminal-recreated"));
					Proof->Test->TestFalse(TEXT("Terminal summon intent rejected locally"), Player.RequestSummon());
					Proof->Test->TestFalse(TEXT("Terminal merge intent rejected locally"),
					                            Player.RequestMergeSelection());
					Proof->Test->TestFalse(TEXT("Terminal sale intent rejected locally"),
					                            Player.RequestSellSelection());
					const uint64 UnitId = View.ClientBoard.Units.IsEmpty() ? 0 : View.ClientBoard.Units[0].InstanceId;
					Proof->Test->TestTrue(TEXT("Input fixture retains a real unit identity"), UnitId != 0);
					Proof->Test->TestFalse(TEXT("Terminal move intent rejected locally"),
					                            Player.RequestMove(UnitId, 17));
					Proof->Test->TestTrue(TEXT("Terminal touch endpoints project onto own board"),
					                           Player.ProjectCellToScreen(0, View.TouchStart) &&
					                               Player.ProjectCellToScreen(17, View.TouchEnd));
					Proof->Test->TestFalse(TEXT("Terminal board selection rejected"),
					                            Player.InputScreenPosition(View.TouchStart));
					SendKeys(Player, IE_Pressed);
					Proof->Test->TestTrue(TEXT("Engine accepted terminal touch begin"),
					                           Player.InputTouch(TestFinger(), ETouchType::Began, View.TouchStart, 1,
					                                             FPlatformTime::Cycles64()));
				}
				Proof->TerminalUI->SetArrayField(TEXT("views"), Views);
				Proof->Record(TEXT("terminal-ui-recreated"),
				                   TEXT("Exactly one new Result/Status per owner; old return/button unbound; new owner binding1; Engine S/M/X and touch began"));
				TerminalStepAt = FPlatformTime::Seconds();
				Stage = 5;
				return false;
			}
			if (Stage == 5 || Stage == 6)
			{
				for (int32 Index = 0; Index < 2; ++Index)
				{
					if (Stage == 5)
					{
						// Observe processed engine key state before releasing; a missing input path must not pass
						// vacuously.
						for (const FKey& Key : {EKeys::S, EKeys::M, EKeys::X})
						{
							Proof->Test->TestTrue(TEXT("Terminal test key reached PlayerInput pressed state"),
							                           Players[Index]->IsInputKeyDown(Key));
						}
						SendKeys(*Players[Index], IE_Released);
					}
					Proof->Test->TestTrue(TEXT("Engine accepted terminal touch move/end"),
					    Players[Index]->InputTouch(TestFinger(), Stage == 5 ? ETouchType::Moved : ETouchType::Ended,
					                               TerminalViews[Index].TouchEnd, Stage == 5 ? 1 : 0,
					                               FPlatformTime::Cycles64()));
				}
				TerminalStepAt = FPlatformTime::Seconds();
				++Stage;
				return false;
			}
			if (Stage == 7)
			{
				VerifyTerminalUnchanged(*Mode, Players);
				for (FTerminalViewBaseline& View : TerminalViews)
				{
					View.RetiredResult.Reset();
					View.RetiredStatus.Reset();
				}
				// Match InitializeForPlayInEditor/EndPlayMap: retain standalone source map assets in Editor.
				// RF_NoFlags destroys the initialized source WorldPartition subsystem while PIE still owns its map.
				// The retired widgets above are neither standalone nor rooted and must still be collected.
				CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
				TerminalStepAt = FPlatformTime::Seconds();
				Stage = 8;
				return false;
			}
			VerifyTerminalUnchanged(*Mode, Players);
			bool bOldWidgetsCollected = true;
			for (const FTerminalViewBaseline& View : TerminalViews)
			{
				bOldWidgetsCollected &= View.OldResult.IsStale(false, true) && View.OldStatus.IsStale(false, true);
			}
			Proof->Test->TestTrue(TEXT("All four retired Result/Status widgets garbage collected"),
			                           bOldWidgetsCollected);
			Proof->TerminalUI->SetBoolField(TEXT("allFourOldWidgetsCollected"), bOldWidgetsCollected);
			const bool bSourceMapRetained = SourceMapAsset.IsValid() && SourceMapAsset->HasAnyFlags(RF_Standalone);
			Proof->Test->TestTrue(TEXT("Standalone PIE source map survives editor GC"), bSourceMapRetained);
			Proof->TerminalUI->SetBoolField(TEXT("sourceMapRetainedAfterGC"), bSourceMapRetained);
			Proof->TerminalUI->SetNumberField(TEXT("actualTerminalServerRequests"), Proof->TerminalServerRequests);
			Proof->TerminalUI->SetBoolField(TEXT("logicTimerActive"), Mode->IsLogicTimerActive());
			Proof->TerminalUI->SetBoolField(TEXT("passedBeforeEditorShutdown"), !Proof->Test->HasAnyErrors());
			Proof->Record(TEXT("terminal-ui-input-and-gc"),
			                   FString::Printf(TEXT("Engine S/M/X and touch begin/move/end; intent APIs; server RPCs=%d; old widgets collected=%d; state/cache/RNG unchanged"),
			                                        Proof->TerminalServerRequests, bOldWidgetsCollected));
			return true;
		}

	private:
		static FTouchId TestFinger()
		{
			return FTouchId(FInputDeviceId::CreateFromInternalId(0), ETouchIndex::Touch1);
		}
		static void SendKeys(ALDPlayerController& Player, EInputEvent Event)
		{
			const ULocalPlayer* Local = Player.GetLocalPlayer();
			UGameViewportClient* Viewport = Local ? Local->ViewportClient.Get() : nullptr;
			for (const FKey& Key : {EKeys::S, EKeys::M, EKeys::X})
			{
				Player.InputKey(FInputKeyEventArgs(Viewport ? Viewport->Viewport : nullptr,
				                                   FInputDeviceId::CreateFromInternalId(0), Key, Event,
				                                   FPlatformTime::Cycles64()));
			}
		}
		void VerifyTerminalUnchanged(ALDGameMode& Mode, ALDPlayerController* const Players[2])
		{
			Proof->Test->TestFalse(TEXT("Terminal logic timer stays inactive through UI recreation/input"),
			                            Mode.IsLogicTimerActive());
			Proof->Test->TestFalse(TEXT("Terminal command clock before delegate stays unbound"),
			                            Mode.GetCommandProcessor()->BeforeExternalCommand.IsBound());
			Proof->Test->TestFalse(TEXT("Terminal command clock after delegate stays unbound"),
			                            Mode.GetCommandProcessor()->AfterExternalCommandClock.IsBound());
			Proof->Test->TestEqual(TEXT("Terminal Engine input generated no actual server RPC"),
			                            Proof->TerminalServerRequests, 0);
			TArray<TSharedPtr<FJsonValue>> States;
			for (int32 Index = 0; Index < 2; ++Index)
			{
				ALDPlayerController& Player = *Players[Index];
				const FTerminalViewBaseline& View = TerminalViews[Index];
				ULDResultWidget* Result = nullptr;
				ULDBattleStatusWidget* Status = nullptr;
				Proof->Test->TestEqual(TEXT("Result remains unique after later input ticks"),
				                            FindVisibleWidgets(Player, Result), 1);
				Proof->Test->TestEqual(TEXT("Status remains unique after later input ticks"),
				                            FindVisibleWidgets(Player, Status), 1);
				Proof->Test->TestTrue(TEXT("Replacement identity stable across later ticks"),
				                           Result == View.NewResult.Get() && Status == View.NewStatus.Get());
				if (Result)
				{
					FOnLDReturnRequested ReturnBindings = Result->OnReturnRequested;
					Proof->Test->TestEqual(TEXT("Later ticks keep one owner return subscription"),
					                            ReturnBindings.RemoveAll(&Player), 1);
					Proof->Test->TestFalse(TEXT("Later ticks add no extra return listeners"), ReturnBindings.IsBound());
				}
				Proof->Test->TestFalse(TEXT("Terminal gameplay remains closed"), Player.CanUseGameplayActions());
				Proof->Test->TestFalse(TEXT("Terminal input leaves no pending command"), Player.HasPendingCommand());
				Proof->Test->TestTrue(TEXT("Owner board unchanged in all fields"),
				                           SameSnapshot(View.ClientBoard, Player.GetBoardSnapshot()));
				Proof->Test->TestTrue(TEXT("Owner economy unchanged in all fields"),
				                           SameSnapshot(View.ClientEconomy, Player.GetEconomySnapshot()));
				Proof->Test->TestTrue(TEXT("Owner last response unchanged in all fields"),
				                           SameSnapshot(View.LastResult, Player.GetLastResult()));
				Proof->Test->TestTrue(TEXT("GameState unchanged in all fields"),
				    SameSnapshot(View.Battle, Player.GetWorld()->GetGameState<ALDGameState>()->GetBattleSnapshot()));
				Proof->Test->TestTrue(TEXT("Authoritative board unchanged in all fields"),
				                           SameSnapshot(View.ServerBoard, Mode.GetBoardManager()->GetSnapshot(Index)));
				Proof->Test->TestTrue(
				    TEXT("Authoritative economy unchanged in all fields"),
				         SameSnapshot(View.ServerEconomy, Mode.GetEconomyService()->GetSnapshot(Index)));
				Proof->Test->TestEqual(TEXT("Authoritative RNG unchanged"),
				                            Mode.GetEconomyService()->GetRandomState(Index), View.RandomState);
				Proof->Test->TestEqual(TEXT("No new command cache entry"),
				                            Mode.GetCommandProcessor()->GetCachedResultCount(Index),
				                            View.CachedResults);
				Proof->Test->TestEqual(TEXT("Terminal touch/drag does not change selection"),
				                            Player.GetSelectedCellId(), View.SelectedCell);
				for (const FKey& Key : {EKeys::S, EKeys::M, EKeys::X})
				{
					Proof->Test->TestFalse(TEXT("Terminal test key released in engine input"),
					                            Player.IsInputKeyDown(Key));
					int32 OwnerKeyBindings = 0;
					if (Player.InputComponent)
					{
						for (const FInputKeyBinding& Binding : Player.InputComponent->KeyBindings)
						{
							OwnerKeyBindings += Binding.Chord.Key == Key && Binding.KeyEvent == IE_Pressed &&
							                            Binding.KeyDelegate.IsBoundToObject(&Player)
							                        ? 1
							                        : 0;
						}
					}
					Proof->Test->TestEqual(TEXT("Exactly one real Controller key binding"), OwnerKeyBindings, 1);
				}
				const bool bTouchReleasedAtDestination =
				    Player.PlayerInput && Player.PlayerInput->GetTouchStates()[ETouchIndex::Touch1].Pressure == 0 &&
				    FVector2D(Player.PlayerInput->GetTouchStates()[ETouchIndex::Touch1].ViewportRelativeLocation)
				        .Equals(View.TouchEnd, .1);
				Proof->Test->TestTrue(TEXT("Engine touch ended at projected destination"), bTouchReleasedAtDestination);
				int32 OwnerTouchBindings = 0;
				if (Player.InputComponent)
				{
					for (const FInputTouchBinding& Binding : Player.InputComponent->TouchBindings)
					{
						OwnerTouchBindings += (Binding.KeyEvent == IE_Pressed || Binding.KeyEvent == IE_Released) &&
						                              Binding.TouchDelegate.GetDelegate().IsBoundToObject(&Player)
						                          ? 1
						                          : 0;
					}
				}
				Proof->Test->TestEqual(TEXT("Real Controller touch begin/end bindings retained"), OwnerTouchBindings,
				                            2);
				TSharedPtr<FJsonObject> State = MakeShared<FJsonObject>();
				State->SetStringField(TEXT("role"), Index == 0 ? TEXT("host") : TEXT("client"));
				State->SetStringField(TEXT("matchId"), View.Battle.MatchId.ToString());
				State->SetNumberField(TEXT("battleRevisionBefore"), View.Battle.Revision);
				State->SetNumberField(
				    TEXT("battleRevisionAfter"),
				         Player.GetWorld()->GetGameState<ALDGameState>()->GetBattleSnapshot().Revision);
				State->SetNumberField(TEXT("boardRevisionBefore"), View.ServerBoard.BoardRevision);
				State->SetNumberField(TEXT("boardRevisionAfter"),
				                           Mode.GetBoardManager()->GetSnapshot(Index).BoardRevision);
				State->SetNumberField(TEXT("economyRevisionBefore"), View.ServerEconomy.EconomyRevision);
				State->SetNumberField(TEXT("economyRevisionAfter"),
				                           Mode.GetEconomyService()->GetSnapshot(Index).EconomyRevision);
				State->SetNumberField(TEXT("goldBefore"), View.ServerEconomy.Gold);
				State->SetNumberField(TEXT("goldAfter"), Mode.GetEconomyService()->GetSnapshot(Index).Gold);
				State->SetNumberField(TEXT("cachedResultsBefore"), View.CachedResults);
				State->SetNumberField(TEXT("cachedResultsAfter"),
				                           Mode.GetCommandProcessor()->GetCachedResultCount(Index));
				State->SetNumberField(TEXT("randomStateBefore"), View.RandomState);
				State->SetNumberField(TEXT("randomStateAfter"), Mode.GetEconomyService()->GetRandomState(Index));
				State->SetNumberField(TEXT("lastRequestIdBefore"), View.LastResult.RequestId);
				State->SetNumberField(TEXT("lastRequestIdAfter"), Player.GetLastResult().RequestId);
				State->SetBoolField(TEXT("touchReleasedAtDestination"), bTouchReleasedAtDestination);
				States.Add(MakeShared<FJsonValueObject>(State));
			}
			Proof->TerminalUI->SetArrayField(TEXT("stateBeforeAfter"), States);
		}
		TSharedRef<FPIEProof> Proof;
		FTerminalViewBaseline TerminalViews[2];
		TWeakObjectPtr<UWorld> SourceMapAsset;
		double StartedAt = 0;
		double RunningObservedAt = 0;
		double TerminalStepAt = 0;
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
			Proof->Test->TestTrue(TEXT("Own read-only event observer restored"), Proof->bEventObserverRestored);
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
	if (!GEditor || GEditor->PlayWorld || !FSlateApplication::IsInitialized() ||
	    FParse::Param(FCommandLine::Get(), TEXT("nullrhi")) ||
	                  FParse::Param(FCommandLine::Get(), TEXT("DisableTouch")) ||
	                                AActor::ProcessEventDelegate.IsBound())
	{
		AddError(
		    TEXT("PIE test requires an idle GPU Editor with Slate, touch enabled and an unbound Actor event observer"));
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
