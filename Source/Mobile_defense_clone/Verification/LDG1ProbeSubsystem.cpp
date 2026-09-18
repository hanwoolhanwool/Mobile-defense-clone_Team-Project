#include "Verification/LDG1ProbeSubsystem.h"

#include "Battle/LDEnemyActor.h"
#include "Core/LDGameMode.h"
#include "Core/LDGameState.h"
#include "Core/LDPlayerController.h"
#include "Core/LDPlayerState.h"
#include "Data/LDGameData.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "Misc/EngineVersion.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogLDP0Probe, Log, All);

namespace
{
	const FIntPoint Viewports[] = {{540, 1170}, {1080, 2340}, {720, 1280}, {720, 1600},
	                               {768, 1024}, {800, 1280},  {1280, 720}};

	FVector ExpectedCellCenter(int32 CellId)
	{
		const int32 Owner = CellId / 18;
		const int32 Row = (CellId % 18) / 6;
		return FVector(-350 + (CellId % 6) * 140, Owner == 0 ? -420 + Row * 140 : 140 + Row * 140, 0);
	}

	FVector ExpectedPresentation(FVector Canonical, int32 LocalPlayer)
	{
		if (LocalPlayer == 1)
		{
			Canonical.Y *= -1;
		}
		return Canonical;
	}

	FVector ExpectedRoutePoint(int32 Route, double TotalDistance)
	{
		// Independent rectangular route expectation, not the production sampler or data point array.
		const double Side = Route == 0 ? -1.0 : 1.0;
		const double Distance = FMath::Fmod(TotalDistance, 3080.0);
		if (Distance < 560)
		{
			return FVector(490, Side * (560 - Distance), 0);
		}
		if (Distance < 1540)
		{
			return FVector(490 - (Distance - 560), 0, 0);
		}
		if (Distance < 2100)
		{
			return FVector(-490, Side * (Distance - 1540), 0);
		}
		return FVector(-490 + (Distance - 2100), Side * 560, 0);
	}
} // namespace

bool ULDG1ProbeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
	return false;
#else
	FString Probe;
	const UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld() &&
	       FParse::Value(FCommandLine::Get(), TEXT("P0Probe="), Probe) &&
	                     Probe == TEXT("G1") && Super::ShouldCreateSubsystem(Outer);
#endif
}

void ULDG1ProbeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	CreatedAt = FPlatformTime::Seconds();
	FParse::Value(FCommandLine::Get(), TEXT("P0ProbeOutput="), OutputDirectory);
	if (OutputDirectory.IsEmpty())
	{
		OutputDirectory =
		    FPaths::ProjectSavedDir() /
		    TEXT("P0Runs") / FString::Printf(TEXT("manual-G1-%u"), FPlatformProcess::GetCurrentProcessId());
	}
	OutputDirectory = FPaths::ConvertRelativePathToFull(OutputDirectory);
	IFileManager::Get().MakeDirectory(*OutputDirectory, true);
	UE_LOG(LogLDP0Probe, Display, TEXT("Explicit G1 fixture: two routes, no combat/economy; %s"), *OutputDirectory);
}

TStatId ULDG1ProbeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(ULDG1ProbeSubsystem, STATGROUP_Tickables);
}

bool ULDG1ProbeSubsystem::StartRoutes()
{
	ALDGameMode* Mode = GetWorld()->GetAuthGameMode<ALDGameMode>();
	ALDGameState* State = GetWorld()->GetGameState<ALDGameState>();
	if (!Mode || !State || !Mode->GetGameData())
	{
		return false;
	}
	TSet<int32> Players;
	for (APlayerState* Player : State->PlayerArray)
	{
		if (const ALDPlayerState* Participant = Cast<ALDPlayerState>(Player))
		{
			if (Participant->GetPlayerIndex() >= 0)
			{
				Players.Add(Participant->GetPlayerIndex());
			}
		}
	}
	if (Players.Num() != 2)
	{
		return false;
	}
	RouteStartedAt = GetWorld()->GetTimeSeconds();
	for (int32 Route = 0; Route < 2; ++Route)
	{
		ALDEnemyActor* Enemy = GetWorld()->SpawnActor<ALDEnemyActor>();
		const bool bRouteInitialized =
		    Enemy && Enemy->InitializeRoute(State->GetMatchContext().MatchId, 1001 + Route, Route,
		                                    Mode->GetGameData()->GetRules().PointsByGateCm[Route], 150, RouteStartedAt);
		Check(FString::Printf(TEXT("spawn-route-%d"), Route), bRouteInitialized,
		                      TEXT("Real server actor with fixed fixture ID"));
		if (Enemy)
		{
			ServerRoutes.Add(Enemy);
		}
	}
	NextRouteStep = RouteStartedAt;
	return true;
}

void ULDG1ProbeSubsystem::Check(const FString& Name, bool bPassed, const FString& Detail)
{
	const FString Key = Name + (bPassed ? TEXT(":pass") : TEXT(":fail"));
	if (RecordedChecks.Contains(Key))
	{
		return;
	}
	RecordedChecks.Add(Key);
	TSharedPtr<FJsonObject> Entry = MakeShared<FJsonObject>();
	Entry->SetStringField(TEXT("case"), Name);
	Entry->SetBoolField(TEXT("pass"), bPassed);
	Entry->SetStringField(TEXT("detail"), Detail);
	Checks.Add(MakeShared<FJsonValueObject>(Entry));
	bFailed |= !bPassed;
	if (!bPassed)
	{
		UE_LOG(LogLDP0Probe, Error, TEXT("%s: %s"), *Name, *Detail);
	}
}

void ULDG1ProbeSubsystem::InspectRoutes(ALDPlayerController& Controller)
{
	const double Now = FPlatformTime::Seconds();
	int32 Count = 0;
	for (TActorIterator<ALDEnemyActor> It(GetWorld()); It; ++It)
	{
		ALDEnemyActor* Enemy = *It;
		const FLDEnemyRouteSnapshot& Snapshot = Enemy->GetRouteSnapshot();
		if (Snapshot.EnemyId != 1001 && Snapshot.EnemyId != 1002)
		{
			continue;
		}
		++Count;
		const int32 Route = static_cast<int32>(Snapshot.EnemyId - 1001);
		const FString Label = FString::Printf(TEXT("route-%d"), Route);
		if (!ObservedActors.Contains(Snapshot.EnemyId))
		{
			ObservedActors.Add(Snapshot.EnemyId, Enemy);
		}
		Check(Label + TEXT("-identity"), ObservedActors[Snapshot.EnemyId] == Enemy && Snapshot.RouteIndex == Route,
		                   TEXT("Same actor pointer, EnemyId and RouteIndex throughout two laps"));
		Check(Label + TEXT("-canonical"),
		                   Enemy->GetActorLocation().Equals(ExpectedRoutePoint(Route, Snapshot.TotalDistanceCm), 0.5),
		                   TEXT("Canonical root agrees with independent 560/980/560/980 rectangle"));
		const AGameStateBase* State = GetWorld()->GetGameState();
		const double Prediction =
		    State && Snapshot.bActive
		        ? FMath::Clamp(State->GetServerWorldTimeSeconds() - Snapshot.SampleServerSeconds, 0.0, 0.25)
		        : 0;
		const FVector Display =
		    ExpectedPresentation(
		        ExpectedRoutePoint(Route, Snapshot.TotalDistanceCm + Prediction * Snapshot.SpeedCmPerSecond),
		        LocalPlayerIndex) +
		    FVector(0, 0, 35);
		if (ReadyAt > 0 && Now - ReadyAt > 0.5)
		{
			Check(Label + TEXT("-presentation"),
			                   Enemy->IsPresentationVisible() && Enemy->GetPresentationLocation().Equals(Display, 0.5),
			                   TEXT("Normal view composition produces visible mesh with reflection, capped prediction and 35cm height"));
		}
		if (Snapshot.TotalDistanceCm >= 6160)
		{
			Check(Label + TEXT("-two-laps"), true, FString::Printf(TEXT("distance=%.3f"), Snapshot.TotalDistanceCm));
		}
		if (Now >= NextRouteSample)
		{
			TSharedPtr<FJsonObject> Sample = MakeShared<FJsonObject>();
			Sample->SetNumberField(TEXT("id"), Snapshot.EnemyId);
			Sample->SetNumberField(TEXT("route"), Snapshot.RouteIndex);
			Sample->SetNumberField(TEXT("distanceCm"), Snapshot.TotalDistanceCm);
			Sample->SetStringField(TEXT("canonical"), Enemy->GetActorLocation().ToString());
			Sample->SetStringField(TEXT("presentation"), Enemy->GetPresentationLocation().ToString());
			Samples.Add(MakeShared<FJsonValueObject>(Sample));
		}
	}
	if (ReadyAt > 0 && Now - ReadyAt > 5)
	{
		Check(TEXT("exactly-two-route-actors"), Count == 2, FString::FromInt(Count));
	}
	if (Now >= NextRouteSample)
	{
		NextRouteSample = Now + 5;
	}
}

void ULDG1ProbeSubsystem::InspectViewport(ALDPlayerController& Controller, int32 AspectIndex)
{
	int32 Width = 0;
	int32 Height = 0;
	Controller.GetViewportSize(Width, Height);
	const FString Prefix = FString::Printf(TEXT("view-%d-"), AspectIndex);
	Check(Prefix + TEXT("size"), Width == Viewports[AspectIndex].X && Height == Viewports[AspectIndex].Y,
	                    FString::Printf(TEXT("actual=%dx%d expected=%dx%d"), Width, Height, Viewports[AspectIndex].X,
	                                         Viewports[AspectIndex].Y));
	TArray<FVector2D> Centers;
	Centers.SetNum(36);
	for (int32 Cell = 0; Cell < 36; ++Cell)
	{
		FVector2D Independent;
		FVector2D PublicProjection;
		const bool bProjected = Controller.ProjectWorldLocationToScreen(
		    ExpectedPresentation(ExpectedCellCenter(Cell), LocalPlayerIndex), Independent, false);
		const bool bPublic = Controller.ProjectCellToScreen(Cell, PublicProjection);
		Centers[Cell] = Independent;
		Check(Prefix + FString::Printf(TEXT("projection-%d"), Cell),
		                               bProjected && bPublic && Independent.Equals(PublicProjection, 1.0) &&
		                                   Independent.X > 0 && Independent.Y > 0 && Independent.X < Width &&
		                                   Independent.Y < Height,
		                               Independent.ToString());
		const bool bOwned = Cell / 18 == LocalPlayerIndex;
		const int32 Previous = Controller.GetSelectedCellId();
		const bool bAccepted = Controller.InputScreenPosition(Independent);
		Check(Prefix + FString::Printf(TEXT("input-%d"), Cell),
		                               bAccepted == bOwned &&
		                                   Controller.GetSelectedCellId() == (bOwned ? Cell : Previous),
		                               FString::Printf(TEXT("owned=%d accepted=%d selected=%d"), bOwned, bAccepted,
		                                                    Controller.GetSelectedCellId()));
		if (bOwned)
		{
			for (int32 Corner = 0; Corner < 4; ++Corner)
			{
				FVector Point = ExpectedCellCenter(Cell) + FVector(Corner & 1 ? 62 : -62, Corner & 2 ? 62 : -62, 0);
				FVector2D Screen;
				Controller.ProjectWorldLocationToScreen(ExpectedPresentation(Point, LocalPlayerIndex), Screen, false);
				Check(Prefix +
				      FString::Printf(
				          TEXT("cell-%d-corner-%d"), Cell, Corner),
				          Controller.InputScreenPosition(Screen) && Controller.GetSelectedCellId() == Cell,
				          TEXT("Interior corner of the actual full cell, through the mouse/touch input boundary"));
			}
		}
	}
	const int32 Own = LocalPlayerIndex * 18;
	const int32 Other = (1 - LocalPlayerIndex) * 18;
	Check(Prefix + TEXT("own-bottom"), Centers[Own + 9].Y > Centers[Other + 9].Y, TEXT("Owner board below peer board"));
	const double ColumnPixels = (Centers[Own + 1] - Centers[Own]).Size();
	const double RowPixels = (Centers[Own + 6] - Centers[Own]).Size();
	Check(Prefix + TEXT("square-cells"), FMath::Abs(ColumnPixels - RowPixels) < 1,
	                    FString::Printf(TEXT("column=%.3f row=%.3f"), ColumnPixels, RowPixels));
	FVector2D Spawn;
	FVector2D CenterStart;
	FVector2D CenterEnd;
	Controller.ProjectWorldLocationToScreen(
	    ExpectedPresentation(FVector(490, LocalPlayerIndex == 0 ? -560 : 560, 0), LocalPlayerIndex), Spawn);
	Controller.ProjectWorldLocationToScreen(FVector(490, 0, 0), CenterStart);
	Controller.ProjectWorldLocationToScreen(FVector(-490, 0, 0), CenterEnd);
	Check(Prefix + TEXT("spawn-left"), Spawn.X < Centers[Own + 5].X, Spawn.ToString());
	Check(Prefix + TEXT("shared-direction"), CenterStart.X < CenterEnd.X,
	                    TEXT("Both routes move left to right on central segment"));
	Check(Prefix + TEXT("outside-input"), !Controller.InputScreenPosition(FVector2D(-20, -20)),
	                    TEXT("Outside viewport rejected"));
	TSharedPtr<FJsonObject> View = MakeShared<FJsonObject>();
	View->SetNumberField(TEXT("width"), Width);
	View->SetNumberField(TEXT("height"), Height);
	View->SetNumberField(TEXT("cellPixels"), ColumnPixels);
	const FString Screenshot = OutputDirectory / FString::Printf(TEXT("view-%d.png"), AspectIndex);
	View->SetStringField(TEXT("screenshot"), Screenshot);
	Views.Add(MakeShared<FJsonValueObject>(View));
	FScreenshotRequest::RequestScreenshot(Screenshot, true, false, false, FIntRect(), true);
	Check(Prefix + TEXT("previous-touch-queue-finished"), TouchCell == INDEX_NONE,
	                    TEXT("No input sequence crosses a viewport change"));
	TouchCell = 0;
	TouchPhase = 0;
	TouchAspect = AspectIndex;
	UE_LOG(LogLDP0Probe, Display,
	       TEXT("Viewport %d: %dx%d player=%d; screenshot requested"), AspectIndex, Width, Height, LocalPlayerIndex);
}

void ULDG1ProbeSubsystem::PumpTouchInput(ALDPlayerController& Controller)
{
	if (TouchCell == INDEX_NONE)
	{
		return;
	}
	const FTouchId Finger(FInputDeviceId::CreateFromInternalId(0), ETouchIndex::Touch1);
	if (TouchPhase == 0)
	{
		Controller.ProjectWorldLocationToScreen(ExpectedPresentation(ExpectedCellCenter(TouchCell), LocalPlayerIndex),
		                                        TouchPosition);
		TouchExpectedSelection = TouchCell / 18 == LocalPlayerIndex ? TouchCell : Controller.GetSelectedCellId();
		Controller.InputTouch(Finger, ETouchType::Began, TouchPosition, 1, FPlatformTime::Cycles64());
		TouchPhase = 1;
	}
	else if (TouchPhase == 1)
	{
		TouchPhase = 2;
	}
	else
	{
		Check(FString::Printf(
		    TEXT("view-%d-engine-touch-%d"), TouchAspect, TouchCell),
		    Controller.GetSelectedCellId() == TouchExpectedSelection,
		    FString::Printf(TEXT("Engine InputTouch -> PlayerInput -> bound callback; expected=%d actual=%d"),
		                         TouchExpectedSelection, Controller.GetSelectedCellId()));
		Controller.InputTouch(Finger, ETouchType::Ended, TouchPosition, 0, FPlatformTime::Cycles64());
		TouchPhase = 0;
		++TouchCell;
		if (TouchCell == 36)
		{
			TouchCell = INDEX_NONE;
		}
	}
}

void ULDG1ProbeSubsystem::Tick(float DeltaTime)
{
	if (bFinished || !GetWorld()->HasBegunPlay())
	{
		return;
	}
	const double Now = FPlatformTime::Seconds();
	if (const ALDGameState* State = GetWorld()->GetGameState<ALDGameState>())
	{
		if (State->GetPhase() == ELDMatchPhase::Aborted || State->GetPhase() == ELDMatchPhase::Result)
		{
			Check(TEXT("fixture-terminal-phase"), false, TEXT("Match ended before G1 fixture completed"));
			Finish();
			return;
		}
	}
	if (Now - CreatedAt > 90)
	{
		Check(TEXT("readiness-timeout"), false,
		           TEXT("Required participant/view/route readiness did not finish in 90s"));
		Finish();
		return;
	}
	if (GetWorld()->GetNetMode() != NM_Client)
	{
		if (!bRoutesStarted)
		{
			bRoutesStarted = StartRoutes();
		}
		if (bRoutesStarted)
		{
			const double ServerNow = GetWorld()->GetTimeSeconds();
			while (NextRouteStep <= ServerNow)
			{
				for (TWeakObjectPtr<ALDEnemyActor> Enemy : ServerRoutes)
				{
					if (Enemy.IsValid())
					{
						Enemy->AdvanceRouteTo(NextRouteStep);
					}
				}
				NextRouteStep += 0.05;
			}
		}
	}
	ALDPlayerController* Controller = Cast<ALDPlayerController>(GetWorld()->GetFirstPlayerController());
	if (!Controller || !Controller->IsLocalBoardReady())
	{
		return;
	}
	LocalPlayerIndex = Controller->GetLocalParticipantIndex();
	InspectRoutes(*Controller);
	if (ReadyAt < 0)
	{
		if (ObservedActors.Num() != 2)
		{
			return;
		}
		ReadyAt = Now;
		FinishAt = Now + (GetWorld()->GetNetMode() == NM_Client ? 61 : 68);
	}
	FrameMilliseconds.Add(DeltaTime * 1000);
	const double Elapsed = Now - ReadyAt;
	PumpTouchInput(*Controller);
	const int32 Stage = FMath::Min(static_cast<int32>(Elapsed / 7), static_cast<int32>(UE_ARRAY_COUNT(Viewports)) - 1);
	if (ResizeStage != Stage)
	{
		ResizeStage = Stage;
		Controller->ConsoleCommand(FString::Printf(TEXT("r.SetRes %dx%dw"), Viewports[Stage].X, Viewports[Stage].Y));
	}
	if (InspectedStage < Stage && Elapsed >= Stage * 7 + 2)
	{
		InspectedStage = Stage;
		InspectViewport(*Controller, Stage);
	}
	if (Now >= FinishAt)
	{
		Finish();
	}
}

void ULDG1ProbeSubsystem::Finish()
{
	if (bFinished)
	{
		return;
	}
	bFinished = true;
	Check(TEXT("required-aspects-observed"), Views.Num() == UE_ARRAY_COUNT(Viewports), FString::FromInt(Views.Num()));
	Check(TEXT("all-engine-touch-sequences-finished"), TouchCell == INDEX_NONE,
	           TEXT("Every viewport completed all 36 engine touch events"));
	for (int32 Route = 0; Route < 2; ++Route)
	{
		Check(FString::Printf(TEXT("final-two-laps-%d"), Route),
		                      RecordedChecks.Contains(FString::Printf(TEXT("route-%d-two-laps:pass"), Route)),
		                                              TEXT("Observed persistent actor past 6160cm"));
	}
	for (int32 Index = 0; Index < Views.Num(); ++Index)
	{
		Check(FString::Printf(TEXT("screenshot-saved-%d"), Index),
		                      IFileManager::Get().FileSize(
		                          *(OutputDirectory / FString::Printf(TEXT("view-%d.png"), Index))) > 1024,
		                          TEXT("Rendered PNG saved; visual review is a separate human/model observation"));
	}
	for (TWeakObjectPtr<ALDEnemyActor> Enemy : ServerRoutes)
	{
		if (Enemy.IsValid())
		{
			Enemy->StopRoute();
		}
	}
	FrameMilliseconds.Sort();
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("scope"), TEXT("Rendered two-process G1 fixture; actual projection and mouse/touch shared input boundary; no physical input or combat/economy"));
	Result->SetStringField(TEXT("result"), bFailed ? TEXT("Fail") : TEXT("Pass"));
	Result->SetStringField(TEXT("engine"), FEngineVersion::Current().ToString());
	Result->SetStringField(TEXT("netMode"),
	                            GetWorld()->GetNetMode() == NM_Client ? TEXT("client") : TEXT("listen-server"));
	Result->SetNumberField(TEXT("localPlayerIndex"), LocalPlayerIndex);
	Result->SetArrayField(TEXT("checks"), Checks);
	Result->SetArrayField(TEXT("routeSamples"), Samples);
	Result->SetArrayField(TEXT("viewports"), Views);
	if (!FrameMilliseconds.IsEmpty())
	{
		Result->SetNumberField(TEXT("frameCount"), FrameMilliseconds.Num());
		Result->SetNumberField(TEXT("frameMsP50"), FrameMilliseconds[FrameMilliseconds.Num() / 2]);
		Result->SetNumberField(TEXT("frameMsP95"),
		                            FrameMilliseconds[FMath::FloorToInt((FrameMilliseconds.Num() - 1) * 0.95)]);
	}
	FString Json;
	FJsonSerializer::Serialize(Result.ToSharedRef(), TJsonWriterFactory<>::Create(&Json));
	FFileHelper::SaveStringToFile(Json, *(OutputDirectory / TEXT("result.json")));
	UE_LOG(LogLDP0Probe, Display,
	       TEXT("G1_RESULT %s checks=%d output=%s"),
	            bFailed ? TEXT("Fail") : TEXT("Pass"), Checks.Num(), *OutputDirectory);
	FPlatformMisc::RequestExit(false);
}

void ULDG1ProbeSubsystem::Deinitialize()
{
	for (TWeakObjectPtr<ALDEnemyActor> Enemy : ServerRoutes)
	{
		if (Enemy.IsValid())
		{
			Enemy->StopRoute();
		}
	}
	ServerRoutes.Empty();
	ObservedActors.Empty();
	Super::Deinitialize();
}
