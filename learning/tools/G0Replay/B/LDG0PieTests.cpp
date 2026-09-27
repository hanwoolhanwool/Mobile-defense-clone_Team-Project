// Provided replay fixture only. G0 product files remain at their original lesson source.
#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Core/LDGameMode.h"
#include "Core/LDGameState.h"
#include "Core/LDPlayerController.h"
#include "Core/LDPlayerState.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Network/LDCommandProcessor.h"
#include "Serialization/JsonSerializer.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Tests/AutomationEditorCommon.h"
#include "UObject/UnrealType.h"

#ifndef LD_G0_CANONICAL_PIE
#define LD_G0_CANONICAL_PIE 0
#endif

#if DO_ENABLE_NET_TEST
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

	FString ExportPacketSettings(const FPacketSimulationSettings& Settings)
	{
		FString Result;
		FPacketSimulationSettings::StaticStruct()->ExportText(Result, &Settings, nullptr, nullptr, PPF_None, nullptr);
		return Result;
	}

	struct FG0PIEProof
	{
		FAutomationTestBase* Test = nullptr;
		ULevelEditorPlaySettings* Original = nullptr;
		FString OriginalConfig;
		FString OutputDirectory;
		TWeakObjectPtr<ALDPlayerController> Host;
		TWeakObjectPtr<ALDPlayerController> Client;
		TWeakObjectPtr<ALDPlayerController> RemoteServer;
		TWeakObjectPtr<UNetDriver> LossDriver;
		FPacketSimulationSettings OriginalPackets;
		FString OriginalPacketsText;
		FDelegateHandle EventObserver;
		FDelegateHandle HostCompletedHandle;
		FDelegateHandle ClientCompletedHandle;
		TArray<FLDCommand> ServerReceived;
		TArray<FLDCommandResult> ClientReceived;
		TArray<FLDCommandResult> Completed;
		TArray<TSharedPtr<FJsonValue>> Observations;
		int32 HostCompleted = 0;
		bool bLossApplied = false;
		bool bPacketsRestored = true;
		bool bObserverRestored = true;
		bool bRestored = false;

		~FG0PIEProof()
		{
			StopObservation();
			RestoreSettings();
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
		void Watch()
		{
			// Read-only dispatch observation, scoped to these two actors. Return false to preserve dispatch.
			AActor::ProcessEventDelegate.BindLambda(
			    [this](AActor* Actor, UFunction* Function, void* Parameters)
			    {
				    if (Actor == RemoteServer.Get() && Function->GetFName() == TEXT("ServerRequestCommand"))
				    {
					    const FStructProperty* Property = FindFProperty<FStructProperty>(Function, TEXT("Command"));
					    if (Property)
					    {
						    ServerReceived.Add(*Property->ContainerPtrToValuePtr<FLDCommand>(Parameters));
					    }
				    }
				    if (Actor == Client.Get() && Function->GetFName() == TEXT("ClientCommandResult"))
				    {
					    const FStructProperty* Property = FindFProperty<FStructProperty>(Function, TEXT("Result"));
					    if (Property)
					    {
						    ClientReceived.Add(*Property->ContainerPtrToValuePtr<FLDCommandResult>(Parameters));
					    }
				    }
				    return false;
			    });
			EventObserver = AActor::ProcessEventDelegate.GetHandle();
			bObserverRestored = false;
			HostCompletedHandle =
			    Host->OnCommandCompleted.AddLambda([this](const FLDCommandResult&) { ++HostCompleted; });
			ClientCompletedHandle =
			    Client->OnCommandCompleted.AddLambda([this](const FLDCommandResult& Result) { Completed.Add(Result); });
		}
		void ApplyLoss()
		{
			LossDriver = Client->GetWorld()->GetNetDriver();
			OriginalPackets = LossDriver->PacketSimulationSettings;
			OriginalPacketsText = ExportPacketSettings(OriginalPackets);
			FPacketSimulationSettings Loss = OriginalPackets;
			Loss.PktLoss = 100;
			LossDriver->SetPacketSimulationSettings(Loss);
			Test->TestEqual(TEXT("Client driver accepted transport loss setting"),
			                     LossDriver->PacketSimulationSettings.PktLoss, 100);
			bLossApplied = true;
			bPacketsRestored = false;
			Record(
			    TEXT("packet-loss-start"),
			         TEXT("Only client PIE NetDriver outgoing PktLoss=100; no application response-token exhaustion"));
		}
		void RestorePackets()
		{
			if (bLossApplied)
			{
				if (LossDriver.IsValid())
				{
					LossDriver->SetPacketSimulationSettings(OriginalPackets);
					bPacketsRestored =
					    ExportPacketSettings(LossDriver->PacketSimulationSettings) == OriginalPacketsText;
				}
				bLossApplied = false;
			}
		}
		void StopObservation()
		{
			RestorePackets();
			if (Host.IsValid())
			{
				Host->OnCommandCompleted.Remove(HostCompletedHandle);
			}
			if (Client.IsValid())
			{
				Client->OnCommandCompleted.Remove(ClientCompletedHandle);
			}
			if (!bObserverRestored && AActor::ProcessEventDelegate.GetHandle() == EventObserver)
			{
				AActor::ProcessEventDelegate.Unbind();
				bObserverRestored = true;
			}
		}
		void RestoreSettings()
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
				bRestored = ExportPlayConfig(*Settings) == OriginalConfig;
			}
		}
		int32 Requests(uint32 Id) const
		{
			return ServerReceived.FilterByPredicate([Id](const FLDCommand& C) { return C.RequestId == Id; }).Num();
		}
		int32 Responses(uint32 Id, ELDCommandResultCode Code) const
		{
			return ClientReceived
			    .FilterByPredicate([Id, Code](const FLDCommandResult& R)
			                       { return R.RequestId == Id && R.ResultCode == Code; })
			    .Num();
		}
		int32 TerminalCompletions() const
		{
			return Completed
			    .FilterByPredicate([](const FLDCommandResult& R)
			                       { return R.ResultCode != ELDCommandResultCode::Pending; })
			    .Num();
		}
		void Save()
		{
			TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
			Root->SetStringField(TEXT("kind"), TEXT("actual-editor-PIE-G0-owned-RPC"));
			Root->SetStringField(TEXT("result"), Test->HasAnyErrors() ? TEXT("Fail") : TEXT("Pass"));
			Root->SetStringField(TEXT("productSource"),
			                          LD_G0_CANONICAL_PIE ? TEXT("649c1dedd6832c41089a76b59bc76518cd262296")
			                                              : TEXT("03acb67e95a804b2d49e4f17fa3d4ec5a41dfd92"));
			Root->SetBoolField(TEXT("canonicalTerminalVariant"), LD_G0_CANONICAL_PIE != 0);
			Root->SetNumberField(TEXT("processId"), FPlatformProcess::GetCurrentProcessId());
			Root->SetBoolField(TEXT("settingsRestored"), bRestored);
			Root->SetBoolField(TEXT("packetSettingsRestored"), bPacketsRestored);
			Root->SetBoolField(TEXT("eventObserverRestored"), bObserverRestored);
			Root->SetNumberField(TEXT("terminalCompletions"), TerminalCompletions());
			Root->SetArrayField(TEXT("observations"), Observations);
			Root->SetStringField(TEXT("limits"), TEXT("Native GameMode override, G0 closed Stub admission. No board/economy effects, BP wiring, gameplay UI, package, Android or G3 claim. Stale/Pending replies are explicit server-RPC fixtures; loss is client transport only, not response-rate limiting."));
			TArray<TSharedPtr<FJsonValue>> RequestsJson;
			for (const FLDCommand& C : ServerReceived)
			{
				TSharedPtr<FJsonObject> Item = MakeShared<FJsonObject>();
				Item->SetNumberField(TEXT("requestId"), C.RequestId);
				Item->SetNumberField(TEXT("epoch"), C.ConnectionEpoch);
				Item->SetNumberField(TEXT("source"), C.Source);
				RequestsJson.Add(MakeShared<FJsonValueObject>(Item));
			}
			Root->SetArrayField(TEXT("actualServerReceivedRequests"), RequestsJson);
			TArray<TSharedPtr<FJsonValue>> ResultsJson;
			for (const FLDCommandResult& R : ClientReceived)
			{
				TSharedPtr<FJsonObject> Item = MakeShared<FJsonObject>();
				Item->SetStringField(TEXT("matchId"), R.MatchId.ToString());
				Item->SetNumberField(TEXT("epoch"), R.ConnectionEpoch);
				Item->SetNumberField(TEXT("requestId"), R.RequestId);
				Item->SetNumberField(TEXT("resultCode"), static_cast<uint8>(R.ResultCode));
				Item->SetNumberField(TEXT("eventId"), R.EventId);
				ResultsJson.Add(MakeShared<FJsonValueObject>(Item));
			}
			Root->SetArrayField(TEXT("actualClientReceivedResults"), ResultsJson);
			FString Json;
			FJsonSerializer::Serialize(Root, TJsonWriterFactory<>::Create(&Json));
			if (!FFileHelper::SaveStringToFile(Json, *(OutputDirectory / TEXT("pie-proof.json"))))
			{
				Test->AddError(TEXT("Could not save G0 PIE proof"));
			}
		}
	};

	class FVerifyG0PIE final : public IAutomationLatentCommand
	{
	public:
		explicit FVerifyG0PIE(TSharedRef<FG0PIEProof> InProof) : Proof(InProof) {}
		virtual bool Update() override
		{
			const double Now = FPlatformTime::Seconds();
			if (!StartedAt)
				StartedAt = Now;
			if (Now - StartedAt > 60)
			{
				Proof->Test->AddError(FString::Printf(TEXT("G0 PIE timed out at stage %d"), Stage));
				return Finish();
			}
			if (Proof->Test->HasAnyErrors())
				return Finish();
			ALDPlayerController* Host = nullptr;
			ALDPlayerController* Client = nullptr;
			ALDPlayerController* Remote = nullptr;
			int32 Worlds = 0;
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				UWorld* World = Context.World();
				if (Context.WorldType != EWorldType::PIE || !World)
					continue;
				++Worlds;
				for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
				{
					ALDPlayerController* PC = Cast<ALDPlayerController>(It->Get());
					if (!PC)
						continue;
					if (World->GetNetMode() == NM_ListenServer)
					{
						if (PC->IsLocalController())
							Host = PC;
						else
							Remote = PC;
					}
					else if (World->GetNetMode() == NM_Client && PC->IsLocalController())
						Client = PC;
				}
			}
			if (Worlds != 2 || !Host || !Client || !Remote)
				return false;
			ALDGameState* HS = Host->GetWorld()->GetGameState<ALDGameState>();
			ALDGameState* CS = Client->GetWorld()->GetGameState<ALDGameState>();
			ALDPlayerState* HP = Host->GetPlayerState<ALDPlayerState>();
			ALDPlayerState* CP = Client->GetPlayerState<ALDPlayerState>();
			ALDPlayerState* RP = Remote->GetPlayerState<ALDPlayerState>();
			ALDGameMode* Mode = Host->GetWorld()->GetAuthGameMode<ALDGameMode>();
			if (!HS || !CS || !HP || !CP || !RP || !Mode || !Client->GetWorld()->GetNetDriver())
				return false;
			const FObjectPropertyBase* ProcessorProperty =
			    FindFProperty<FObjectPropertyBase>(Mode->GetClass(), TEXT("CommandProcessor"));
			ULDCommandProcessor* Processor =
			    ProcessorProperty
			        ? Cast<ULDCommandProcessor>(ProcessorProperty->GetObjectPropertyValue_InContainer(Mode))
			        : nullptr;
			if (!Processor || !HP->GetParticipantContext().IsValid() || !CP->GetParticipantContext().IsValid())
				return false;
			const FLDParticipantContext HC = HP->GetParticipantContext();
			const FLDParticipantContext CC = CP->GetParticipantContext();
			FLDCommand Command;
			if (Stage == 0)
			{
				const FStructProperty* MatchProperty =
				    FindFProperty<FStructProperty>(Client->GetClass(), TEXT("CurrentMatchId"));
				const FUInt64Property* EpochProperty =
				    FindFProperty<FUInt64Property>(Client->GetClass(), TEXT("ConnectionEpoch"));
				if (!MatchProperty || !EpochProperty)
				{
					Proof->Test->AddError(TEXT("G0 owner-context properties missing; wrong product source"));
					return Finish();
				}
				if (*MatchProperty->ContainerPtrToValuePtr<FGuid>(Client) != CC.MatchId ||
				    EpochProperty->GetPropertyValue_InContainer(Client) != CC.ConnectionEpoch ||
				    HS->GetPhase() != ELDMatchPhase::Preparing || CS->GetPhase() != ELDMatchPhase::Preparing ||
				    CS->PlayerArray.Num() != 2)
					return false;
				Proof->Test->TestEqual(TEXT("Remote match replicated"), CS->GetMatchContext().MatchId,
				                            HS->GetMatchContext().MatchId);
				Proof->Test->TestEqual(TEXT("Host participant index"), HC.PlayerIndex, 0);
				Proof->Test->TestEqual(TEXT("Remote participant index"), CC.PlayerIndex, 1);
				Proof->Test->TestEqual(TEXT("Owner context belongs to the shared match"), CC.MatchId,
				                            HS->GetMatchContext().MatchId);
				Proof->Test->TestTrue(TEXT("Participant epochs are distinct"),
				                           HC.ConnectionEpoch != CC.ConnectionEpoch);
				Proof->Test->TestEqual(TEXT("Remote server counterpart belongs to the owning client's epoch"),
				                            RP->GetParticipantContext().ConnectionEpoch, CC.ConnectionEpoch);
				Proof->Test->TestTrue(TEXT("RPC crosses client and authority with a real net connection"),
				                           !Client->HasAuthority() && Remote->HasAuthority() &&
				                               Remote->GetNetConnection() != nullptr &&
				                               Client->GetWorld()->GetNetDriver()->ServerConnection != nullptr);
				Proof->Test->TestEqual(TEXT("Both public participant states reached client"), CS->PlayerArray.Num(), 2);
				Proof->Host = Host;
				Proof->Client = Client;
				Proof->RemoteServer = Remote;
				if (AActor::ProcessEventDelegate.IsBound())
				{
					Proof->Test->AddError(TEXT("Existing ProcessEvent observer will not be replaced"));
					return Finish();
				}
				Proof->Watch();
				Proof->Record(TEXT("worlds"), FString::Printf(TEXT("distinct PIE worlds host=%s client=%s match=%s indices=0/1 epochs=%llu/%llu; native override; Preparing Stub"),
				                                  *Host->GetWorld()->GetPathName(), *Client->GetWorld()->GetPathName(),
				                                  *CC.MatchId.ToString(), HC.ConnectionEpoch, CC.ConnectionEpoch));
				Proof->Test->TestTrue(TEXT("Host local submit admitted to closed-phase check"),
				                           Host->SubmitLocalCommand(Command));
				Proof->Test->TestTrue(TEXT("Owning remote submit invokes real RPC"),
				                           Client->SubmitLocalCommand(Command));
				Proof->Test->TestTrue(TEXT("Remote pending retained before response"), Client->HasPendingCommand());
				Proof->Test->TestFalse(TEXT("Second new intent blocked while pending"),
				                            Client->SubmitLocalCommand(Command));
				Proof->Test->TestTrue(TEXT("Retry uses same pending request"), Client->RetryPendingCommand());
				Stage = 1;
				return false;
			}
			if (Stage == 1)
			{
				if (Proof->Requests(1) < 2 || Proof->Responses(1, ELDCommandResultCode::PhaseNotAllowed) < 2)
					return false;
				Proof->Test->TestEqual(TEXT("G0 cannot fabricate a purchase success"), Host->GetLastResult().ResultCode,
				                            ELDCommandResultCode::PhaseNotAllowed);
				Proof->Test->TestEqual(TEXT("Host completes locally once, not counted as network RPC"),
				                            Proof->HostCompleted, 1);
				Proof->Test->TestEqual(TEXT("Duplicate real replies complete remote intent once"),
				                            Proof->TerminalCompletions(), 1);
				Proof->Test->TestFalse(TEXT("Real final response clears pending"), Client->HasPendingCommand());
				Proof->Test->TestEqual(TEXT("Two actual remote deliveries keep one cache entry"),
				                            Processor->GetCachedResultCount(1), 1);
				Proof->Test->TestEqual(TEXT("Host cache is independent"), Processor->GetCachedResultCount(0), 1);
				Proof->Test->TestFalse(TEXT("Nonlocal server counterpart cannot submit a local intent"),
				                            Remote->SubmitLocalCommand(Command));
				Command.ConnectionEpoch = HC.ConnectionEpoch;
				Command.RequestId = 77;
				Client->ServerRequestCommand(Command); // Real owning transport; forged other-participant epoch.
				Proof->Record(TEXT("owned-rpc"), TEXT("request1 twice on server, PhaseNotAllowed twice on client, one completion/cache entry; nonlocal intent refused"));
				Stage = 2;
				return false;
			}
			if (Stage == 2)
			{
				if (!Proof->Requests(77) || !Proof->Responses(77, ELDCommandResultCode::InvalidEpoch))
					return false;
				Proof->Test->TestEqual(TEXT("Foreign owner epoch cannot add a cache entry"),
				                            Processor->GetCachedResultCount(1), 1);
				Proof->Test->TestEqual(TEXT("Unmatched reply cannot complete a local intent"),
				                            Proof->TerminalCompletions(), 1);
				Proof->Test->TestEqual(TEXT("Unmatched reply leaves last result intact"),
				                            Client->GetLastResult().RequestId, uint32(1));
				Proof->Record(TEXT("owner-mismatch"),
				                   TEXT("owning client RPC with host epoch received InvalidEpoch; client completion/cache unchanged; payload has no player index"));
				Proof->ApplyLoss();
				Proof->Test->TestTrue(TEXT("Second intent begins during transport loss"),
				                           Client->SubmitLocalCommand(Command));
				Proof->Test->TestTrue(TEXT("Same pending retry during transport loss"), Client->RetryPendingCommand());
				StageAt = Now;
				Stage = 3;
				return false;
			}
			if (Stage == 3)
			{
				if (Now - StageAt < .4)
					return false;
				Proof->Test->TestEqual(TEXT("100 percent client packet loss prevents request2 server dispatch"),
				                            Proof->Requests(2), 0);
				Proof->Test->TestTrue(TEXT("Transport loss retains pending"), Client->HasPendingCommand());
				FLDCommandResult Fixture;
				Fixture.MatchId = FGuid::NewGuid();
				Fixture.ConnectionEpoch = CC.ConnectionEpoch;
				Fixture.RequestId = 2;
				Fixture.ResultCode = ELDCommandResultCode::Success;
				Fixture.EventId = 701;
				Remote->ClientCommandResult(Fixture);
				Fixture.MatchId = CC.MatchId;
				Fixture.ConnectionEpoch = HC.ConnectionEpoch;
				Fixture.EventId = 702;
				Remote->ClientCommandResult(Fixture);
				Fixture.ConnectionEpoch = CC.ConnectionEpoch;
				Fixture.ResultCode = ELDCommandResultCode::Pending;
				Fixture.EventId = 703;
				Remote->ClientCommandResult(Fixture);
				Proof->Record(TEXT("reply-fixtures"), TEXT("server Client RPC injects old match701, old epoch702 and matching Pending703; no direct implementation call"));
				Stage = 4;
				return false;
			}
			if (Stage == 4)
			{
				if (Proof->ClientReceived
				        .FilterByPredicate([](const FLDCommandResult& R)
				                           { return R.EventId >= 701 && R.EventId <= 703; })
				        .Num() < 3)
					return false;
				Proof->Test->TestEqual(TEXT("Old match/epoch finals rejected; only matching Pending notified"),
				                            Proof->Completed.Num(), 2);
				Proof->Test->TestEqual(TEXT("Pending is not a second terminal completion"),
				                            Proof->TerminalCompletions(), 1);
				Proof->Test->TestTrue(TEXT("Matching Pending retains request2"), Client->HasPendingCommand());
				Proof->Test->TestEqual(TEXT("Only matching fixture can change last result"),
				                            Client->GetLastResult().EventId, uint64(703));
				Proof->RestorePackets();
				Proof->Test->TestTrue(TEXT("Same request retries after transport restoration"),
				                           Client->RetryPendingCommand());
				Proof->Record(TEXT("packet-loss-end"),
				                   TEXT("all original client NetDriver settings restored; same request2 retried"));
				Stage = 5;
				return false;
			}
			if (Stage == 5)
			{
				if (Proof->Requests(2) < 3 || Proof->Responses(2, ELDCommandResultCode::PhaseNotAllowed) < 3)
					return false;
				Proof->Test->TestEqual(TEXT("Three request2 deliveries still produce only one terminal completion"),
				                            Proof->TerminalCompletions(), 2);
				Proof->Test->TestEqual(TEXT("Only two remote request identities are cached"),
				                            Processor->GetCachedResultCount(1), 2);
				Proof->Test->TestFalse(TEXT("Recovered final reply clears pending"), Client->HasPendingCommand());
				Proof->Test->TestEqual(TEXT("Recovered request number unchanged"), Client->GetLastResult().RequestId,
				                            uint32(2));
				Proof->Record(TEXT("recovered"),
				                   TEXT("request2 retained through client transport loss; three dispatches, one new cache entry/terminal completion; admission remains G0 Stub"));
#if LD_G0_CANONICAL_PIE
				BeforeTerminal = Client->GetLastResult();
				Proof->Test->AddExpectedError(TEXT("Match aborted: G0 canonical PIE terminal fixture"),
				                                   EAutomationExpectedErrorFlags::Contains, 1);
				Mode->AbortMatch(TEXT("G0 canonical PIE terminal fixture"));
				Stage = 6;
				return false;
#else
				return Finish();
#endif
			}
#if LD_G0_CANONICAL_PIE
			if (Stage == 6)
			{
				if (HS->GetPhase() != ELDMatchPhase::Aborted || CS->GetPhase() != ELDMatchPhase::Aborted)
					return false;
				TerminalResponseStart = Proof->ClientReceived.Num();
				Command.ConnectionEpoch = CC.ConnectionEpoch;
				Command.RequestId = 2;
				Client->ServerRequestCommand(Command);
				Command.Source = 1;
				Client->ServerRequestCommand(Command);
				Command.Source = 0;
				Command.RequestId = 3;
				Client->ServerRequestCommand(Command);
				Stage = 7;
				return false;
			}
			if (Proof->ClientReceived.Num() - TerminalResponseStart < 3)
				return false;
			const FLDCommandResult& Same = Proof->ClientReceived[TerminalResponseStart];
			const FLDCommandResult& Changed = Proof->ClientReceived[TerminalResponseStart + 1];
			const FLDCommandResult& New = Proof->ClientReceived[TerminalResponseStart + 2];
			Proof->Test->TestEqual(TEXT("Canonical close retains identical cached result"), Same.ResultCode,
			                            ELDCommandResultCode::PhaseNotAllowed);
			Proof->Test->TestTrue(
			    TEXT("Every cached response field survives canonical close"),
			         FLDCommandResult::StaticStruct()->CompareScriptStruct(&Same, &BeforeTerminal, 0));
			Proof->Test->TestEqual(TEXT("Changed same request remains conflict after close"), Changed.ResultCode,
			                            ELDCommandResultCode::RequestIdConflict);
			Proof->Test->TestEqual(TEXT("Changed payload retained its request number"), Changed.RequestId, uint32(2));
			Proof->Test->TestEqual(TEXT("New request cannot execute after close"), New.ResultCode,
			                            ELDCommandResultCode::PhaseNotAllowed);
			Proof->Test->TestEqual(TEXT("New rejected request has a new number"), New.RequestId, uint32(3));
			Proof->Test->TestEqual(TEXT("Terminal raw replies cannot complete an absent pending intent"),
			                            Proof->TerminalCompletions(), 2);
			Proof->Test->TestEqual(TEXT("Only new rejected identity adds one cached outcome"),
			                            Processor->GetCachedResultCount(1), 3);
			Proof->Record(TEXT("canonical-terminal"),
			                   TEXT("AbortMatch replicated; actual raw replies identical=PhaseNotAllowed, changed=RequestIdConflict, new=PhaseNotAllowed; cache3/completions2"));
#endif
			return Finish();
		}

	private:
		bool Finish()
		{
			Proof->StopObservation();
			return true;
		}
		TSharedRef<FG0PIEProof> Proof;
		double StartedAt = 0;
		double StageAt = 0;
		int32 Stage = 0;
		int32 TerminalResponseStart = 0;
		FLDCommandResult BeforeTerminal;
	};

	class FRestoreG0PIE final : public IAutomationLatentCommand
	{
	public:
		explicit FRestoreG0PIE(TSharedRef<FG0PIEProof> InProof) : Proof(InProof) {}
		virtual bool Update() override
		{
			if (!StartedAt)
				StartedAt = FPlatformTime::Seconds();
			int32 Remaining = 0;
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
				Remaining += Context.WorldType == EWorldType::PIE;
			if (Remaining && FPlatformTime::Seconds() - StartedAt < 15)
				return false;
			Proof->StopObservation();
			Proof->RestoreSettings();
			Proof->Test->TestEqual(TEXT("PIE Worlds removed"), Remaining, 0);
			Proof->Test->TestTrue(TEXT("Play config restored"), Proof->bRestored);
			Proof->Test->TestTrue(TEXT("Client-only packet settings restored"), Proof->bPacketsRestored);
			Proof->Test->TestTrue(TEXT("Read-only event observer removed"), Proof->bObserverRestored);
			Proof->Record(TEXT("cleanup"),
			    FString::Printf(TEXT("PIE worlds remaining=%d; original settings/network/observer restored=%d/%d/%d"),
			                         Remaining, Proof->bRestored, Proof->bPacketsRestored, Proof->bObserverRestored));
			Proof->Save();
			return true;
		}

	private:
		TSharedRef<FG0PIEProof> Proof;
		double StartedAt = 0;
	};
} // namespace

#if LD_G0_CANONICAL_PIE
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDG0PIETest, "LD.PIE.G0.Canonical.TerminalCache",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
#else
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDG0PIETest, "LD.PIE.G0.B.OwnedRPC",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
#endif
bool FLDG0PIETest::RunTest(const FString& Parameters)
{
	if (!GEditor || GEditor->PlayWorld || !FSlateApplication::IsInitialized() ||
	    AActor::ProcessEventDelegate.IsBound() || FParse::Param(FCommandLine::Get(), TEXT("nullrhi")))
	{
		AddError(
		    TEXT("G0 PIE requires an idle GPU Editor and no existing event observer; existing sessions are preserved"));
		return false;
	}
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		if (Context.WorldType == EWorldType::PIE)
		{
			AddError(TEXT("Existing PIE world will not be replaced"));
			return false;
		}
	}
	FString RunId = TEXT("G0-B-PIE-") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
	FParse::Value(FCommandLine::Get(), TEXT("P0PIERun="), RunId);
	if (RunId.IsEmpty() || RunId.Contains(TEXT("/")) || RunId.Contains(TEXT("\\")) ||
	                                                                   RunId.Contains(TEXT("..")) ||
	                                                                                  RunId.Contains(TEXT(":")))
	{
		AddError(TEXT("P0PIERun must be one new directory name"));
		return false;
	}
	TSharedRef<FG0PIEProof> Proof = MakeShared<FG0PIEProof>();
	Proof->Test = this;
	Proof->OutputDirectory = FPaths::ProjectSavedDir() / TEXT("P0Runs") / RunId;
	if (IFileManager::Get().DirectoryExists(*Proof->OutputDirectory))
	{
		AddError(TEXT("G0 PIE proof directory exists; previous evidence is preserved"));
		return false;
	}
	if (!IFileManager::Get().MakeDirectory(*Proof->OutputDirectory, true))
	{
		AddError(TEXT("Could not create new G0 PIE proof directory"));
		return false;
	}
	Proof->Original =
	    DuplicateObject<ULevelEditorPlaySettings>(GetMutableDefault<ULevelEditorPlaySettings>(), GetTransientPackage());
	Proof->Original->AddToRoot();
	Proof->OriginalConfig = ExportPlayConfig(*Proof->Original);
	ULevelEditorPlaySettings* Settings =
	    DuplicateObject<ULevelEditorPlaySettings>(Proof->Original, GetTransientPackage());
	Settings->SetPlayNetMode(EPlayNetMode::PIE_ListenServer);
	Settings->SetRunUnderOneProcess(true);
	Settings->SetPlayNumberOfClients(2);
	Settings->NewWindowWidth = 540;
	Settings->NewWindowHeight = 720;
	Settings->SetClientWindowSize(FIntPoint(540, 720));
	Settings->GameGetsMouseControl = false;
	Settings->AddToRoot(); // FStartPIEForAutomationCommand releases its temporary settings.
	FRequestPlaySessionParams Request;
	Request.SessionDestination = EPlaySessionDestinationType::InProcess;
	Request.WorldType = EPlaySessionWorldType::PlayInEditor;
	Request.EditorPlaySettings = Settings;
	Request.GlobalMapOverride = TEXT("/Game/TopDown/Lvl_TopDown");
	Request.GameModeOverride = ALDGameMode::StaticClass();
	Request.bAllowOnlineSubsystem = false;
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIEForAutomationCommand(Request));
	ADD_LATENT_AUTOMATION_COMMAND(FVerifyG0PIE(Proof));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(FRestoreG0PIE(Proof));
	return true;
}
#endif // DO_ENABLE_NET_TEST
#endif // WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
